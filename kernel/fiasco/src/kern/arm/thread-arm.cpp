IMPLEMENTATION[arm]:

#include <cassert>
#include <cstdio>

#include "globals.h"
#include "sa_1100.h"
#include "timer.h"
#include "thread_state.h"
#include "types.h"
#include "vmem_alloc.h"


enum {
  FSR_STATUS_MASK = 0x0d,
  FSR_TRANSL      = 0x05,
  FSR_DOMAIN      = 0x09,
  FSR_PERMISSION  = 0x0d,


};



// 
// Public services
// 

/** (Re-) Ininialize a thread and make it running.
    This call also cancels IPC.
    @param eip new user instruction pointer.  Set only if != 0xffffffff.
    @param esp new user stack pointer.  Set only if != 0xffffffff.
    @param o_eip return current instruction pointer if pointer != 0
    @param o_esp return current stack pointer if pointer != 0
    @param o_pager return current pager if pointer != 0
    @param o_preempter return current internal preempter if pointer != 0
    @param o_eflags return current eflags register if pointer != 0
    @return false if !exists(); true otherwise
 */
PUBLIC
bool
Thread::initialize(Address ip, Address sp,
		   Thread* pager, Thread* preempter,
		   Address *o_ip = 0, 
		   Address *o_sp = 0,
		   Thread* *o_pager = 0, 
		   Thread* *o_preempter = 0,
		   Address *o_eflags = 0)
{

  (void)o_eflags;
  Lock_guard <Thread_lock> guard (thread_lock());

  if (state() == Thread_invalid)
    return false;

  Entry_frame *r = regs();

  if (o_pager) *o_pager = _pager;
  if (o_preempter) *o_preempter = _preempter;
  if (o_sp) *o_sp = r->sp();
  if (o_ip) *o_ip = r->pc();
  //if (o_eflags) *o_eflags = r->eflags;

  if (ip != 0xffffffff)
    {
      r->pc( ip );
      r->psr = 0x010;
      if (! (state() & Thread_dead))
	{
#if 0
	  kdb_ke("reseting non-dead thread");
#endif
	  // cancel ongoing IPC or other activity
	  state_change(~Thread_ipc_in_progress, Thread_cancel);
	}
    }

  if (pager != 0) _pager = pager;
  if (preempter != 0) _preempter = preempter;
  if (sp != 0xffffffff) r->sp( sp );
  
  state_change(~Thread_dead, Thread_running);

  return true;
}



/** Return to user.  This function is the default routine run if a newly  
    initialized context is being switch_to()'ed.
 */
