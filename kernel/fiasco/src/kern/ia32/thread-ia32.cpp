INTERFACE [ia32]:

class Trap_state;

EXTENSION class Thread
{
private:
  // small address space stuff
  bool        handle_smas_gp_fault ();
  static int  (*int3_handler)(Trap_state*);
};

//---------------------------------------------------------------------------
IMPLEMENTATION [ia32]:

#include "fpu_state.h"
#include "gdt.h"
#include "globalconfig.h"
#include "idt.h"
#include "io.h"
#include "kernel_console.h"
#include "simpleio.h"
#include "terminate.h"
#include "utcb_init.h"
#include "watchdog.h"

int  (*Thread::int3_handler)(Trap_state*) = handle_int3;

IMPLEMENT static inline NEEDS ["gdt.h"]
Mword const
Thread::exception_cs()
{
  return Gdt::gdt_code_user | Gdt::Selector_user;
}

/**
 * The ia32 specific part of the thread constructor.
 */
PRIVATE inline NEEDS ["gdt.h"]
void
Thread::arch_init()
{
  // clear out user regs that can be returned from the thread_ex_regs
  // system call to prevent covert channel
  Entry_frame *r = regs();
  r->sp(0);
  r->ip(0);
  if(Config::enable_io_protection)
    r->flags(EFLAGS_IOPL_K | EFLAGS_IF | 2);	// ei
  else
    r->flags(EFLAGS_IOPL_U | EFLAGS_IF | 2);	// XXX iopl=kernel
  r->cs(Gdt::gdt_code_user | Gdt::Selector_user);
  r->ss(Gdt::gdt_data_user | Gdt::Selector_user);

#ifdef CONFIG_HANDLE_SEGMENTS
  _fs = Gdt::gdt_data_user | Gdt::Selector_user;
  _gs = Utcb_init::gs_value();
#endif

  // make sure the thread's kernel stack is mapped in its address space
  _task->kmem_update(this);
}

/** Extra version of timer interrupt handler which is used when the jdb is
    active to prevent busy waiting. */
extern "C" 
void
thread_timer_interrupt_stop(void)
{
  Timer::acknowledge();
}

/** Slow version of timer interrupt.  Activated on every clock tick.
    Checks if something related to debugging is to do. After returning
    from this function, the real timer interrupt handler is called.
 */
extern "C" 
void
thread_timer_interrupt_slow(void)
{
  if (Config::esc_hack)
    {
      // <ESC> timer hack: check if ESC key hit at keyboard
      if (Io::in8(0x60) == 1)
        kdb_ke("ESC");
    }

  if (Config::serial_esc != Config::SERIAL_NO_ESC)
    {
      // Here we have to check for serial characters because the 
      // serial interrupt could have an lower priority than a not
      // acknowledged interrupt. The regular case is to stop when
      // receiving the serial interrupt.
      if(Kconsole::console()->char_avail()==1)
        kdb_ke("SERIAL_ESC");
    }

  if (Config::watchdog)
    {
      // tell doggy that we are alive
      Watchdog::touch();
    }
}

IMPLEMENT
void
Thread::user_invoke()
{
  user_invoke_generic();

  asm volatile
    ("  movl %%eax,%%esp \n"    // set stack pointer to regs structure
     "  movl %%ecx,%%es  \n"
     "  movl %%ecx,%%ds  \n"
     "  xorl %%eax,%%eax \n"    // clean out user regs
     "  xorl %%ecx,%%ecx \n"
     "  xorl %%edx,%%edx \n"
     "  xorl %%esi,%%esi \n"
     "  xorl %%edi,%%edi \n" 
     "  xorl %%ebx,%%ebx \n"
     "  xorl %%ebp,%%ebp \n"
     "  iret             \n"
     :                          // no output
     : "a" (nonull_static_cast<Return_frame*>(current()->regs())),
       "c" (Gdt::gdt_data_user | Gdt::Selector_user)
     );

  // never returns here
}

