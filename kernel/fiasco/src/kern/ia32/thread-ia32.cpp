INTERFACE:

class trap_state;

EXTENSION class Thread
{
private:
  // small address space stuff
  bool handle_smas_gp_fault(Mword error_code);

  static void memcpy_byte_ds_es(void *dst, void const *src, size_t n);
  static void memcpy_mword_ds_es(void *dst, void const *src, size_t n);
  static void handle_double_fault(void) asm ("thread_handle_double_fault");

public:
  static bool may_enter_jdb;
  template < typename T > T peek( T const *addr, int from_user );
};

IMPLEMENTATION[ia32]:

#include "cpu.h"
#include "cpu_lock.h"
#include "entry.h"
#include "fpu_alloc.h"
#include "fpu_state.h"
#include "globalconfig.h"
#include "idt.h"
#include "io.h"
#include "kernel_console.h"
#include "l4_types.h"
#include "logdefs.h"
#include "pic.h"
#include "processor.h"
#include "profile.h"
#include "regdefs.h"
#include "terminate.h"
#include "utcb_alloc.h"
#include "watchdog.h"
#include "vmem_alloc.h"
#include "reset.h"
#include "simpleio.h"
#include <flux/x86/seg.h>
#include <flux/x86/tss.h>

bool Thread::may_enter_jdb = false;


IMPLEMENT inline 
void Thread::memcpy_byte_ds_es(void *dst, void const *src, size_t n)
{
  unsigned dummy1, dummy2, dummy3;
  asm volatile ( " cld                                     \n"
		 " repz movsl %%ds:(%%esi), %%es:(%%edi)   \n"
		 " mov %%edx, %%ecx                        \n"
		 " repz movsb %%ds:(%%esi), %%es:(%%edi)   \n"
		 :
		 "=c"(dummy1), "=S"(dummy2), "=D"(dummy3)
		 :
		 "c"(n>>2), "d"(n & 3), "S"(src), "D"(dst)
		 : 
		 "memory");
}

IMPLEMENT inline
void Thread::memcpy_mword_ds_es(void *dst, void const *src, size_t n)
{
  unsigned dummy1, dummy2, dummy3;
  asm volatile ( " cld                                     \n"
		 " repz movsl %%ds:(%%esi), %%es:(%%edi)   \n"
		 :
		 "=c"(dummy1), "=S"(dummy2), "=D"(dummy3)
		 :
		 "c"(n), "S"(src), "D"(dst)
		 :
		 "memory");
}

IMPLEMENT inline
template< typename T >
T Thread::peek( T const *addr, int from_user )
{
  return from_user ? peek_user(addr) : *(T*)addr;
}

inline NEEDS ["logdefs.h","timer.h"]
void thread_timer_interrupt_arch(void)
{
  // we're entered with disabled interrupts; that's necessary so that
  // we can't be preempted until we've acknowledged the interrupt and
  // updated the clock
  if (Config::scheduling_using_pit)
    {
      LOG_TIMER_IRQ(0);
    }
  else
    {
      LOG_TIMER_IRQ(8);
    }

#if 0  // screen spinner for debugging purposes
  (*(unsigned char*)(Kmem::phys_to_virt(0xb8000 + 16)))++;
#endif
}

// extra version for timer interrupt which is used when the jdb is active
// to prevent busy waiting.
extern "C" 
void thread_timer_interrupt_stop(void)
{
  Timer::acknowledge();
}

/** The timer interrupt.  Activated on every clock tick. 
    Updates the kernel clock, runs timeouts and reschedules of the current
    thread has used up its timer quantum.
 */
extern "C" 
void thread_timer_interrupt_slow(void)
{
  // we're entered with disabled interrupts; that's necessary so that
  // we can't be preempted until we've acknowledged the interrupt and
  // updated the clock

  if (Config::esc_hack)
    {
      // <ESC> timer hack: check if ESC key hit at keyboard
      if (Io::in8(0x60) == 1)
        kdb_ke("ESC");
    }

  if (Config::serial_esc == Config::SERIAL_ESC_NOIRQ)
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

  thread_timer_interrupt();
}

