INTERFACE:

#include <sys/types.h>

class trap_state;

IMPLEMENTATION[ux]:

#include <cstdio>
#include "boot_info.h"		// for boot_info
#include "config.h"		// for page sizes
#include "cpu.h"		// for cpu
#include "cpu_lock.h"
#include "emulation.h"
#include "fpu_alloc.h"
#include "kdb_ke.h"		// for kdb_ke
#include "lock_guard.h"
#include "panic.h"
#include "processor.h"		// for cli/sti
#include "regdefs.h"
#include "vmem_alloc.h"

// OSKIT crap
#include "undef_oskit.h"

IMPLEMENT
Thread::Thread (Space* space, L4_uid id,
		int init_prio, unsigned short mcp)
	: Receiver (&_thread_lock, space), 
          Sender (id, 0)           // select optimized version of constructor
{
  Lock_guard <Thread_lock> guard (thread_lock());

  if (state() != Thread_invalid)	// Someone else was faster in initializing!
    return;				// That's perfectly OK.

  _magic          = magic;
  _space          = space;
  _irq            = 0;
  _idt_limit      = 0;
  _recover_jmpbuf = 0;
  _timeout        = 0;

  *reinterpret_cast<void(**)()> (--kernel_sp) = user_invoke;

  sched()->set_prio (init_prio);
  sched()->set_mcp (mcp);
  sched()->set_timeslice (Config::default_time_slice);
  sched()->set_ticks_left (Config::default_time_slice);
  sched()->reset_cputime ();
  
  // Allocate FPU state now because it indirectly calls current()
  // save_state runs on a signal stack and current() doesn't work there.
  Fpu_alloc::alloc_state(fpu_state());
  
  // clear out user regs that can be returned from the thread_ex_regs
  // system call to prevent covert channel
  Return_frame *r = regs();
  r->esp = 0;
  r->eip = 0;
  r->cs  = Emulation::get_kernel_cs() & ~1;		// force iret trap
  r->ss  = Emulation::get_kernel_ss();

  if(Config::enable_io_protection)
    r->eflags = EFLAGS_IOPL_K | EFLAGS_IF | 2;
  else
    r->eflags = EFLAGS_IOPL_U | EFLAGS_IF | 2;     // XXX iopl=kernel

  _pager = _preempter = _ext_preempter = nil_thread;

  if (space_index() == Thread::lookup(thread_lock()->lock_owner())->space_index()) {

    // same task -> enqueue after creator
    present_enqueue (Thread::lookup(thread_lock()->lock_owner()));

  } else { 

    // other task -> enqueue in front of this task
    present_enqueue (lookup_first_thread(Thread::lookup(thread_lock()
                   ->lock_owner())->space_index())->present_prev);
    // that's safe because thread 0 of a task is always present
  }

  state_add(Thread_dead);
  
  // ok, we're ready to go!
}

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
Thread::handle_slow_trap (trap_state *ts)
{
  extern unsigned gdb_trap_recover; // in OSKit's gdb_trap.c
  unsigned eip;

  if (gdb_trap_recover)   
    goto generic; // we're in the GDB stub -- let generic handler handle it

  if (! ((ts->cs & 3) || (ts->eflags & EFLAGS_VM)))
    goto generic;               // we were in kernel mode -- nothing to emulate

  if (ts->trapno == 2)          // NMI?
    goto generic;               // NMI always enters kernel debugger

  if (ts->trapno == 0xffffffff) // debugger interrupt
    goto generic;         

  // so we were in user mode -- look for something to emulate

  // We continue running with interrupts off -- no sti() here.

  // Set up exception handling.  If we suffer an un-handled user-space
  // page fault, kill the thread.
  jmp_buf pf_recovery;    
  unsigned error;         
  if ((error = setjmp(pf_recovery)) != 0)
    {
      printf ("KERNEL: %x.%x (tcb=0x%x) killed: unhandled page fault, "
                   "code=0x%x\n",
                   unsigned (space_index()), id().lthread(),
                   (unsigned) this, error);
      goto fail_nomsg;    
    }

  _recover_jmpbuf = &pf_recovery;

  eip = ts->eip;   

  // check for "invalid opcode" exception
  if (Config::X2_LIKE_SYS_CALLS && ts->trapno == 6)
    {
      if (peek_user ((Unsigned16 *) ts->eip) == 0x90f0)		// "lock; nop" opcode
	{
	  ts->eip += 2;			// step behind the opcode

	  if (space_index() == Config::sigma0_taskno)
            ts->eax = Kmem::virt_to_phys (Kmem::info());
	  else
            ts->eax = Config::AUTO_MAP_KIP_ADDRESS;

	  goto success;
	}
    }

  // check for "general protection" exception
  if (ts->trapno == 13)   
    {
      // check for "lidt (%eax)"
      if ((ts->err & 0xffff) == 0 && eip < Kmem::mem_user_max - 4 &&
          (peek_user((Mword*)eip) & 0xffffff) == 0x18010f) {

        // read descriptor
        if (ts->eax >= Kmem::mem_user_max - 6)
          goto fail;    

        x86_gate *idt = peek_user ((x86_gate **)(ts->eax + 2));
        vm_size_t limit = Config::backward_compatibility ?
                          255 : peek_user ((Unsigned16 *) ts->eax);

        if (limit >= Kmem::mem_user_max ||
            reinterpret_cast<vm_offset_t>(idt) >= Kmem::mem_user_max -
                                                  limit - 1)
          goto fail;

        // OK; store descriptor
        _idt = idt;     
        _idt_limit = (limit + 1) / sizeof(x86_gate);

        ts->eip += 3;		          // consume instruction and continue
        goto success;
      }

      // check for "wrmsr (%eax)"
      if ((ts->eflags & EFLAGS_IOPL) == EFLAGS_IOPL_U // only allow priv tasks
          && (ts->err & 0xffff) == 0
          && eip < Kmem::mem_user_max - 2
          && (peek_user((Unsigned16*)eip) ) == 0x300f
          && (Cpu::features() & FEAT_MSR)       // cpu has MSRs
         )
        {
          // do "wrmsr (msr[ecx], edx:eax)" in kernel
          unsigned dummy1, dummy2, dummy3;

          asm volatile(".byte 0xf; .byte 0x30\n"        /* wrmsr */
                      : "=a" (dummy1), "=d" (dummy2), "=c" (dummy3)
                      : "0" (ts->eax), "1" (ts->edx), "2" (ts->ecx));

          // consume instruction and continue
          smp_cas(&ts->eip, eip, eip + 2); // ignore errors
          goto success;   
        }

      // check for "rdmsr (%eax)"
      if ((ts->eflags & EFLAGS_IOPL) == EFLAGS_IOPL_U // only allow priv tasks
          && (ts->err & 0xffff) == 0
          && eip < Kmem::mem_user_max - 2
          && (peek_user((Unsigned16*)eip) ) == 0x320f
          && (Cpu::features() & FEAT_MSR)       // cpu has MSRs
         )
        {
          // do "rdmsr (msr[ecx], edx:eax)" in kernel
          unsigned dummy; 

          asm volatile(".byte 0xf; .byte 0x32\n"        /* rdmsr */
                       : "=a" (ts->eax), "=d" (ts->edx), "=c" (dummy)
                       : "2" (ts->ecx));

          // consume instruction and continue
          smp_cas(&ts->eip, eip, eip + 2); // ignore errors
          goto success;   
        }

      // check for "hlt" -> deliver L4 version
      if ((ts->eflags & EFLAGS_IOPL) == EFLAGS_IOPL_U // only allow priv tasks
          && (ts->err & 0xffff) == 0
          && eip < Kmem::mem_user_max - 1
          && peek_user ((Unsigned8 *) eip) == 0xf
         )
        {
          // FIXME: What version should Fiasco deliver?
          ts->eax = 0x00010000;

          // consume instruction and continue
          smp_cas(&ts->eip, eip, eip + 1); // ignore errors
          goto success;   
        }
    }

  // let's see if we have a trampoline to invoke
  if (ts->trapno < 0x20 && ts->trapno < _idt_limit)
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

      x86_gate g;
      copy_from_user<Unsigned8>(&g, _idt + ts->trapno, sizeof (g));

      if (Config::backward_compatibility // backward compat.: don't check
          || ((g.word_count & 0xe0) == 0
              && (g.access & 0x1f) == 0x0f)) // gate descriptor ok?
        {
          vm_offset_t handler = (g.offset_high << 16) | g.offset_low;

          if ((handler != 0 || !Config::backward_compatibility) // bw compat.: != 0?
              && handler < Kmem::mem_user_max // in user space?
              && ts->esp <= Kmem::mem_user_max
              && ts->esp > 4 * 4) // enough space on user stack?
            {
              // OK, reflect the trap to user mode
              unsigned32_t esp = ts->esp;

              if (! raise_exception(ts, handler))
                {         
                  // someone interfered and changed our state
                  check(state_del(Thread_cancel));

                  goto success;
                }

              unsigned32_t user_stack[4] = { ts->err,
                                             eip,
                                             Emulation::get_kernel_cs(),
                                             ts->eflags };

              // esp - ts->esp tells us how many bytes to copy to the stack
              // This information comes from raise_exception above
              // off tells us where to start in user_stack array,
              // i.e. whether to skip the error code for some traps
              unsigned len = esp - ts->esp;
              unsigned off = (sizeof (user_stack) - len) / sizeof (*user_stack);
              copy_to_user<char>((void *) ts->esp, user_stack + off, len);

              /* reset single trap flag to mirror cpu correctly */
              if (ts->trapno == 1)
                ts->eflags &= ~EFLAGS_TF;

              goto success;		// we've consumed the trap
            }
        }
    }

  // backward compatibility cruft: check for those insane "int3" debug
  // messaging command sequences
  if (ts->trapno == 3     
      /*&& (ts->eflags & EFLAGS_IOPL) == EFLAGS_IOPL_U*/) // only allow priv tasks
    {
      // no bounds checking here -- we assume privileged tasks are
      // civilized citizens :-)
      unsigned char todo = peek_user ((Unsigned8 *) eip);
      bool enter_kdb = false;
      char *buffer;

      unsigned char len;

      switch (todo) {

        case 0xeb:              // jmp == enter_kdebug()

          printf ("\nKDEBUG (EIP:%08x): ", eip);
          
          len = peek_user ((Unsigned8 *)(eip + 1));

          if (len) {
            buffer = (char *) alloca (len);
            copy_from_user<char> (buffer, (char *)(eip + 2), len);
            printf ("%.*s", len, buffer);
          }

          kdb_ke (NULL);
          break;

        case 0x90:              // nop == kd_display()
          if (peek_user ((Unsigned8 *)(eip + 1)) != 0xeb || (len =
              peek_user ((Unsigned8 *)(eip + 2))) == 0)
            goto generic;

          buffer = (char *) alloca (len);
          copy_from_user<char>(buffer, (char *)(eip + 3), len);
          printf ("%.*s", len, buffer);

          break;          

        case 0x3c:       	       // cmpb
          todo = peek_user ((Unsigned8 *)(eip + 1));

          switch (todo) { 

            case 0:             // outchar
              putchar (ts->eax & 0xff);
              break;      

            case 2:             // outstring
              char c;
              while ((c = peek_user ((Unsigned8 *)(ts->eax++))) != 0)
                putchar (c);
              break;      

            case 5:             // outhex32
              printf ("%08x", ts->eax);
              break;      

            case 6:             // outhex20
              printf ("%05x", ts->eax & 0xfffff);
              break;      

            case 7:             // outhex16
              printf ("%04x", ts->eax & 0xffff);
              break;      

            case 8:             // outhex12
              printf ("%03x", ts->eax & 0xfff);
              break;      

            case 9:             // outhex8
              printf ("%02x", ts->eax & 0xff);
              break;      

            case 11:            // outdec
              printf ("%d", ts->eax);
              break;      

            case 24:            // start kernel profiling
            case 25:            // stop kernel profiling, dump data to serial
            case 26:            // stop kernel profiling; do not dump
              break;      

            case 30:            // register debug symbols/lines information
              panic ("jdb_symbol::register_symbols called");
              break;      

            case 31:      	// watchdog
              break;      

            default:            // ko
              if (todo < ' ')
                goto nostr;

              putchar (todo);
          }
          break;          

        default:                // user breakpoint
          goto nostr;     
      }

      if (enter_kdb) {
        panic ("Enter kdb");
        goto generic;       // panic
      }

      goto success;             // success -- consume int3

    nostr:
      puts ("KDB: int3");
      goto generic;             // enter the kernel debugger
    }

  // privileged tasks also may invoke the kernel debugger with a debug
  // exception
  if (ts->trapno == 1 &&
     (ts->eflags & EFLAGS_IOPL) == EFLAGS_IOPL_U) // only allow priv tasks
    goto generic;       

  // can't handle trap -- kill the thread

