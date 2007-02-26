
/*
 * Fiasco-UX (v2 and x0)
 * Functions for setting up host processes implementing tasks
 */

INTERFACE:

class Hostproc
{
public:
  static unsigned	create	(unsigned int taskno);

private:
  static void		setup	(unsigned int taskno);
};

IMPLEMENTATION:

#include <flux/x86/multiboot.h>
#include <asm/unistd.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "boot_info.h"
#include "config.h"
#include "cpu_lock.h"
#include "emulation.h"
#include "globals.h"
#include "kmem.h"
#include "lock_guard.h"
#include "trampoline.h"

IMPLEMENT
void
Hostproc::setup (unsigned int taskno) {

  sigset_t fullmask;
  stack_t stack;
  char **args = Boot_info::args();
  size_t arglen = 0;
  int status;

  // Zero all args, making ps output pretty
  do arglen += strlen (*args) + 1; while (*++args);
  memset (*Boot_info::args(), 0, arglen); 

  // Put task number into argv[0] for ps
  snprintf (*Boot_info::args(), arglen, "[Task %x]", taskno);

  fclose (stdin);
  fclose (stdout);
  fclose (stderr);

  sigfillset  (&fullmask);
  sigprocmask (SIG_UNBLOCK, &fullmask, NULL);

  stack.ss_sp    = (void *) Emulation::trampoline_page;
  stack.ss_size  = Config::PAGE_SIZE;
  stack.ss_flags = 0;
  status = sigaltstack (&stack, NULL);
  assert (!status);

  /*
   * Install signal handlers the hard way
   * to avoid glibc's restorer code games
   */

  struct kernel_sigaction {
    __sighandler_t      handler;
    unsigned long       flags;  
    void (*restorer) (void);    
    sigset_t            mask;   
  } action;

  action.handler = (__sighandler_t) Emulation::trampoline_page;
  action.flags     = SA_ONSTACK | SA_SIGINFO;
  action.restorer  = 0;
  sigfillset (&action.mask);

  asm volatile ("int $0x80" : "=a"(status) : "a"(__NR_rt_sigaction), "b"(SIGSEGV), "c"(&action), "d"(0), "S"(_NSIG / 8));
  asm volatile ("int $0x80" : "=a"(status) : "a"(__NR_rt_sigaction), "b"(SIGTRAP), "c"(&action), "d"(0), "S"(_NSIG / 8));

  /*
   * Map the magic pages
   */
  mmap ((void *) Emulation::trampoline_page, Config::PAGE_SIZE,
	PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_FIXED,
	Boot_info::fd(), Emulation::trampoline_frame);

  mmap ((void *) Emulation::utcb_address_page, Config::PAGE_SIZE,
	PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED,
	Boot_info::fd(), Emulation::utcb_address_frame);

  ptrace (PTRACE_TRACEME, 0, NULL, NULL);

  raise (SIGSTOP);
}

IMPLEMENT
unsigned
Hostproc::create (unsigned int taskno) {

  Lock_guard<Cpu_lock> guard (&cpu_lock);

  static pid_t pid;
  static unsigned esp;
  static unsigned number;
  int status;

  /* We need a variable which is not on the stack */
  number = taskno;

  /*
   * Careful with local variables here because we are changing the
   * stack pointer for the fork system call. This ensures that the
   * child gets its own COW stack rather than running on the parent
   * stack.
   */      
  asm volatile ("movl %%esp, %0 \n\t"
                "movl %1, %%esp" : "=m" (esp), "=m" (boot_stack));

  /*
   * Make sure the child doesn't inherit any of our unflushed output
   */
  fflush (NULL);

  switch (pid = fork ()) {
  
    case -1:                            // Failed
      return 0;
  
    case 0:                             // Child
    
      /*
       * number resides on the old parent stack. It is safe to read
       * it here, because the parent is waitpid'ing for us and hasn't
       * cleared the stackframe yet
       */

      setup (number);

      _exit (1);                        // unreached
      
    default:                            // Parent

      asm volatile ("movl %0, %%esp" : : "m" (esp));

      while (waitpid (pid, &status, 0) != pid)
        ;
         
      assert (WIFSTOPPED (status) && WSTOPSIG (status) == SIGSTOP);

      Trampoline::munmap (pid, 0, Emulation::utcb_address_page);

      return pid;
  }
}  