/** Return to user.  This function is the default routine run if a newly  
    initialized context is being switch_to()'ed.
 */
PROTECTED static 
void Thread::user_invoke() 
{
//   while (! (current()->state() & Thread_running))
//     current()->schedule();
  assert (current()->state() & Thread_running);
  
  asm volatile
    ("  movl %%eax,%%esp \n"    // set stack pointer to regs structure
     "  movw %1,%%ax     \n"
     "  movw %%ax,%%es   \n"
     "  movw %%ax,%%ds   \n"
     "  xorl %%ecx,%%ecx \n"     // clean out user regs
     "  xorl %%edx,%%edx \n"
     "  xorl %%esi,%%esi \n"
     "  xorl %%edi,%%edi \n" 
     "  xorl %%ebx,%%ebx \n"
     "  xorl %%ebp,%%ebp \n"
     "  xorl %%eax,%%eax \n"
     "  iret             \n"
     :                          // no output
     : "a" (nonull_static_cast<Return_frame*>(current()->regs())), 
     "i" (Kmem::gdt_data_user | SEL_PL_U)
     );

  // never returns here
}

/*
 * Handle FPU trap for this context. Assumes disabled interrupts
 */
PUBLIC inline NEEDS["fpu_alloc.h","fpu_state.h"]
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
  state_change_dirty (~0, Thread_fpu_owner);
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

PRIVATE inline NOEXPORT int
Thread::is_privileged_for_debug(trap_state * /*ts*/)
{
#if 0
  return ((ts->eflags & EFLAGS_IOPL) == EFLAGS_IOPL_U);
#else
  return 1;
#endif
}