/*
 * Handle FPU trap for this context. Assumes disabled interrupts
 */
PUBLIC inline NEEDS ["fpu_alloc.h","fpu_state.h"]
void
Thread::handle_fpu_trap()
{
  // If we own the FPU, we should never be getting an "FPU unavailable" trap
  assert (Fpu::owner() != this);

  // Enable the FPU before accessing it, otherwise recursive trap
  Fpu::enable();

  // Save the FPU state of the previous FPU owner (lazy) if applicable
  if (Fpu::owner()) {
    Fpu::owner()->state_change_dirty (~Thread_fpu_owner, 0);
    Fpu::save_state (Fpu::owner()->fpu_state());
  }

  // Allocate FPU state slab if we didn't already have one
  if (!fpu_state()->state_buffer())
    Fpu_alloc::alloc_state (fpu_state());

  // Become FPU owner and restore own FPU state
  state_change_dirty (~0U, Thread_fpu_owner);
  Fpu::restore_state (fpu_state());

  Fpu::set_owner (this);
}

/** A C interface for Context::handle_fpu_trap, callable from assembly code.
    @relates Context
 */
// The "FPU not available" trap entry point
extern "C"
void
thread_handle_fputrap()
{
  LOG_TRAP_N(7);

  nonull_static_cast<Thread*>(current())->handle_fpu_trap();
}

PUBLIC static inline
void
Thread::set_int3_handler(int (*handler)(Trap_state *ts))
{
  int3_handler = handler;
}

/**
 * Default handle for int3 extensions if JDB is disabled. If the JDB is
 * available, Jdb::handle_int3_threadctx is called instead.
 * @return 0 not handled, wait for user response
 *         1 successfully handled
 */
PRIVATE static
int
Thread::handle_int3 (Trap_state *ts)
{
  Space *s       = current_thread()->space();
  int from_user  = ts->cs & 3;
  Address   ip   = ts->ip();
  Unsigned8 todo = s->peek((Unsigned8*)ip, from_user);
  Unsigned8 *str;
  int len;
  char c;

  switch (todo)
    {
    case 0xeb: // jmp == enter_kdebug()
      len = s->peek((Unsigned8*)(ip+1), from_user);
      str = (Unsigned8*)(ip + 2);

      putstr("KDB: ");
      if (len > 0)
	{
	  for (; len; len--)
      	    putchar(s->peek(str++, from_user));
	}
      putchar('\n');
      return 0; // => Jdb

    case 0x90: // nop == l4kd_display()
      if (          s->peek((Unsigned8*)(ip+1), from_user)  != 0xeb /*jmp*/
	  || (len = s->peek((Unsigned8*)(ip+2), from_user)) <= 0)
	return 0; // => Jdb

      str = (Unsigned8*)(ip + 3);
      for (; len; len--)
	putchar(s->peek(str++, from_user));
      break;

    case 0x3c: // cmpb
      todo = s->peek((Unsigned8*)(ip+1), from_user);
      switch (todo)
	{
	case  0: // l4kd_outchar
	  putchar(ts->eax & 0xff);
	  break;
        case  1: // l4kd_outnstring
	  str = (Unsigned8*)ts->eax;
    	  len = ts->ebx;
	  for(; len > 0; len--)
	    putchar(s->peek(str++, from_user));
	  break;
	case  2: // l4kd_outstr
	  str = (Unsigned8*)ts->eax;
	  for (; (c=s->peek(str++, from_user)); )
      	    putchar(c);
	  break;
	case  5: // l4kd_outhex32
	  printf("%08lx", ts->eax);
	  break;
	case  6: // l4kd_outhex20
	  printf("%05lx", ts->eax & 0xfffff);
	  break;
	case  7: // l4kd_outhex16
	  printf("%04lx", ts->eax & 0xffff);
	  break;
	case  8: // l4kd_outhex12
	  printf("%03lx", ts->eax & 0xfff);
	  break;
	case  9: // l4kd_outhex8
	  printf("%02lx", ts->eax & 0xff);
	  break;
	case 11: // l4kd_outdec
	  printf("%ld", ts->eax);
	  break;
	case 31: // Watchdog
	  switch (ts->ecx)
	    {
	    case 1:
	      // enable watchdog
	      Watchdog::user_enable();
	      break;
	    case 2:
	      // disable watchdog
	      Watchdog::user_disable();
	      break;
	    case 3:
	      // user takes over the control of watchdog and is from now on
	      // responsible for calling "I'm still alive" events (function 5)
	      Watchdog::user_takeover_control();
	      break;
	    case 4:
	      // user returns control of watchdog to kernel
	      Watchdog::user_giveback_control();
    	      break;
	    case 5:
	      // I'm still alive
	      Watchdog::touch();
	      break;
	    }
	  break;

	default: // ko
	  if (todo < ' ')
	    return 0; // => Jdb

	  putchar(todo);
	  break;
	}
      break;

    default:
      return 0; // => Jdb
    }

  return 1;
}