fail:
  printf ("KERNEL: %x.%x (tcb=0x%x) killed: unhandled trap\n",
               unsigned(space_index()), id().lthread(), (unsigned) this);
fail_nomsg:
  trap_dump(ts);          

  if (Config::conservative)
    kdb_ke("thread killed");

  // we haven't been re-initialized (cancel was not set) -- so sleep
  if (state_change_safely (~Thread_running, Thread_cancel | Thread_dead))
    while (! (state() & Thread_running))
      schedule();       

success:
  _recover_jmpbuf = 0;    
  return 0;

generic:
  _recover_jmpbuf = 0;    

  if (!nested_trap_handler)
    return -1;

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
     : "b" (gdb_trap_recover ?  // gdb fault recovery?
            0 : (nested_handler_stack + sizeof(nested_handler_stack))),
       "1" (ts),          
       "2" (nested_trap_handler)
     : "memory");         

  return (ret == 0) ? 0 : -1;
}

/**
 * Return to user.
 * This function is the default routine run if a newly initialized context
 * is being switch_to()'ed.
 */                   
PROTECTED static void Thread::user_invoke() {

  assert (current()->state() & Thread_running);

  asm volatile
    ("	xorl %%ecx , %%ecx      \n\t"
     "	xorl %%ebx , %%ebx      \n\t"
     "	cmpl $2    , %%edx      \n\t"     // Sigma0
     "	je 1f                   \n\t"
     "  cmpl $4    , %%edx      \n\t"     // Rmgr
     "  jne 2f                  \n\t"
     "1:                        \n\t" 
     "  movl %1    , %%ecx      \n\t"     // Kernel Info Page
     "  movl %2    , %%ebx      \n\t"     // Multiboot Info  
     "2:                        \n\t"
     "  movl %%eax , %%esp      \n\t"
     "  xorl %%edx , %%edx      \n\t"
     "  xorl %%esi , %%esi      \n\t"     // clean out user regs      
     "  xorl %%edi , %%edi      \n\t"                           
     "  xorl %%ebp , %%ebp      \n\t"
     "  xorl %%eax , %%eax      \n\t"
     "  iret"                                                        
     :                          	// no output
     : "d" ((unsigned) Thread::lookup(current())->space_index()),
       "m" (Kmem::virt_to_phys(Kmem::info())),
       "m" (Kmem::virt_to_phys(Boot_info::mbi_virt())),
       "a" (nonull_static_cast<Return_frame*>(current()->regs())));
}