PRIVATE inline NOEXPORT int
Thread::int3_extension(trap_state *ts)
{
  // no bounds checking here -- we assume privileged tasks are
  // civilized citizens :-)
  int from_user  = ts->cs & 3;
  Address   eip  = ts->eip;
  Unsigned8 todo = peek((Unsigned8*)eip, from_user);
  Unsigned8 *str;
  int len;

  switch (todo)
    {
    case 0xeb: // jmp == enter_kdebug()
      len = peek((Unsigned8*)(eip+1), from_user);
      str = (Unsigned8*)(eip + 2);

#if defined(CONFIG_JDB)
      if ((len > 0) && peek(str, from_user) == '*')
	{
     	  if ((len > 1) && peek(str+1, from_user) == '#')
	    // special: enter_kdebug("*#...") => execute jdb command
 	    return 0;

	  // skip '*'
	  len--; str++;

	  // log message into trace buffer instead of enter_kdebug
      	  if (peek(str, from_user == '+'))
	    {
	      // special: enter_kdebug("*+...") => extended log msg
	      // skip '+'
	      len--; str++;
	      Tb_entry_ke_reg *tb =
		static_cast<Tb_entry_ke_reg*>(Jdb_tbuf::new_entry());
	      tb->set(this, eip, 0, 0, ts);
	      for (int i=0; i<len; i++)
      		tb->set_buf(i, peek(str++, from_user));
	    }
	  else
	    {
	      // special: enter_kdebug("*...") => log entry
	      // fill in entry
	      Tb_entry_ke *tb =
		static_cast<Tb_entry_ke*>(Jdb_tbuf::new_entry());
	      tb->set(this, eip, 0, 0);
	      for (int i=0; i<len; i++)
		tb->set_buf(i, peek(str++, from_user));
	    }
	  Jdb_tbuf::commit_entry();
	  break;
	}
#endif

      putstr("KDB: ");
      if (len > 0)
	{
	  for (int i=0; i<len; i++)
      	    putchar(peek(str++, from_user));
	}
      putchar('\n');
      return 0;

    case 0x90: // nop == kd_display()
      if (           (peek((Unsigned8*)(eip+1), from_user)   != 0xeb /*jmp*/)
	  || ((len = (peek((Unsigned8*)(eip+2), from_user))) <= 0))
	return 0;

      str = (Unsigned8*)(eip + 3);
      for (int i=0; i<len; i++)
	putchar(peek(str++, from_user));
      break;

    case 0x3c: // cmpb
      todo = peek((Unsigned8*)(eip+1), from_user);
      switch (todo)
	{
	case  0: putchar(ts->eax & 0xff); break; // outchar
        case  1: 
	  {
            if(!from_user)
              break;
    	    str = (Unsigned8*)ts->eax;
            len = ts->ebx;
	    for(; len > 0; len--)
	      putchar(peek(str++, from_user));
	  }
	case  2:
	  {
    	    str = (Unsigned8*)ts->eax;
	    char c;
	    for(; (c=peek(str++, from_user)); )
	      putchar(c);
	  }
	break; // outstr
	case  5: printf("%08x", ts->eax);           break; // outhex32
	case  6: printf("%05x", ts->eax & 0xfffff); break; // outhex20
	case  7: printf("%04x", ts->eax & 0xffff);  break; // outhex16
	case  8: printf("%03x", ts->eax & 0xfff);   break; // outhex12
	case  9: printf("%02x", ts->eax & 0xff);    break; // outhex8
	case 11: printf("%d",   ts->eax);           break; // outdec

#ifdef CONFIG_PROFILE
	case 24:            // start kernel profiling
   	  if (Config::profiling)
 	    {
	      Proc::Status flags = Proc::cli_save();
	      profile::start();
	      Proc::sti_restore(flags);
	    }
	  break;

	case 25:            // stop kernel profiling, dump data to serial
	  if (Config::profiling)
	    {
	      Proc::Status flags = Proc::cli_save();
	      profile::stop_and_dump();
	      Proc::sti_restore(flags);
	    }
	  break;

	case 26:            // stop kernel profiling; do not dump
	  if (Config::profiling)
	    {
	      Proc::Status flags = Proc::cli_save();
	      profile::stop();
	      Proc::sti_restore(flags);
	    }
	  break;
#endif

	case 29:            // get kernel tracebuffer info
	  switch (ts->eax)
	    {
	    case 0:
	      ts->eax = Kmem::tbuf_status_page;
	      break;
#if defined(CONFIG_JDB)
	    case 1: // fiasco_tbuf_log
		{
		  // interrupts are disabled in handle_slow_trap()
		  Tb_entry_ke *tb =
		    static_cast<Tb_entry_ke*>(Jdb_tbuf::new_entry());
		  str = (Unsigned8*)ts->edx; // XXX pf!!
		  char c;
		  int i;

		  for (i=0; (c = peek(str++, from_user)); i++)
		    tb->set_buf(i, c);

		  tb->set_buf(i, 0);
		  tb->set(this, eip);
		  ts->eax = (Mword)tb;

		  Jdb_tbuf::commit_entry();
		}
	      break;
	    case 4: // fiasco_tbuf_log_3val()
		{
		  // interrupts are disabled in handle_slow_trap()
		  Tb_entry_ke_reg *tb =
		    static_cast<Tb_entry_ke_reg*>(Jdb_tbuf::new_entry());
		  str = (Unsigned8*)ts->edx; // XXX pf!!
		  char c;
		  int i;

		  for (i=0; (c = peek(str++, from_user)); i++)
		    tb->set_buf(i, c);

		  tb->set_buf(i, 0);
		  tb->set(this, eip, ts->ecx, ts->esi, ts->edi);
		  ts->eax = (Mword)tb;

		  Jdb_tbuf::commit_entry();
		}
	      break;
#endif
	    case 2:
	    case 3:
	      // handled by jdb
	      return 0;
	    }
	  break;

	case 31:
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
	    return 0;

	  putchar(todo);
	  break;
	}
      break;

    default: // nothing to emulate -- user breakpoint
      return 0;
    }

  return 1;
}

#ifdef CONFIG_KDB
extern unsigned gdb_trap_recover; // in gdb_trap.c
#else
unsigned gdb_trap_recover;
#endif

/** The global trap handler switch.
    This function handles CPU-exception reflection, emulation of CPU 
    instructions (LIDT, WRMSR, RDMSR), int3 debug messages, 
    kernel-debugger invocation, and thread crashes (if a trap cannot be 
    handled).
    @param state trap state
    @return 0 if trap has been consumed by handler;
           -1 if trap could not be handled.
 */    