//---------------------------------------------------------------------------
IMPLEMENTATION [ia32-!{debug,kdb}]:

/** There is no nested trap handler if both jdb and kdb are disabled.
 * Important: We don't need the nested_handler_stack here.
 */
PRIVATE static inline
int
Thread::call_nested_trap_handler(Trap_state *)
{
  return -1;
}

//---------------------------------------------------------------------------
IMPLEMENTATION [ia32-{debug,kdb}]:

extern unsigned gdb_trap_recover; // in gdb_trap.c

/** Call the nested trap handler (either Jdb::enter_kdebugger() or the
 * gdb stub. Setup our own stack frame */
PRIVATE static
int
Thread::call_nested_trap_handler(Trap_state *ts)
{
  Proc::cli();

  int ret;
  static char nested_handler_stack[Config::PAGE_SIZE];
  unsigned dummy1, dummy2;

  // don't set %esp if gdb fault recovery to ensure that exceptions inside
  // kdb/jdb don't overwrite the stack
  asm volatile
    ("movl   %%esp,%%edx	\n\t"
     "cmpl   $0,(%%esi)		\n\t"
     "jne    1f			\n\t"
     "movl   %%ebx,%%esp	\n\t"
     "1:			\n\t"
     "incl   (%%esi)		\n\t"
     "pushl  %%edx		\n\t"
     "call   *%5		\n\t"
     "popl   %%esp		\n\t"
     "cmpl   $0,(%%esi)		\n\t"
     "je     1f			\n\t"
     "decl   (%%esi)		\n\t"
     "1:			\n\t"
     : "=a"(ret), "=c"(dummy1), "=d"(dummy2)
     : "a"(ts), "b"(nested_handler_stack + sizeof(nested_handler_stack)),
       "m"(nested_trap_handler), "S"(&gdb_trap_recover)
     : "memory");

  return ret == 0 ? 0 : -1;
}

//---------------------------------------------------------------------------
IMPLEMENTATION [ia32]:

PRIVATE inline
int
Thread::check_trap13_kernel (Trap_state *ts, bool from_user)
{
  if (EXPECT_FALSE (ts->trapno == 13 && (ts->err & 3) == 0))
    {
      // First check if user loaded a segment register with 0 because the
      // resulting exception #13 can be raised from user _and_ kernel. If
      // the user tried to load another segment selector, the thread gets
      // killed.
      // XXX Should we emulate this too? Michael Hohmuth: Yes, we should.
      if (EXPECT_FALSE (!(ts->ds & 0xffff)))
	{
	  Cpu::set_ds (Gdt::data_segment (from_user));
	  return 0;
	}
      if (EXPECT_FALSE (!(ts->es & 0xffff)))
	{
	  Cpu::set_es (Gdt::data_segment (from_user));
	  return 0;
	}
      if (EXPECT_FALSE (!(ts->fs & 0xffff)))
	{
	  ts->fs = Gdt::gdt_data_user | Gdt::Selector_user;
	  return 0;
	}
      if (EXPECT_FALSE (!(ts->gs & 0xffff)))
	{
	  ts->gs = Utcb_init::gs_value();
	  return 0;
	}
      if (EXPECT_FALSE (ts->ds & 0xfff8) == Gdt::gdt_code_user)
	{
	  WARN("%x.%x eip=%08lx: code selector ds=%04lx",
      	       d_taskno(), d_threadno(), ts->eip, ts->ds & 0xffff);
	  Cpu::set_ds (Gdt::data_segment (from_user));
	  return 0;
	}
      if (EXPECT_FALSE (ts->ds & 0xfff8) == Gdt::gdt_code_user)
	{
	  WARN("%x.%x eip=%08lx: code selector es=%04lx",
      	       d_taskno(), d_threadno(), ts->eip, ts->es & 0xffff);
	  Cpu::set_es (Gdt::data_segment (from_user));
	  return 0;
	}
      if (EXPECT_FALSE (ts->ds & 0xfff8) == Gdt::gdt_code_user)
	{
	  WARN("%x.%x eip=%08lx: code selector fs=%04lx",
      	       d_taskno(), d_threadno(), ts->eip, ts->fs & 0xffff);
	  ts->fs = Gdt::gdt_data_user | Gdt::Selector_user;
	  return 0;
	}
      if (EXPECT_FALSE (ts->ds & 0xfff8) == Gdt::gdt_code_user)
	{
	  WARN("%x.%x eip=%08lx: code selector gs=%04lx",
      	       d_taskno(), d_threadno(), ts->eip, ts->gs & 0xffff);
	  ts->gs = Utcb_init::gs_value();
	  return 0;
	}
    }

  return 1;
}

PRIVATE inline
void
Thread::check_f00f_bug (Trap_state *ts)
{
  // If we page fault on the IDT, it must be because of the F00F bug.
  // Figure out exception slot and raise the corresponding exception.
  // XXX: Should we also modify the error code?
  if (ts->trapno == 14		// page fault?
      && ts->cr2 >= Idt::idt()
      && ts->cr2 <  Idt::idt() + Idt::_idt_max * 8)
    ts->trapno = (ts->cr2 - Idt::idt()) / 8;
}