PROTECTED static 
void Thread::user_invoke() 
{
  //  printf("Thread: %p [state=%08x, space=%p]\n", current(), current()->state(), current_space() );

  assert (current()->state() & Thread_running);
#if 0
  printf("user_invoke of %p @ %08x sp=%08x\n",
	 current(),current()->regs()->pc(),
	 current()->regs()->sp() );
  //current()->regs()->sp(0x30000);
#endif

#if 1
  asm volatile
    ("  mov sp, %[stack_p]    \n"    // set stack pointer to regs structure
     "  mov r1, sp            \n"
     // TODO clean out user regs
     "  ldr lr, [r1], #4      \n"
     "  msr spsr, lr          \n"
     "  ldmia r1, {sp}^       \n"
     "  ldmia r1, {r0}        \n"
     "  add sp,sp, #20        \n"
     "  ldr lr, [sp, #-4]     \n"
     "  movs pc, lr           \n"
     :  
     : 
     [stack_p] "r" (nonull_static_cast<Return_frame*>(current()->regs()))
     );
#endif
  puts("should never be reached");
  while(1) {
    current()->state_del(Thread_running);
    current()->schedule();
  };

  // never returns here
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
	       unsigned short init_prio, unsigned short mcp)
  : Receiver (&_thread_lock, space, init_prio, mcp, Config::default_time_slice), 
    Sender         (id),	// select optimized version of constructor
    _preemption    (id),
    _sched_timeout (this)
{

  // set a magic value -- we use it later to verify the stack hasn't
  // been overrun
  _magic = magic;
  _space = space;
  _irq = 0;
  _recover_jmpbuf = 0;
  _timeout = 0;
  Lock_guard <Thread_lock> guard (thread_lock());

  if (state() != Thread_invalid)
    return;                     // Someone else was faster in initializing!
                                // That's perfectly OK.

  *reinterpret_cast<void(**)()> (--kernel_sp) = user_invoke;

  // clear out user regs that can be returned from the thread_ex_regs
  // system call to prevent covert channel
  Entry_frame *r = regs();
  r->sp(0);
  r->pc(0);

  _space->kmem_update(this);
  // make sure the thread's kernel stack is mapped in its address space
  //  _space->kmem_update(reinterpret_cast<Address>(this));
  
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

IMPLEMENT inline NEEDS["space.h", <cstdio>, "types.h" ,"config.h"]
bool Thread::handle_sigma0_page_fault( Address pfa )
{
  if(pfa<0xc0000000 && pfa>=0xe0000000)
    {
      printf("BAD Sigma0 access @%08x\n",pfa);
      return false;
    }
#if 0
  if(pfa>=0x80000000) 
    { // adapter area
      printf("MAP adapter area: %08x\n",pfa & Config::SUPERPAGE_MASK);
      return (space()->v_insert((pfa & Config::SUPERPAGE_MASK),
				(pfa & Config::SUPERPAGE_MASK),
				Config::SUPERPAGE_SIZE, 
				Space::Page_writable    
				| Space::Page_user_accessible
				| Space::Page_noncacheable)
	      != Space::Insert_err_nomem);
    } 
  else
#endif
    {
      return (space()->v_insert((pfa & Config::SUPERPAGE_MASK),
				(pfa & Config::SUPERPAGE_MASK),
				Config::SUPERPAGE_SIZE, 
				Space::Page_writable    
				| Space::Page_user_accessible)
	      != Space::Insert_err_nomem);
    }
}


static char *error_str[] =
  {"Vector Exception",
   "Alignment",
   "Ext Abort on Linefetch",
   "Translation",
   "Ext Abort on non-Linefetch",
   "Domain",
   "Ext Abort on Translation",
   "Permission"};

extern "C" {

  /**
   * The low-level page fault handler called from entry.S.  We're invoked with
   * interrupts turned off.  Apart from turning on interrupts in almost
   * all cases (except for kernel page faults in TCB area), just forwards
   * the call to Thread::handle_page_fault().
   * @param pfa page-fault virtual address
   * @param error_code CPU error code
   * @return true if page fault could be resolved, false otherwise                      
   */
  void pagefault_entry(const Mword pfa, const Mword error_code, const Mword pc )
  {
    // Pagefault in user mode or interrupts were enabled
    Proc::sti();
    if(!current_thread()->handle_page_fault (pfa, error_code, pc)) {
      printf("slow-trap: pfa=0x%08x, error=0x%08x (%s) , pc=0x%08x\n",
	     pfa, error_code, error_str[(error_code & 1) | ((error_code >> 1) & 6)], pc);
      kdb_ke("UNHANDLED SLOW TRAP");
#warning MUST enter handle slow trap handler here
    }
  }

  void irq_handler(Unsigned32 pc) 
  {
    Unsigned32 irqs = Sa1100::hw_reg(Sa1100::ICIP);
    if(irqs && (1 << 26)) {
      Thread::handle_timer_interrupt();
    }
  }
};

/**
 * Copy n Mwords from virtual user address usrc to virtual kernel address kdst.
 * Normally this is done with GCC's memcpy. When using small address spaces,
 * though, we us the GS segment for access to user space, so we don't mind
 * being moved around address spaces while copying.
 * @brief Copy between user and kernel address space
 * @param kdst Destination address in kernel space
 * @param usrc Source address in user space
 * @param n Number of Mwords to copy
 */
IMPLEMENT inline
template< typename T >
void Thread::copy_from_user(T *kdst, T const *usrc, size_t n)
{
  assert (this == current());

  // copy from user byte by byte
  memcpy( kdst, usrc, n*sizeof(T) );
}

IMPLEMENT inline
template < typename T >
void Thread::poke_user( T *addr, T value)
{
  assert (this == current());

  *addr = value;
}

IMPLEMENT inline
template< typename T >
T Thread::peek_user( T const *addr )
{
  assert (this == current());

  return *addr;
}

PUBLIC
int
Thread::is_valid()
{
  return 1; /* XXX */
}