/**
 * Translate a virtual address in a threads user address space into a virtual
 * address in the kernel address space. If no page mapping exists for this
 * address or the page has insufficient access rights for write access, fail.
 * @brief Translate user virtual to kernel virtual address
 * @param addr Virtual address which should be translated
 * @param write Write access to that address is desired
 * @return the virtual address in kernel space corresponding to that address
 */
PUBLIC inline NEEDS ["undef_oskit.h","config.h"]
char *
Thread::uvirt_to_kvirt (Address addr, bool write) {

  Address  phys;
  size_t   size;
  unsigned attr;

  // See if there is a mapping for this address
  if (space()->v_lookup (addr, &phys, &size, &attr)) {

    // See if we want to write and are not allowed to
    // Generic check because INTEL_PTE_WRITE == INTEL_PDE_WRITE
    if (write && !(attr & INTEL_PTE_WRITE))
      return (char *) -1;

    // A frame was found, add the offset
    if (size == Config::SUPERPAGE_SIZE)
      phys |= (addr & ~Config::SUPERPAGE_MASK);
    else
      phys |= (addr & ~Config::PAGE_MASK);
                                 
    // Return the kvirt address for this frame
    return (char *) Kmem::phys_to_virt (phys);         
  }

  return (char *) -1;
}

/**
 * Copy n bytes from virtual user address usrc to virtual kernel address kdst.
 * When crossing page boundaries, addresses must be translated anew
 * @brief Copy between user and kernel address space
 * @param kdst Destination address in kernel space
 * @param usrc Source address in user space
 * @param n Number of bytes to copy
 */