PRIVATE inline
bool
Thread::handle_io_page_fault (Trap_state *ts, Address eip, bool from_user)
{
  // check for page fault at the byte following the IO bitmap
  if (Config::enable_io_protection 
      && ts->trapno == 14           // page fault?
      && (ts->err & 4) == 0         // in supervisor mode?
      && (eip < Kmem::mem_user_max) // delimiter byte accessed?
      && (ts->cr2 == Mem_layout::Io_bitmap + L4_fpage::Io_port_max / 8))
    {
      // page fault in the first byte following the IO bitmap
      // map in the cpu_page read_only at the place
      Space::Status result =
	space()->v_insert (space()->virt_to_phys_s0 
	                     ((void*)Kmem::io_bitmap_delimiter_page()),
			   Mem_layout::Io_bitmap + L4_fpage::Io_port_max / 8,
			   Config::PAGE_SIZE,
			   Pd_entry::global());

      switch (result)
	{
	case Space::Insert_ok:
	  return true;
	case Space::Insert_err_nomem:
	  // kernel failure, translate this into a general protection
	  // violation and hope that somebody handles it
	  ts->trapno = 13;
	  ts->err    =  0;
	  return false;
	default:
	  // no other error code possible
	  assert (false);
	}
    }

  // Check for IO page faults. If we got exception #14, the IO bitmap page is
  // not available. If we got exception #13, the IO bitmap is available but
  // the according bit is set. In both cases we have to dispatch the code at
  // the faulting eip to deterine the IO port and send an IO flexpage to our
  // pager. If it was a page fault, check the faulting address to prevent
  // touching userland.
  if (Config::enable_io_protection && eip < Kmem::mem_user_max &&
      (ts->trapno == 13 ||
       ts->trapno == 14 && Kmem::is_io_bitmap_page_fault (ts->cr2)))
    {
      unsigned port, size;
      if (get_ioport (eip, ts, &port, &size))
        {
	  Mword io_page = L4_fpage::io (port, size, 0).raw();

          // set User mode flag to get correct EIP in handle_page_fault_pager
          // pretend a write page fault
          static const unsigned io_error_code = PF_ERR_WRITE | PF_ERR_USERMODE;

	  CNT_IO_FAULT;

          if (EXPECT_FALSE (log_page_fault()))
	    page_fault_log (io_page, io_error_code, eip);

          if (Config::monitor_page_faults)
            {
              if (_last_pf_address    == io_page &&
		  _last_pf_error_code == io_error_code)
                {
                  if (!log_page_fault())
		    printf ("*IO[%x,%x,%lx]\n", port, size, eip);
                  else
                    putchar ('\n');

                  kdb_ke ("PF happened twice");
                }

              _last_pf_address    = io_page;
              _last_pf_error_code = io_error_code;

              // (See also corresponding code in
              //  Thread::handle_page_fault() and Thread::handle_slow_trap.)
            }

          // treat it as a page fault in the region above 0xf0000000,

	  // We could also reset the Thread_cancel at slowtraps entry but it
	  // could be harmful for debugging (see also comment at slowtraps:).
	  //
	  // This must be done while interrupts are off to prevent that an
	  // other thread sets the flag again.
          state_del (Thread_cancel);

          Ipc_err ipc_code = handle_page_fault_pager (io_page, io_error_code);

          if (!ipc_code.has_error())
            {
              // check for cli/sti
	      Unsigned8 instr = space()->peek ((Unsigned8*) eip, from_user);

              if (space()->is_privileged() &&
		  (instr == 0xfa /*cli*/ || instr == 0xfb /*sti*/))
                {
                  // lazily link in IOPL if necessary
                  if ((ts->eflags & EFLAGS_IOPL) != EFLAGS_IOPL_U)
                    ts->eflags |= EFLAGS_IOPL_U;
                }

              return true;
            }
          // fallthrough if unsuccessful (maybe to user installed handler)
        }
    }
  return false;
}

PRIVATE inline
bool
Thread::handle_sysenter_trap (Trap_state *ts, Address eip, bool from_user)
{
  if (EXPECT_FALSE
      ((ts->trapno == 6 || ts->trapno == 13)
       && (ts->err & 0xffff) == 0
       && (eip < Kmem::mem_user_max - 2)
       && (space()->peek ((Unsigned16*) eip, from_user)) == 0x340f))
    {
      // somebody tried to do sysenter on a machine without support for it
      WARN ("%x.%x (tcb=%p) killed:\n"
	    "\033[1;31mSYSENTER not supported on this machine\033[0m",
	    d_taskno(), d_threadno(), this);

      if (Cpu::have_sysenter())
	// GP exception if sysenter is not correctly set up..
      	WARN ("SYSENTER_CS_MSR: %llx", Cpu::rdmsr (SYSENTER_CS_MSR));
      else
	// We get UD exception on processors without SYSENTER/SYSEXIT.
      	WARN ("SYSENTER/EXIT not available.");

      return false;
    }

  return true;
}

PRIVATE inline
bool
Thread::trap_is_privileged (Trap_state *)
{ return space()->is_privileged(); }

PRIVATE inline
void
Thread::do_wrmsr_in_kernel (Trap_state *ts)
{
  // do "wrmsr (msr[ecx], edx:eax)" in kernel
  Cpu::wrmsr (ts->eax, ts->edx, ts->ecx);
}

PRIVATE inline
void
Thread::do_rdmsr_in_kernel (Trap_state *ts)
{
  // do "rdmsr (msr[ecx], edx:eax)" in kernel
  Unsigned64 msr = Cpu::rdmsr (ts->ecx);
  ts->eax = (Unsigned32) msr;
  ts->edx = (Unsigned32) (msr >> 32);
}