PUBLIC
int
Thread::handle_slow_trap(trap_state *ts)
{ 
  Address eip;
  int from_user = ts->cs & 3;

  if (EXPECT_FALSE (gdb_trap_recover) )
    goto generic_debug;		// we're in the GDB stub or in jdb 
                                // -- let generic handler handle it

  LOG_TRAP;

  // First check if user loaded a segment register with 0 because the
  // resulting exception #13 can be raised from user _and_ kernel. If
  // the user tried to load another segment selector, the thread gets
  // killed. 
  // XXX Perhaps we should emulate this too. 
  //     Michael said: Yes, we should.
  if (EXPECT_FALSE ((ts->ds & 0xffff) == 0 || (ts->es & 0xffff) == 0 ||
		    (ts->fs & 0xffff) == 0 || (ts->gs & 0xffff) == 0) )
    {
      printf("KERNEL: %x.%x: "
	     "NULL segment selector ds=%04x, es=%04x, fs=%04x, gs=%04x\n",
	     debug_taskno(), debug_threadno(),
	     ts->ds & 0xffff, ts->es & 0xffff,
	     ts->fs & 0xffff, ts->gs & 0xffff);
      set_ds(Kmem::data_segment(from_user));
      set_es(Kmem::data_segment(from_user));
      set_fs(Kmem::gdt_data_user | SEL_PL_U);
      set_gs(Utcb_alloc::gs_value());
      return 0;
    }

  if (EXPECT_FALSE ((ts->ds & 0xfff8) == Kmem::gdt_code_user) )
    {
      printf("KERNEL: %x.%x: "
	     "code segment selector ds=%04x, es=%04x, fs=%04x, gs=%04x\n",
	     debug_taskno(), debug_threadno(),
	     ts->ds & 0xffff, ts->es & 0xffff,
	     ts->fs & 0xffff, ts->gs & 0xffff);
      set_ds(Kmem::data_segment(from_user));
      set_es(Kmem::data_segment(from_user));
		set_fs(Kmem::gdt_data_user | SEL_PL_U);
      set_gs(Utcb_alloc::gs_value());
      return 0;
    } 

  if (EXPECT_FALSE (! ((ts->cs & 3) || (ts->eflags & EFLAGS_VM))) ) {
      // small space faults can be raised in kernel mode, too
      // (long IPC)
      if (ts->trapno == 13 && handle_smas_gp_fault(ts->err))
	goto success;
      goto generic_debug;        // we were in kernel mode -- nothing to emulate
   }

  if (EXPECT_FALSE (ts->trapno == 2) )
    goto generic_debug;        // NMI always enters kernel debugger

  if (EXPECT_FALSE (ts->trapno == 0xffffffff))
    goto generic_debug;        // debugger interrupt

  // If we page fault on the IDT, it must be because of the F00F bug.
  // Figure out exception slot and raise the corresponding exception.
  // XXX: Should we also modify the error code?
  if (ts->trapno == 14 && ts->cr2 >= Idt::idt && 
                          ts->cr2 <  Idt::idt + idt_max * 8)
    ts->trapno = (ts->cr2 - Idt::idt) / 8;


  // so we were in user mode -- look for something to emulate
  
  // We continue running with interrupts off -- no sti() here. But
  // interrupts may be enabled by the pagefault handler if we get a
  // pagefault in peek_user().

  // Set up exception handling.  If we suffer an un-handled user-space
  // page fault, kill the thread.
  jmp_buf pf_recovery; 
  unsigned error;
  if (EXPECT_FALSE ((error = setjmp(pf_recovery)) != 0) )
    {
      printf("KERNEL: %x.%x (tcb=%08x) killed:\n"
	     "\033[1mUnhandled page fault, code=%08x\033[m\n",
	     debug_taskno(), debug_threadno(),
	     (unsigned)this, error);
      goto fail_nomsg;
    }

  _recover_jmpbuf = &pf_recovery;
  eip = ts->eip;

  // check for page fault at the byte following the IO bitmap
  if (Config::enable_io_protection 
      && (ts->trapno == 14)         // page fault?
      && ((ts->err & 4) == 0)       // in kernel? ie. a user mode IO instr.?
      && (eip < Kmem::mem_user_max) // delimiter byte accessed ?
      && (ts->cr2 == Kmem::io_bitmap + L4_fpage::IO_PORT_MAX / 8))
    {
      // page fault in the first byte following the IO bitmap
      // map in the cpu_page read_only at the place
      Space::Status result =
        space()->v_insert(space()->virt_to_phys(
                                           Kmem::io_bitmap_delimiter_page()),
			  Kmem::io_bitmap + L4_fpage::IO_PORT_MAX / 8,
			  Config::PAGE_SIZE, Kmem::pde_global());

     switch(result)
       {
       case Space::Insert_ok:
	 goto success;
       case Space::Insert_err_nomem:
	 // kernel failure, translate this into a general protection
	 // violation and hope that somebody handles it
	 ts->trapno=13;
	 ts->err=0;
	 break;
       default:
	 // no other error code possible
	 assert(false);
       }
    }

  // check for IO page faults
  if (Config::enable_io_protection
      && (ts->trapno == 13 || ts->trapno == 14)
      && eip < Kmem::mem_user_max)
    {
      unsigned port, size;
      if (get_ioport(eip, ts, &port, &size))
        {
	  Mword io_page = L4_fpage::io(port, size, 0).raw();

          // set User mode flag to get correct EIP in handle_page_fault_pager
          // pretend a write page fault
          unsigned io_error_code = PF_ERR_WRITE | PF_ERR_USERMODE;

          // TODO: put this into a debug_page_fault_handler
          if (EXPECT_FALSE( log_page_fault()) )
	    page_fault_log (io_page, io_error_code, eip);

          if (Config::monitor_page_faults &&
              // do monitoring only if we come in via general protection
              // otherwise Thread::handle_page_fault does it
              ts->trapno == 13)
            {
              if (   _last_pf_address    == io_page
                  && _last_pf_error_code == io_error_code)
                {
                  //#warning LOGGING
                  if (!log_page_fault())
		    printf("*IO[%x,%x,%x]\n", port,size,eip);
                  else
                    putchar('\n');

                  kdb_ke("PF happened twice");
                }

              _last_pf_address    = io_page;
              _last_pf_error_code = io_error_code;

              // (See also corresponding code in
              //  Thread::handle_page_fault() and
              //  Thread::handle_slow_trap.)
            }

          // treat it as a page fault in the region above 0xf0000000,

	  // We could also reset the Thread_cancel at slowtraps entry but it
	  // could be harmful for debugging (see also comment at slowtraps:).
	  //
	  // This must be done while interrupts are off to prevent that an
	  // other thread sets the flag again.
          state_del (Thread_cancel);

          L4_msgdope ipc_code = handle_page_fault_pager(io_page, 
						        io_error_code);

          if (!ipc_code.has_error())
            {
              // check for sti/cli
              if (((   peek((Unsigned8*)eip, from_user) == 0xfa)   // cli
                   || (peek((Unsigned8*)eip, from_user) == 0xfb))  // sti
                  && space()->is_privileged()
                  )
                {
                  // lazily link in IOPL if necessary
                  if ((ts->eflags & EFLAGS_IOPL) != EFLAGS_IOPL_U)
                    ts->eflags |= EFLAGS_IOPL_U;
                }

              goto success;
            }
          // fallthrough if unsuccessful (maybe to user installed handler)
        }
    }

  if (EXPECT_FALSE ((Config::ABI_V4 || Config::X2_LIKE_SYS_CALLS)
		    && ts->trapno == 6) )
    {
      if (peek((Unsigned16*)ts->eip, from_user) == 0x90f0) // lock; nop
        {
	  ts->eip += 2;		// step behind the lock; nop; syscall
	  ts->eax = space()->kip_address();
          goto success;
        }
    }

  
  if (EXPECT_FALSE (   ((ts->err & 0xffff) == 0)
			&& (eip < Kmem::mem_user_max - 2)
			&& ((peek((Unsigned16*)eip, from_user)) == 0x340f)
		   && (ts->trapno == 6 || ts->trapno == 13)))
	{
	  // somebody tried to do sysenter on a machine without support for it
	  printf("KERNEL: %x.%x (tcb=%p) killed:\n"
	         "\033[1;31mSYSENTER not supported on this machine\033[0m\n",
		 debug_taskno(), debug_threadno(),
		 this);
	  if (Cpu::features() & FEAT_SEP)
       {
         // GP exception if sysenter is not correctly set up..
         printf("SYSENTER_CS_MSR: %llx\n",Cpu::rdmsr (SYSENTER_CS_MSR));
       } else
       {
         // We get UD exception on processors without SYSENTER/SYSEXIT.
         printf("SYSENTER/EXIT not available.\n");
       }
	  goto fail_nomsg;
	}


  // check for general protection exception
  if (ts->trapno == 13)
    {
      // find out if we are a privileged task
      bool is_privileged = space()->is_privileged();

      // check for "lidt (%eax)"
      if (EXPECT_FALSE ((ts->err & 0xffff) == 0
			 && eip < Kmem::mem_user_max - 4
			 && (peek((Mword*)eip, from_user) & 0xffffff) 
			 == 0x18010f) )
	{
          // emulate "lidt (%eax)"

          // read descriptor
          if (ts->eax >= Kmem::mem_user_max - 6)
            goto fail;

          x86_gate *idt = peek((x86_gate**)(ts->eax + 2), from_user);
  	  vm_size_t limit = Config::backward_compatibility
			            ? 255
				    : peek((Unsigned16*)ts->eax, from_user);

          if (limit >= Kmem::mem_user_max
              || reinterpret_cast<Address>(idt) >= Kmem::mem_user_max-limit-1)
            goto fail;

          // OK; store descriptor
          _idt = idt;
          _idt_limit = (limit + 1) / sizeof(x86_gate);

          // consume instruction and continue
          smp_cas(&ts->eip, eip, eip + 3); // ignore errors
          goto success;
        }

      // check for "wrmsr (%eax)"
      if (EXPECT_FALSE (is_privileged
			&& ((ts->err & 0xffff) == 0)
			&& (eip < Kmem::mem_user_max - 2)
			&& ((peek((Unsigned16*)eip, from_user)) == 0x300f)
			&& (Cpu::features() & FEAT_MSR)) )
        {
          // do "wrmsr (msr[ecx], edx:eax)" in kernel
	  Cpu::wrmsr(ts->eax, ts->edx, ts->ecx);

          // consume instruction and continue
          smp_cas(&ts->eip, eip, eip + 2); // ignore errors
          goto success;
        }

      // check for "rdmsr (%eax)"
      if (EXPECT_FALSE (is_privileged
			&& ((ts->err & 0xffff) == 0)
			&& (eip < Kmem::mem_user_max - 2)
			&& ((peek((Unsigned16*)eip, from_user)) == 0x320f)
			&& (Cpu::features() & FEAT_MSR)) )
        {
          // do "rdmsr (msr[ecx], edx:eax)" in kernel
	  Unsigned64 msr = Cpu::rdmsr(ts->ecx);
	  ts->eax = (Unsigned32)msr;
	  ts->edx = (Unsigned32)(msr >> 32);

          // consume instruction and continue
          smp_cas(&ts->eip, eip, eip + 2); // ignore errors
          goto success;
        }

      // check for "hlt" -> deliver L4 version
      if (EXPECT_FALSE (   ((ts->err & 0xffff) == 0)
			&& (eip < Kmem::mem_user_max - 1)
			&& ((peek((Unsigned8*)eip, from_user)) == 0xf4)) )
	{
          // FIXME: What version should Fiasco deliver?
	  ts->eax = 0x00010000;

	  // consume instruction and continue
          smp_cas(&ts->eip, eip, eip + 1); // ignore errors
          goto success;
	}

      if (handle_smas_gp_fault(ts->err))
	goto success;
    }

  // let's see if we have a trampoline to invoke
  if (   ts->trapno < 0x20
      && ts->trapno < _idt_limit)
    {
      // translate page fault exceptions to general protection
      // exception -- users shouldn't have to care about about page
      // faults (and probably couldn't find out the fault address
      // anyway as they can't access the %cr2 (page fault address)
      // register)
      if (ts->trapno == 14)
        {
          ts->trapno = 13;
          ts->err = 0;
        }

      x86_gate g = peek(_idt + ts->trapno, from_user);

      if (Config::backward_compatibility // backward compat.: don't check
          || ((g.word_count & 0xe0) == 0
              && (g.access & 0x1f) == 0x0f)) // gate descriptor ok?
        {
          Address o = (g.offset_high << 16) | g.offset_low;

          if ((o != 0 || !Config::backward_compatibility) // bw compat.: != 0?
              && o < Kmem::mem_user_max // in user space?
              && ts->esp <= Kmem::mem_user_max
              && ts->esp > 4 * 4) // enough space on user stack?
            {
              // OK, reflect the trap to user mode
              Unsigned32 esp = (ts->esp);

              if (! raise_exception(ts, o))
                {
                  // someone interfered and changed our state
                  check(state_del(Thread_cancel));

                  goto success;
                }

	      esp -= sizeof(Mword);
              poke_user( (Mword*)esp, ts->eflags);
	      esp -= sizeof(Mword);
              poke_user( (Mword*)esp, (Mword)(Kmem::gdt_code_user | SEL_PL_U));
	      esp -= sizeof(Mword);
              poke_user( (Mword*)esp, eip);
              /* reset single trap flag to mirror cpu correctly */
              if (ts->trapno == 1)
                ts->eflags &= ~EFLAGS_TF;

              if ((ts->trapno >= 0x0a
                   && ts->trapno <= 0x0e)
                  || ts->trapno == 0x08
                  || ts->trapno == 0x11) // need to push error code?
                {
	          esp -= sizeof(Mword);
                  poke_user((Mword*)esp, ts->err);
                }

              goto success;     // we've consumed the trap
            }
        }
    }

  // backward compatibility cruft: check for those insane "int3" debug
  // messaging command sequences
  if ((ts->trapno == 3) && is_privileged_for_debug(ts))
    {
      if (int3_extension(ts))
	goto success;

      goto generic_debug;
    }

  // privileged tasks also may invoke the kernel debugger with a debug
  // exception
  if ((ts->trapno == 1) && is_privileged_for_debug(ts))
    goto generic_debug;


  // can't handle trap -- kill the thread

fail:
  printf("KERNEL: %x.%x (tcb=%08x) killed:\n"
         "\033[1mUnhandled trap\033[m\n",
	 debug_taskno(), debug_threadno(),
	 (unsigned)this);

fail_nomsg:
  trap_dump(ts);

  if (Config::conservative)
    kdb_ke("thread killed");

  // We could also reset the Thread_cancel at slowtraps entry but it could
  // be harmful for debugging (see also comment at slowtraps:).
  // 
  // This must occur while we are still cli'd
  state_del (Thread_cancel);

  if (state_change_safely(~Thread_running, Thread_cancel|Thread_dead))
    {
      // we haven't been re-initialized (cancel was not set) -- so sleep
      while (! (state() & Thread_running))
        schedule();
    }

success:
  _recover_jmpbuf = 0;
  return 0;

generic_debug:
  _recover_jmpbuf = 0;

  if (!nested_trap_handler)
    {
      // no kernel debugger present
      printf(" [Ret/Esc]\n"); // ask whether we should continue or panic
      int r;
      // cannot use normal getchar because it may block with hlt and irq's
      // are off here
      while((r=Kconsole::console()->getchar(false)) == -1)
        Proc::pause();

      if (r == '\033')
	{
	  // esc pressed
	  terminate(1);
          return 0;   // panic
	}
      else
  	return 0;
    }

  Proc::cli();
  //  kdb::com_port_init();             // re-initialize the GDB serial port

  // run the nested trap handler on a separate stack
  // equiv of: return nested_trap_handler(ts) == 0 ? true : false;

  int ret;
  static char nested_handler_stack[Config::PAGE_SIZE];
  unsigned dummy1, dummy2;

  // don't set %esp if gdb fault recovery to ensure that exceptions inside
  // kdb/jdb don't overwrite the stack
  asm volatile
    ("movl   %%esp,%%eax	\n\t"
     "orl    %%ebx,%%ebx	\n\t"
     "jz     1f			\n\t"
     "movl   %%ebx,%%esp	\n\t"
     "1:			\n\t"
     "pushl  %%eax		\n\t"
     "pushl  %%ecx		\n\t"
     "call   *%%edx		\n\t"
     "addl   $4,%%esp		\n\t"
     "popl   %%esp		\n\t"
     : "=a"(ret), "=c"(dummy1), "=d"(dummy2)
     :  "b"(gdb_trap_recover
		? 0 : (nested_handler_stack + sizeof(nested_handler_stack))),
        "c"(ts), "d"(nested_trap_handler)
     : "memory");

  return (ret == 0) ? 0 : -1;
}

