INTERFACE:

class trap_state;

EXTENSION class Thread
{
private:
  // small address space stuff
  bool handle_smas_gp_fault(Mword error_code);

  static void memcpy_byte_ds_es(void *dst, void const *src, size_t n);
  static void memcpy_mword_ds_es(void *dst, void const *src, size_t n);
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
#include "rtc.h"
#include "watchdog.h"
#include "vmem_alloc.h"
#include "terminate.h"
#include <flux/x86/seg.h>

#include "irq.h" // for set_timer_vector...

// OSKIT crap
#include "undef_oskit.h"



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



/** Constructor.
    @param space the address space
    @param id user-visible thread ID of the sender
    @param init_prio initial priority 
    @param mcp thread's maximum controlled priority
    @post state() != Thread_invalid
 */
IMPLEMENT
Thread::Thread(Space* space,
	       L4_uid id,
	       int init_prio, unsigned short mcp)
  : Receiver (&_thread_lock, space), 
    Sender (id, 0)           // select optimized version of constructor
{
  // set a magic value -- we use it later to verify the stack hasn't
  // been overrun
  _magic = magic;

  Lock_guard <Thread_lock> guard (thread_lock());

  if (state() != Thread_invalid)
    return;                     // Someone else was faster in initializing!
                                // That's perfectly OK.

  *reinterpret_cast<void(**)()> (--kernel_sp) = user_invoke;

  _space = space;
  _irq = 0;
  _idt_limit = 0;
  _recover_jmpbuf = 0;
  _timeout = 0;

  sched()->set_prio (init_prio);
  sched()->set_mcp (mcp);
  sched()->set_timeslice (Config::default_time_slice);
  sched()->set_ticks_left (Config::default_time_slice);
  sched()->reset_cputime ();
  
  // clear out user regs that can be returned from the thread_ex_regs
  // system call to prevent covert channel
  Return_frame *r = regs();
  r->esp = 0;
  r->eip = 0;
  if(Config::enable_io_protection)
    r->eflags = EFLAGS_IOPL_K | EFLAGS_IF | 2;   // ei
  else
    r->eflags = EFLAGS_IOPL_U | EFLAGS_IF | 2;     // XXX iopl=kernel
  r->cs = Kmem::gdt_code_user | SEL_PL_U;
  r->ss = Kmem::gdt_data_user | SEL_PL_U;

 // make sure the thread's kernel stack is mapped in its address space
  _space->kmem_update(reinterpret_cast<vm_offset_t>(this));
  
  _pager = _preempter = _ext_preempter = nil_thread;

  if (space_index() == Thread::lookup(thread_lock()->lock_owner())
                       ->space_index())
    {
      // same task -> enqueue after creator
      present_enqueue(Thread::lookup(thread_lock()->lock_owner()));
    }
  else
    { 
      // other task -> enqueue in front of this task
      present_enqueue(lookup_first_thread(Thread::lookup
					  (thread_lock()->lock_owner())
					  ->space_index())
                      ->present_prev);
      // that's safe because thread 0 of a task is always present
    }

  state_add(Thread_dead);
  
  // ok, we're ready to go!
}

inline NEEDS ["logdefs.h","timer.h"]
void thread_timer_interrupt_arch(void)
{
  // we're entered with disabled interrupts; that's necessary so that
  // we can't be preempted until we've acknowledged the interrupt and
  // updated the clock
  if (Config::scheduling_using_pit)
      LOG_TIMER_IRQ(0);
  else
      LOG_TIMER_IRQ(8);
  
  Timer::acknowledge();

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
        jdb_enter_kdebug("ESC");
    }

  if (Config::serial_esc)
    {
      // Here we have to check for serial characters because the 
      // serial interrupt could have an lower priority than a not
      // acknowledged interrupt. The regular case is to stop when
      // receiving the serial interrupt.
      if(Kconsole::console()->char_avail()==1)
        jdb_enter_kdebug("SERIAL_ESC");
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

/** The global trap handler switch.
    This function handles CPU-exception reflection, emulation of CPU 
    instructions (LIDT, WRMSR, RDMSR), int3 debug messages, 
    kernel-debugger invocation, and thread crashes (if a trap cannot be 
    handled).
    In theory there is no need to have two versions of this function.
    In practice it is unfortunately impossible to change this monster
    function and have it doing the same afterwards. Therefore the
    small address space version is seperate but will be merged back as 
    soon as it is guaranteed to work flawlessly again.
    @param state trap state
    @return 0 if trap has been consumed by handler;
           -1 if trap could not be handled.
 */    
#ifdef CONFIG_APIC_MASK
extern unsigned apic_irq_nr;
#endif
PUBLIC
int Thread::handle_slow_trap(trap_state *ts)
{ 
  unsigned eip;

#ifdef CONFIG_KDB
  extern unsigned gdb_trap_recover; // in OSKit's gdb_trap.c
  if (gdb_trap_recover)
    goto generic; // we're in the GDB stub -- let generic handler handle it
#endif

  LOG_TRAP;

  if (! ((ts->cs & 3) || (ts->eflags & EFLAGS_VM)))
    goto generic;               // we were in kernel mode -- nothing to emulate

  if (ts->trapno == 2)          // NMI?
    goto generic;               // NMI always enters kernel debugger

  if (ts->trapno == 0xffffffff) // debugger interrupt
    goto generic;

  // If we page fault on the IDT, it must be because of the F00F bug.
  // Figure out exception slot and raise the corresponding exception.
  // XXX: Should we also modify the error code?
  if (ts->trapno == 14 && ts->cr2 >= Idt::idt && 
                          ts->cr2 <  Idt::idt + idt_max * 8)
    ts->trapno = (ts->cr2 - Idt::idt) / 8;

  // so we were in user mode -- look for something to emulate
  
  // We continue running with interrupts off -- no sti() here.

  // Set up exception handling.  If we suffer an un-handled user-space
  // page fault, kill the thread.
  jmp_buf pf_recovery; 
  unsigned error;
  if ((error = setjmp(pf_recovery)) != 0)
    {
      printf("KERNEL: %x.%x (tcb=0x%x) killed: unhandled page fault, "
             "code=0x%x\n",
             unsigned(space_index()), id().lthread(), (unsigned)this, error);
      goto fail_nomsg;
    }

  _recover_jmpbuf = &pf_recovery;

  eip = ts->eip;

  // check for page fault at the byte following the IO bitmap
  if(Config::enable_io_protection &&
     ts->trapno == 14 &&        // is page fault ?
     (ts->err & 4) == 0 &&      // in kernel mode ? ie. a user mode IO instr. ?
     eip < Kmem::mem_user_max && // EIP valid ?
                                // delimiter byte accessed ?
     ts->cr2 == Kmem::io_bitmap + L4_fpage::IO_PORT_MAX / 8
     )
    {
      // page fault in the first byte following the IO bitmap
      // map in the cpu_page read_only at the place

      Space::status_t result =
        space()->v_insert(
                        space()->virt_to_phys(
                                     Kmem::io_bitmap_delimiter_page()),
                        Kmem::io_bitmap + L4_fpage::IO_PORT_MAX / 8,
                        Config::PAGE_SIZE,
                        Kmem::pde_global()
                        );

     switch(result)
        {
        case Space::Insert_ok:
          goto success;
        case Space::Insert_err_nomem:
          // kernel failure, translate this into a
          // general protection violation and hope that somebody handles it
          ts->trapno=13;
          ts->err=0;
          break;                // fall through
        default:                // no other error code possible
          assert(false);
        }
    }

  // check for IO page faults
  if (Config::enable_io_protection
      && (ts->trapno == 13 || ts->trapno == 14)
      && eip < Kmem::mem_user_max)
    {
      unsigned port, size;
      if(get_ioport(eip, ts, &port, &size))
        {
          L4_fpage fp = L4_fpage::io(port, size, 0);
          // set User mode flag to get correct EIP in handle_page_fault_pager
          // pretend a write page fault
          unsigned io_error_code = PF_ERR_WRITE | PF_ERR_USERMODE;

          // TODO: put this into a debug_page_fault_handler
          //#warning NO LOGGING
#if 1
          if (EXPECT_FALSE( log_page_fault) )
            {
              page_fault_log (fp.page(), io_error_code, eip);
            }
#endif

          // if(ts->trapno == 14)
          //   printf("[IO page fault (trap 14 at %08x) ",
          //          ts->cr2);
          // else
          //   printf("[IO page fault (trap %d) ", ts->trapno);
          // printf("at %08x for port 0x%04x, size %d]\n",
          //        eip, port, size );

          if (Config::monitor_page_faults &&
              // do monitoring only if we come in via general protection
              // otherwise Thread::handle_page_fault does it
              ts->trapno == 13)
            {
              if (_last_pf_address == fp.page()
                  && _last_pf_error_code == io_error_code)
                {
                  //#warning LOGGING
#if 1
                  if (!log_page_fault)
                    {
#endif
                      printf("*IO[%x,%x,%x]\n", port,size,eip);
#if 1
                    }
                  else
                    putchar('\n');
#endif

                  kdb_ke("PF happened twice");
                }

              _last_pf_address = fp.page();
              _last_pf_error_code = io_error_code;

              // (See also corresponding code in
              //  Thread::handle_page_fault() and
              //  Thread::handle_slow_trap.)
            }

          // treat it as a page fault in the region above 0xf0000000,

          state_del (Thread_cancel);
                                // this must occur while we are still cli'd

          L4_msgdope ipc_code = handle_page_fault_pager(fp.page(), 
						        io_error_code);

          if (!ipc_code.has_error())
            {
              // check for sti/cli
              if (((   peek_user((Unsigned8*)eip) == 0xfa)   // cli
                   || (peek_user((Unsigned8*)eip) == 0xfb))  // sti
                  && space()->is_privileged()
                  )
                {
                  // lazily link in IOPL if necessary
                  if((ts->eflags & EFLAGS_IOPL) != EFLAGS_IOPL_U)
                    ts->eflags |= EFLAGS_IOPL_U;
                }

              goto success;
            }
          // fallthrough if unsuccessful (maybe to user installed handler)
        }
    }


  if (Config::X2_LIKE_SYS_CALLS && ts->trapno == 6)
    {
      if( peek_user((Unsigned16*)ts->eip) == 0x90f0 ) // lock; nop
        {
          ts->eip = ts->eip+2; // step behind the lock; nop; syscall
          if( space_index() == Config::sigma0_taskno )
            {
              ts->eax = Kmem::virt_to_phys(Kmem::info());
              goto success;
            }
          else
            {
              ts->eax = Config::AUTO_MAP_KIP_ADDRESS;
            }
          goto success;
        }
    }

  // check for general protection exception
  if (ts->trapno == 13)
    {
      // find out if we are a privileged task
      bool is_privileged = space()->is_privileged();

      // check for "lidt (%eax)"
      if ((ts->err & 0xffff) == 0
          && eip < Kmem::mem_user_max - 4
          && (peek_user((Mword*)eip) & 0xffffff) == 0x18010f
         )
        {
          // emulate "lidt (%eax)"

          // read descriptor
          if (ts->eax >= Kmem::mem_user_max - 6)
            goto fail;

          x86_gate *idt = peek_user((x86_gate**)(ts->eax + 2));
  	  vm_size_t limit = Config::backward_compatibility
			            ? 255
				    : peek_user((Unsigned16*)ts->eax);

          if (limit >= Kmem::mem_user_max
              || reinterpret_cast<vm_offset_t>(idt) >= Kmem::mem_user_max
                                                       - limit - 1)
            goto fail;

          // OK; store descriptor
          _idt = idt;
          _idt_limit = (limit + 1) / sizeof(x86_gate);

          // consume instruction and continue
          smp_cas(&ts->eip, eip, eip + 3); // ignore errors
          goto success;
        }

      // check for "wrmsr (%eax)"
      if (is_privileged                      // only allow priv tasks
          && (ts->err & 0xffff) == 0
          && eip < Kmem::mem_user_max - 2
          && (peek_user((Unsigned16*)eip) ) == 0x300f
          && (Cpu::features() & FEAT_MSR)       // cpu has MSRs
         )
        {
          // do "wrmsr (msr[ecx], edx:eax)" in kernel
	  Cpu::wrmsr(ts->eax, ts->edx, ts->ecx);

          // consume instruction and continue
          smp_cas(&ts->eip, eip, eip + 2); // ignore errors
          goto success;
        }

      // check for "rdmsr (%eax)"
      if (is_privileged                      // only allow priv tasks
          && (ts->err & 0xffff) == 0
          && eip < Kmem::mem_user_max - 2
          && (peek_user((Unsigned16*)eip) ) == 0x320f
          && (Cpu::features() & FEAT_MSR)       // cpu has MSRs
         )
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
      if (is_privileged                      // only allow priv tasks
          && (ts->err & 0xffff) == 0
          && eip < Kmem::mem_user_max - 1
          && (peek_user((Unsigned8*)eip) ) == 0xf4
         )
        {
          // FIXME: What version should Fiasco deliver?
          ts->eax = 0x00010000;

          // consume instruction and continue
          smp_cas(&ts->eip, eip, eip + 1); // ignore errors
          goto success;
        }

      if (handle_smas_gp_fault(ts->err))
      {
	goto success;
      }

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

      x86_gate g = peek_user(_idt + ts->trapno);

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
              unsigned32_t esp = (ts->esp);

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
  if (ts->trapno == 3
      // && (ts->eflags & EFLAGS_IOPL) == EFLAGS_IOPL_U) // only allow priv tasks
      )
    {
      // no bounds checking here -- we assume privileged tasks are
      // civilized citizens :-)
      unsigned char todo = peek_user((Unsigned8*)eip);

      char *str;
      int len;

      switch (todo)
        {
        case 0xeb:              // jmp == enter_kdebug()
          len = peek_user((Unsigned8*)(eip + 1));
          str = (char *)(eip + 2);

          if ((len > 0) && (peek_user(str) == '*'))
            {
              if ((len > 1) && (peek_user(str + 1) == '#'))
                goto generic;

              // skip '*'
              len--; str++;

#if defined(CONFIG_JDB)
                {
		  // log message into trace buffer instead of enter_kdebug
		  if (peek_user(str) == '+')
		    {
		      // extended log msg
		      // skip '+'
		      len--; str++;
		      Tb_entry_ke_reg *tb =
			static_cast<Tb_entry_ke_reg*>(Jdb_tbuf::new_entry());
	      	      char *buffer = (char*)alloca(len);
      		      copy_from_user<char>(buffer, str, len);
		      tb->set(this, eip, buffer, len, ts);
		    }
		  else
		    {
		      // fill in entry
		      Tb_entry_ke *tb =
			static_cast<Tb_entry_ke*>(Jdb_tbuf::new_entry());
	      	      char *buffer = (char*)alloca(len);
      		      copy_from_user<char>(buffer, str, len);		 
		      tb->set(this, eip, buffer, len);
		    }
		  Jdb_tbuf::commit_entry();
		}
#endif
              break;
            }

          putstr("KDB: ");

          if (len > 0) 
	    {
	      char *buffer = (char*)alloca(len);
	      copy_from_user<char>(buffer, str, len);
	      putnstr(buffer,len);
	    }

          putchar('\n');
          goto generic;

        case 0x90:              // nop == kd_display()
          if (peek_user((Unsigned8*)(eip + 1)) != 0xeb /*jmp*/)
            goto generic;

          len = peek_user((Unsigned8*)(eip + 2));
          if (len <= 0)
            goto generic;

	  {
	    char *buffer = (char*)alloca(len);
	    copy_from_user<char>(buffer, (char*)(eip + 3), len);
	    putnstr(buffer,len);
	  }
          break;

        case 0x3c:              // cmpb
          todo = peek_user((Unsigned8*)(eip + 1));
          switch (todo)
            {
            case  0: putchar(ts->eax & 0xff);           break; // outchar
            case  2: 
	      {
		char const *str = (char const*)ts->eax;
		unsigned char pos = 0;
		char buffer[64];
		while( (buffer[pos++] = peek_user(str++)) )
		  {
		    if(pos==sizeof(buffer))
		      {
			putnstr(buffer, pos);
			pos = 0;
		      }
		  }
		if(pos)
		  putstr(buffer);
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
		case 1:
		    {
		      // interrupts are disabled in handle_slow_trap()
	    	      Tb_entry_ke *tb =
    			static_cast<Tb_entry_ke*>(Jdb_tbuf::new_entry());
		      const char *str = (const char*) ts->edx; // XXX pf!!
		      char c;
		      int i;

		      for (i=0; (c = peek_user(str++)); i++)
			tb->set_buf(i, c);

		      tb->set_buf(i, 0);
		      tb->set(this, eip);
		      ts->eax = (Mword)tb;

		      Jdb_tbuf::commit_entry();
		    }
		  break;
		case 4:
		    {
		      // interrupts are disabled in handle_slow_trap()
	    	      Tb_entry_ke_reg *tb =
    			static_cast<Tb_entry_ke_reg*>(Jdb_tbuf::new_entry());
		      const char *str = (const char*) ts->edx; // XXX pf!!
		      char c;
		      int i;

		      for (i=0; (c = peek_user(str++)); i++)
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
		  goto generic;
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
                  // user takes over the control of watchdog and is from
                  // now on responsible for calling "I'm still alive"
                  // events (function 5)
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

            default:            // ko
              if (todo < ' ')
                goto generic;

              putchar(todo);
              break;
            }
          break;

        default:                // user breakpoint
          goto generic;
        }

      goto success;
    }

  // privileged tasks also may invoke the kernel debugger with a debug
  // exception
  if (ts->trapno == 1
      && (ts->eflags & EFLAGS_IOPL) == EFLAGS_IOPL_U) // only allow priv tasks
    goto generic;



  // can't handle trap -- kill the thread

fail:
  printf("KERNEL: %x.%x (tcb=0x%x) killed: unhandled trap\n",
         unsigned(space_index()), id().lthread(), (unsigned)this);
fail_nomsg:
  trap_dump(ts);

  if (Config::conservative)
    kdb_ke("thread killed");

  if (state_change_safely(~Thread_running, Thread_cancel|Thread_dead))
    {
      // we haven't been re-initialized (cancel was not set) -- so sleep
      while (! (state() & Thread_running))
        schedule();
    }

success:
  _recover_jmpbuf = 0;
  return 0;

generic:
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

      if( r == '\033')
        { // esc pressed?
          terminate(1);
          return 0;   // panic
        }
      else
        {
          return 0;
        }
    }

  Proc::cli();
  //  kdb::com_port_init();             // re-initialize the GDB serial port

  // run the nested trap handler on a separate stack
  // equiv of: return nested_trap_handler(ts) == 0 ? true : false;

  int ret;
  static char nested_handler_stack[Config::PAGE_SIZE];

  unsigned dummy1, dummy2;

  asm volatile
    (" movl %%esp,%%eax \n"
     " orl  %%ebx,%%ebx \n"     // don't set %esp if gdb fault recovery
     " jz   1f \n"
     " movl %%ebx,%%esp \n"
     "1: pushl %%eax \n"
     " pushl %%ecx \n"
     " call *%%edx \n"
     " addl $4,%%esp \n"
     " popl %%esp"
     : "=a" (ret), "=c" (dummy1), "=d" (dummy2)
#ifdef CONFIG_KDB
     : "b" (gdb_trap_recover // gdb fault recovery?
	       ? 0
	       : (nested_handler_stack + sizeof(nested_handler_stack))),
#else
     : "b" (nested_handler_stack + sizeof(nested_handler_stack)),
#endif
       "1" (ts),
       "2" (nested_trap_handler)
     : "memory");

  return (ret == 0) ? 0 : -1;
}