PRIVATE inline
int
Thread::handle_not_nested_trap (Trap_state *ts)
{
  // no kernel debugger present
  printf (" %x.%02x EIP=%08lx Trap=%02lx [Ret/Esc]\n", 
	  d_taskno(), d_threadno(), ts->eip, ts->trapno); 

  int r;
  // cannot use normal getchar because it may block with hlt and irq's
  // are off here
  while ((r=Kconsole::console()->getchar (false)) == -1)
    Proc::pause();

  if (r == '\033')
    terminate (1);

  return 0;
}


//---------------------------------------------------------------------------
IMPLEMENTATION [ia32-!smas]:

IMPLEMENT inline
bool
Thread::handle_smas_gp_fault()
{
  return false;
}

//---------------------------------------------------------------------------
IMPLEMENTATION [ia32-segments]:

#include <feature.h>
KIP_KERNEL_FEATURE("segments");

PRIVATE inline
bool
Thread::handle_lldt(Trap_state *ts)
{
  int size;
  Address desc_addr;
  unsigned entry_number;
  Task_num task;
  Space *s;
  Thread *t;
  Unsigned64 desc;

  if (EXPECT_TRUE 
      (ts->eip >= Kmem::mem_user_max - 4 || 
       (space()->peek_user((Mword*)ts->eip) & 0xffffff) != 0xd0000f))
    return 0;

  if (EXPECT_FALSE(ts->ebx == 0)) // size argument
    {
      ts->ebx  = Gdt::gdt_tls1 >> 3;
      ts->eip += 3;
      return 1;
    }

  if (0 == tls_setup_emu(ts, &desc_addr, &size, &entry_number, &t))
    {
      while (size >= Cpu::Ldt_entry_size)
	{
	  desc = space()->peek_user((Unsigned64 *)desc_addr);
	  if (X86desc(desc).unsafe())
	    {
	      WARN("set_tls: Bad descriptor.");
	      return 0;
	    }

	  t->_gdt_tls[entry_number] = desc;
	  size      -= Cpu::Ldt_entry_size;
	  desc_addr += Cpu::Ldt_entry_size;
	  entry_number++;
	}

      if (t == current_thread())
	switch_gdt_tls();
      return 1;
    }

  if (lldt_setup_emu(ts, &s, &desc_addr, &size, &entry_number, &task))
    return 0;

  // Allocate the memory if needed
  // LDT maximum size is one page, anything else causes too much headache
  if (!s->ldt_addr())
    s->ldt_addr(Mapped_allocator::allocator()->alloc(Config::PAGE_SHIFT));

  // size is hardcoded
  if (entry_number * Cpu::Ldt_entry_size + size > Config::PAGE_SIZE)
    {
      WARN("set_ldt: LDT size exceeds one page, not supported.");
      return 0;
    }

  s->ldt_size(size + Cpu::Ldt_entry_size * entry_number);

  Unsigned64 *ldtp = 
    reinterpret_cast<Unsigned64 *>(s->ldt_addr()) + entry_number;

  while (size >= Cpu::Ldt_entry_size)
    {
      desc = space()->peek_user((Unsigned64 *)desc_addr);
      if (X86desc(desc).unsafe())
	{
	  WARN("set_ldt: Bad descriptor.");
	  return 0;
	}

      *ldtp = desc;
      size      -= Cpu::Ldt_entry_size;
      desc_addr += Cpu::Ldt_entry_size;
      ldtp++;
    }

  if (id().task() == task)
    Cpu::enable_ldt (s->ldt_addr(), s->ldt_size());

  return 1;
}

//---------------------------------------------------------------------------
IMPLEMENTATION [ia32-!segments]:

PRIVATE inline
bool
Thread::handle_lldt(Trap_state *)
{
  return 0;
}