IMPLEMENT
void
Thread::handle_double_fault(void)
{
  volatile x86_tss *tss  = Kmem::main_tss();
  int c;

  printf("\n\033[1;31mDOUBLE FAULT!\033[m\n"
         "EAX %08x EBX %08x ECX %08x EDX %08x\n"
         "ESI %08x EDI %08x EBP %08x ESP %08x\n"
	 "EIP %08x EFLAGS %08x\n"
	 "CS %04x SS %04x DS %04x ES %04x FS %04x GS %04x\n\n",
	 tss->eax, tss->ebx, tss->ecx, tss->edx,
	 tss->esi, tss->edi, tss->ebp, tss->esp,
	 tss->eip, tss->eflags,
	 tss->cs & 0xffff, tss->ss & 0xffff, tss->ds & 0xffff, 
	 tss->es & 0xffff, tss->fs & 0xffff, tss->gs & 0xffff);

  if (may_enter_jdb)
    {
      puts("Return reboots, \"k\" tries to enter the L4 kernel debugger...");

      while((c=Kconsole::console()->getchar(false)) == -1)
	Proc::pause();

      if (c == 'k' || c == 'K')
	{
	  Mword dummy1, dummy2, dummy3;
	  trap_state ts;

	  // built a nice trap state the jdb can work with
	  ts.eax    = tss->eax;
	  ts.ebx    = tss->ebx;
	  ts.ecx    = tss->ecx;
	  ts.edx    = tss->edx;
	  ts.esi    = tss->esi;
	  ts.edi    = tss->edi;
	  ts.ebp    = tss->ebp;
	  ts.esp    = tss->esp;
	  ts.cs     = tss->cs;
	  ts.ds     = tss->ds;
	  ts.es     = tss->es;
	  ts.ss     = tss->ss;
	  ts.fs     = tss->fs;
	  ts.gs     = tss->gs;
	  ts.trapno = 8;
	  ts.err    = 0;
	  ts.eip    = tss->eip;
	  ts.eflags = tss->eflags;

	  asm volatile
	    ("movl   %%esp,%%eax	\n\t"
	     "pushl  %%eax		\n\t"
	     "pushl  %%ecx		\n\t"
	     "call   *%%edx		\n\t"
	     "addl   $8,%%esp		\n\t"
	     : "=a"(dummy1), "=c"(dummy2), "=d"(dummy3)
	     : "a"(tss->esp), "c"(&ts), "d"(nested_trap_handler)
	     : "memory");
	}

      puts("\033[1mRebooting...\033[0m");
      pc_reset();
    }
  else
    {
      puts("Return reboots");
      while((Kconsole::console()->getchar(false)) == -1)
	Proc::pause();

      puts("\033[1mRebooting...\033[0m");
      pc_reset();
    }
}

PUBLIC
int
Thread::is_mapped()
{
  return Kmem::virt_to_phys((void*)this) != (Address)-1;
}