PUBLIC
void
Thread::copy_user_to_kernel (void *kdst, Address usrc, size_t n)
{
  Lock_guard<Cpu_lock> guard (&cpu_lock);

  char *src = NULL;
  char *dst = (char *) kdst;

  while (n--) {

    if (!src)
      while ((src = uvirt_to_kvirt (usrc, false)) == (char *) -1)
        handle_page_fault (usrc, PF_ERR_USERMODE, 0);

    *dst++ = *src++;

    if ((++usrc & (Config::PAGE_SIZE - 1)) == 0)
      src = NULL;
  }
}

/**
 * Copy n bytes from virtual kernel address ksrc to virtual user address udst.
 * When crossing page boundaries, addresses must be translated anew
 * @brief Copy between kernel and user address space
 * @param udst Destination address in user space
 * @param ksrc Source address in kernel space
 * @param n Number of bytes to copy
 */
PUBLIC
void
Thread::copy_kernel_to_user (Address udst, void *ksrc, size_t n)
{
  Lock_guard<Cpu_lock> guard (&cpu_lock);

  char *dst = NULL;
  char *src = (char *) ksrc;

  while (n--) {

    if (!dst)
      while ((dst = uvirt_to_kvirt (udst, true)) == (char *) -1)
        handle_page_fault (udst, PF_ERR_USERMODE, 0);

    *dst++ = *src++;

    if ((++udst & (Config::PAGE_SIZE - 1)) == 0)
      dst = NULL;
  }
}

/**
 * Copy n bytes from virtual user address usrc to virtual user address udst.
 * When crossing page boundaries, addresses must be translated anew
 * @brief Copy between two user address spaces
 * @param partner Destination thread we're copying to
 * @param udst Destination address in partner's user space
 * @param usrc Source address in user space
 * @param n Number of bytes to copy
 */
PUBLIC
void
Thread::copy_user_to_user (Thread *partner, Address udst, Address usrc, size_t n)
{
  Lock_guard<Cpu_lock> guard (&cpu_lock);

  char *src = NULL;
  char *dst = NULL;

  while (n--) {    

    if (!src)
      while ((src = uvirt_to_kvirt (usrc, false)) == (char *) -1)
        handle_page_fault (usrc, PF_ERR_USERMODE, 0);

    if (!dst)
      while ((dst = partner->uvirt_to_kvirt (udst, true)) == (char *) -1)
        handle_ipc_page_fault (udst);

    *dst++ = *src++;

    if ((++usrc & (Config::PAGE_SIZE - 1)) == 0)
      src = NULL;

    if ((++udst & (Config::PAGE_SIZE - 1)) == 0)
      dst = NULL;
  }
}


// The "FPU not available" trap entry point
extern "C" void
thread_handle_fputrap(void)
{
  panic ("fpu trap");
}

inline void
thread_timer_interrupt_arch()
{}

extern "C" void
thread_timer_interrupt_stop()
{}

extern "C" void
thread_timer_interrupt_slow()
{}
