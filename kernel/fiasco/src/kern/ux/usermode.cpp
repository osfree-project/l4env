
INTERFACE:

#include <csignal>				// for siginfo_t
#include "types.h"

class Usermode
{

public:
  static void	init();

private:
  static const Address	magic_page;

  static void	set_signal		(int sig, void (*func)
                                        (int, siginfo_t *, void *));

  static void	segv_handler		(int signal, siginfo_t *, void *ctx);
  static void	term_handler		(int signal, siginfo_t *, void *ctx);
  static void	sigio_handler		(int signal, siginfo_t *, void *ctx);

  static void	iret_to_user_mode	(struct ucontext *context);
  static void	iret_to_kern_mode	(struct ucontext *context);
  static void	do_iret			(struct ucontext *context);
  static void	terminate		(struct ucontext *context);

  static void	kernel_entry		(struct ucontext *context,
	                                 unsigned int trap,
					 unsigned int xss,
					 unsigned int esp,
					 unsigned int efl,
					 unsigned int xcs,
					 unsigned int eip,
					 unsigned int err,
					 unsigned int cr2);
};

IMPLEMENTATION:

#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <panic.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/ptrace.h>
#include <sys/ucontext.h>
#include <sys/user.h>
#include <sys/wait.h>

#include "boot_info.h"
#include "config.h"
#include "config_tcbsize.h"
#include "emulation.h"
#include "fpu.h"
#include "globals.h"
#include "initcalls.h"
#include "kdb_ke.h"
#include "linker_syms.h"
#include "pic.h"
#include "processor.h"
#include "regdefs.h"
#include "thread.h"
#include "thread_util.h"
#include "undef_oskit.h"

const Address Usermode::magic_page =
     (Address) Kmem::phys_to_virt (Emulation::trampoline_frame);

Proc::Status volatile Proc::virtual_processor_state = 0;

/**
 * Kernel shutdown function
 * We want to shut down due to a termination signal. In order to
 * return properly from main we have to return to the idle loop and
 * return from there. Force scheduling of the idle thread even if
 * there are other threads runnable by giving it the highest prio.
 */
IMPLEMENT
void
Usermode::terminate (struct ucontext *context)
{
  running = 0;

  kernel_thread->ready_dequeue();
  kernel_thread->sched()->set_prio(255);
  kernel_thread->ready_enqueue();

  // Simulate timer interrupt to invoke scheduler
  kernel_entry (context, 0x28,
                context->uc_mcontext.gregs[REG_SS],	/* XSS */
                context->uc_mcontext.gregs[REG_ESP],	/* ESP */
                context->uc_mcontext.gregs[REG_EFL],	/* EFL */
                0,					/* XCS */
                context->uc_mcontext.gregs[REG_EIP],	/* EIP */
                0,					/* ERR */
                0);					/* CR2 */
}

/**
 * Set up kernel stack for kernel entry through an interrupt gate.
 * We are running on the signal stack and modifying the interrupted context
 * on the signal stack to allow us to return anywhere.
 * Depending on the value of the code segment (CS) we set up a processor
 * context of 3 (kernel mode) or 5 (user mode) words on the respective kernel
 * stack, which is the current stack (kernel mode) or the stack determined by
 * the Task State Segment (user mode). For some traps we need to put an
 * additional error code on the stack.
 * This is precisely what an ia32 processor does in hardware.
 * @param context Interrupted context on signal stack
 * @param trap Trap number that caused kernel entry
 * @param xss Stack Segment
 * @param esp Stack Pointer
 * @param efl EFLAGS
 * @param xcs Code Segment
 * @param eip Instruction Pointer
 * @param err Error Code
 * @param cr2 Page Fault Address (if applicable)
 */
IMPLEMENT
void
Usermode::kernel_entry (struct ucontext *context,
                        unsigned int trap,
                        unsigned int xss,
                        unsigned int esp,
                        unsigned int efl,
                        unsigned int xcs,
                        unsigned int eip,
                        unsigned int err,
                        unsigned int cr2) {

  unsigned int kstack;

  /*
   * Sanity check: Here we expect to be a 32-bit interrupt gate. Fail if this is not so
   */
  assert ((*reinterpret_cast<unsigned int *>
         ((char *) Emulation::get_idt_base() + trap * 8 + 4) & 0xe00) == 0xe00);

  /*
   * If interrupts were previously disabled, reflect this fact in EFLAGS
   */
  efl = (efl & ~EFLAGS_IF) | (Proc::processor_state() & EFLAGS_IF);

  sigaddset (&context->uc_sigmask, SIGIO);		/* Interrupts Off */
  Proc::ux_set_virtual_processor_state (efl & ~EFLAGS_IF);

  switch (xcs & 3) {

    case 0:
      kstack = context->uc_mcontext.gregs[REG_ESP];	/* Current Stack */
      *reinterpret_cast<unsigned int *>(kstack - 4)  = efl;
      *reinterpret_cast<unsigned int *>(kstack - 8)  = xcs;
      *reinterpret_cast<unsigned int *>(kstack - 12) = eip;
      *reinterpret_cast<unsigned int *>(kstack - 16) = err;
      context->uc_mcontext.gregs[REG_ESP] = kstack - 3 * sizeof (unsigned int);
      break;

    default:
      kstack = *Kmem::kernel_esp();			/* TSS Stack 0 */
      *reinterpret_cast<unsigned int *>(kstack - 4)  = xss;
      *reinterpret_cast<unsigned int *>(kstack - 8)  = esp;
      *reinterpret_cast<unsigned int *>(kstack - 12) = efl;
      *reinterpret_cast<unsigned int *>(kstack - 16) = xcs;
      *reinterpret_cast<unsigned int *>(kstack - 20) = eip;
      *reinterpret_cast<unsigned int *>(kstack - 24) = err;
      context->uc_mcontext.gregs[REG_ESP] = kstack - 5 * sizeof (unsigned int);
      break;
  }

  switch (trap) {
    case 0xe:				/* Page Fault			*/
      Emulation::set_fault_addr (cr2);
    case 0x8:				/* Double Fault			*/
    case 0xa:				/* Invalid TSS			*/
    case 0xb:				/* Segment Not Present		*/
    case 0xc:				/* Stack Fault			*/
    case 0xd:				/* General Protection Fault	*/
    case 0x11:				/* Alignment Check		*/
      context->uc_mcontext.gregs[REG_ESP] -= sizeof (unsigned int);
      break;
  }

  /* Figure out gate entry */
  context->uc_mcontext.gregs[REG_EIP] =
  *reinterpret_cast<unsigned *>((char *) Emulation::get_idt_base() + trap * 8)     & 0x0000ffff |
  *reinterpret_cast<unsigned *>((char *) Emulation::get_idt_base() + trap * 8 + 4) & 0xffff0000;

  if (Boot_info::debug_ctxt_switch()) {
    L4_uid id = Thread::lookup(context_of ((void *) (kstack - 4)))->id();
    printf ("thread %u:%u enter kernel (%02x) EIP:%08x CS:%02x EFL:%08x CR2:%08x KStack: %08x [%s]\n",
            id.task(), id.lthread(), trap,
            eip, xcs, efl, cr2, kstack,
            (xcs & 3) ? "user mode" : "kern mode");
  }

  /* Make sure the kernel stack is sane */
  assert (kstack >= Kmem::mem_tcbs &&
          kstack <  Kmem::mem_tcbs + (Config::thread_block_size << 18));

  /* Stack underflow check */
  if ((kstack - 4) % THREAD_BLOCK_SIZE < THREAD_BLOCK_SIZE / 4)
    kdb_ke ("possible kernel stack corruption");
}

/**
 * IRET to a user context.
 * We restore the saved context on the stack, namely EIP, CS, EFLAGS, ESP, SS.
 * Additionally all register values are transferred to the task's register set.
 * @param ctx Kern context during iret
 */
IMPLEMENT
void
Usermode::iret_to_user_mode (struct ucontext *context)
{
  unsigned int esp = context->uc_mcontext.gregs[REG_ESP];
  struct user_regs_struct regs;
  struct ucontext *task_context;
  struct pollfd pfd;
  unsigned int trap, error, fault;
  long int opcode;
  int status;
  char buffer[8];

  assert ((context->uc_mcontext.gregs[REG_EFL] & EFLAGS_IF) != 0);

  Context *t = context_of ((void *) esp);
  pid_t pid = t->space_context()->pid();

  fcntl (sockets[0], F_SETOWN, pid);

  /*
   * If there are any interrupts pending up to this point, don't start the task
   * but let it enter kernel immediately. Any interrupts occuring beyond this
   * point will go directly to the task.
   */
  pfd = (struct pollfd) { fd : sockets[0], events : POLLIN, revents : 0 };

  poll (&pfd, 1, 0);

  if (pfd.revents & POLLIN) {

    read (pfd.fd, buffer, sizeof (buffer));

    fcntl (sockets[0], F_SETOWN, Boot_info::pid());

    kernel_entry (context, 0x28,
                  *reinterpret_cast<unsigned int *>(esp + 16),	/* XSS */
                  *reinterpret_cast<unsigned int *>(esp + 12),	/* ESP */
                  *reinterpret_cast<unsigned int *>(esp + 8),	/* EFL */
                  2,						/* XCS */
                  *reinterpret_cast<unsigned int *>(esp),	/* EIP */
                  0,						/* ERR */
                  0);						/* CR2 */
    return;
  }

#if 0
   memset ((void *) magic_page, 0, Config::PAGE_SIZE);
#endif      
  
  // Restore these from the kernel stack (iret context)
  regs.eip    = *reinterpret_cast<unsigned int *>(esp);
  regs.xcs    = *reinterpret_cast<unsigned int *>(esp + 4);
  regs.eflags = *reinterpret_cast<unsigned int *>(esp + 8);
  regs.esp    = *reinterpret_cast<unsigned int *>(esp + 12);
  regs.xss    = *reinterpret_cast<unsigned int *>(esp + 16);

  // Copy these from the kernel
  regs.eax    = context->uc_mcontext.gregs[REG_EAX];
  regs.ebx    = context->uc_mcontext.gregs[REG_EBX];
  regs.ecx    = context->uc_mcontext.gregs[REG_ECX];
  regs.edx    = context->uc_mcontext.gregs[REG_EDX];
  regs.esi    = context->uc_mcontext.gregs[REG_ESI];
  regs.edi    = context->uc_mcontext.gregs[REG_EDI];
  regs.ebp    = context->uc_mcontext.gregs[REG_EBP];
  regs.xds    = context->uc_mcontext.gregs[REG_DS];
  regs.xes    = context->uc_mcontext.gregs[REG_ES];
  regs.xfs    = context->uc_mcontext.gregs[REG_FS];
  regs.xgs    = context->uc_mcontext.gregs[REG_GS];

  if (ptrace (PTRACE_SETREGS, pid, NULL, &regs) == -1)
    perror ("ptrace/SETREGS");

  Fpu::restore_state(t->fpu_state());

  while (1) {
  
    ptrace (PTRACE_SYSCALL, pid, NULL, NULL);

    while (waitpid (pid, &status, 0) != pid);			/* Spin */

    if (WIFEXITED (status))
     printf ("task %u exited with code %u\n", pid, WEXITSTATUS (status));

    if (WIFSIGNALED (status))
      printf ("task %u exited with signal %u\n", pid, WTERMSIG (status));

    assert (WIFSTOPPED (status));

    ptrace (PTRACE_GETREGS, pid, NULL, &regs);

    if (WSTOPSIG (status) == SIGINT || WSTOPSIG (status) == SIGTERM)
      terminate (context);

    else if (WSTOPSIG (status) == SIGWINCH)
      continue;

    else if (WSTOPSIG (status) == SIGSEGV) {

      if (regs.eip >= (int) &_syscalls &&
          regs.eip <  (int) &_syscalls + 0x700 &&
          (regs.eip & 0xff) == 0) {

        trap  = 0x30 + ((regs.eip - (int) &_syscalls) >> 8);
        error = 0;
        fault = 0;

        // Get call return address from stack and adjust stack
        regs.eip = ptrace (PTRACE_PEEKTEXT, pid, regs.esp, NULL);
        regs.esp += 4;

      } else if (((opcode = ptrace (PTRACE_PEEKTEXT, pid, regs.eip, NULL)) &
                   0xff) == 0xcd && (trap = (opcode >> 8) & 0xff) >= 0x30 &&
                   trap <= 0x36) {

        error = 0;
        fault = 0;
        regs.eip += 2;

      } else {

        /* Setup task sighandler code */
        memcpy ((void *) magic_page, &_task_sighandler_start,
             &_task_sighandler_end - &_task_sighandler_start);

        ptrace (PTRACE_CONT, pid, NULL, SIGSEGV);

        while (waitpid (pid, &status, 0) != pid);			/* Spin */

        if (WIFEXITED (status))
          printf ("trace_task exited with code %u\n", WEXITSTATUS (status));

        if (WIFSIGNALED (status))
          printf ("trace_task exited with signal %u\n", WTERMSIG (status));

        assert (WIFSTOPPED (status));

        // See corresponding code in sighandler.S
        task_context = reinterpret_cast<struct ucontext *>
                   (magic_page + *reinterpret_cast<Address *>(magic_page + 0x100));

        trap  = task_context->uc_mcontext.gregs[REG_TRAPNO];	/* trapno */
        error = task_context->uc_mcontext.gregs[REG_ERR];	/* error_code */
        fault = task_context->uc_mcontext.cr2;			/* fault addr */

        switch (trap) {

          case 0xe:				/* Usermode Page Fault */
            error |= PF_ERR_USERADDR;
            if (Boot_info::debug_page_faults()) {
              L4_uid id = Thread::lookup(context_of ((void *) (*Kmem::kernel_esp() - 4)))->id();
              printf ("user pagefault at EIP:%08lx ESP:%08lx PFA:%08x Error:%x Thread:%u.%u\n",
                      regs.eip, regs.esp, fault, error, id.task(), id.lthread());
            }
            break;

          case 0xd:				/* Usermode Protection Fault */
            switch ((opcode = ptrace (PTRACE_PEEKTEXT, pid, regs.eip, NULL)) & 0xff) {

              case 0xfa:					/* cli */
              case 0xfb:					/* sti */
                printf ("cli/sti from usermode (eip:%08lx). continuing\n", regs.eip);
                regs.eip++;
                if (ptrace (PTRACE_SETREGS, pid, NULL, &regs) == -1)
                  perror ("ptrace/SETREGS");
                continue;
            }
            break;	/* Other privileged opcodes, enter kernel */

          default:
            L4_uid id = Thread::lookup(context_of ((void *) (*Kmem::kernel_esp() - 4)))->id();
            panic ("unhandled trap at EIP:%08lx ESP:%08lx Trap:%02x Error:%x Thread:%u.%u\n",
                    regs.eip, regs.esp, trap, error, id.task(), id.lthread());
        }
      }

      kernel_entry (context, trap,
                    regs.xss,		/* XSS */
                    regs.esp,		/* ESP */
                    regs.eflags,	/* EFL */
                    2,			/* XCS */
                    regs.eip,		/* EIP */
                    error,		/* ERR */
                    fault);		/* CR2 */
                    
      break;

    } else if (WSTOPSIG (status) == SIGTRAP) {
      
      kernel_entry (context, 0x3,
                    regs.xss,		/* XSS */
                    regs.esp,		/* ESP */
                    regs.eflags,	/* EFL */
                    2,			/* XCS */
                    regs.eip,		/* EIP */
                    0,			/* ERR */
                    0);			/* CR2 */
                    
      break;

    } else if (WSTOPSIG (status) == SIGILL) {
      
      kernel_entry (context, 0x6,
                    regs.xss,		/* XSS */
                    regs.esp,		/* ESP */
                    regs.eflags,	/* EFL */
                    2,			/* XCS */
                    regs.eip,		/* EIP */
                    0,			/* ERR */
                    0);			/* CR2 */
                    
      break;

    } else if (WSTOPSIG (status) == SIGIO) {

      /*
       * Interrupts that were trapped on the task are silently discared. Restart task
       */
      if (read (sockets[0], buffer, sizeof (buffer)) <= 0)
        continue;

      kernel_entry (context, 0x28,
                    regs.xss,		/* XSS */
                    regs.esp,		/* ESP */
                    regs.eflags,	/* EFL */
                    2,			/* XCS */
                    regs.eip,		/* EIP */
                    0,			/* ERR */
                    0);			/* CR2 */
                    
      break;

    } else
      kdb_ke ("Task stopped with unexpected signal");
  }

  fcntl (sockets[0], F_SETOWN, Boot_info::pid());

  pfd = (struct pollfd) { fd : sockets[0], events : POLLIN, revents : 0 };

  poll (&pfd, 1, 0);

  if (pfd.revents & POLLIN)
    kill (Boot_info::pid(), SIGIO);

  context->uc_mcontext.gregs[REG_EAX] = regs.eax;
  context->uc_mcontext.gregs[REG_EBX] = regs.ebx;
  context->uc_mcontext.gregs[REG_ECX] = regs.ecx;
  context->uc_mcontext.gregs[REG_EDX] = regs.edx;
  context->uc_mcontext.gregs[REG_ESI] = regs.esi;
  context->uc_mcontext.gregs[REG_EDI] = regs.edi;
  context->uc_mcontext.gregs[REG_EBP] = regs.ebp;
  context->uc_mcontext.gregs[REG_DS]  = regs.xds;
  context->uc_mcontext.gregs[REG_ES]  = regs.xes;
  context->uc_mcontext.gregs[REG_FS]  = regs.xfs;
  context->uc_mcontext.gregs[REG_GS]  = regs.xgs;

  Fpu::save_state(t->fpu_state());
}

/**
 * IRET to a kernel context.
 * We restore the saved context on the stack, namely EIP and EFLAGS.
 * We do NOT restore CS, because the kernel thinks it has privilege level 0
 * but in usermode it has to have privilege level 3. We also adjust ESP by
 * 3 words, thus clearing the context from the stack.
 * @param ctx Kern context during iret
 */
IMPLEMENT
void
Usermode::iret_to_kern_mode (struct ucontext *ctx)
{
  unsigned kstack = ctx->uc_mcontext.gregs[REG_ESP];

  ctx->uc_mcontext.gregs[REG_EIP]  = *reinterpret_cast<unsigned *>(kstack);
  ctx->uc_mcontext.gregs[REG_EFL]  = *reinterpret_cast<unsigned *>(kstack + 8);
  ctx->uc_mcontext.gregs[REG_ESP] += 3 * sizeof (unsigned);
}

/**
 * Emulate IRET instruction.
 * Depending on the value of the saved code segment (CS) on the kernel stack
 * we return to kernel mode (CPL == 0) or user mode (CPL != 0).
 * @param ctx Kern context during iret
 */
IMPLEMENT
void
Usermode::do_iret (struct ucontext *ctx) {

  unsigned int esp = ctx->uc_mcontext.gregs[REG_ESP];

  if (Boot_info::debug_ctxt_switch()) {
    L4_uid id = Thread::lookup (context_of ((void *) esp))->id();
    printf ("thread %u:%u iret to %s mode EIP:%08x CS:%02x EFL:%08x, KStack %08x\n",
            id.task(), id.lthread(),
            *reinterpret_cast<unsigned int *>(esp + 4) & 3 ? "user" : "kern",
            *reinterpret_cast<unsigned int *>(esp),
            *reinterpret_cast<unsigned int *>(esp + 4),
            *reinterpret_cast<unsigned int *>(esp + 8),
            *reinterpret_cast<unsigned int *>(esp + 4) & 3 ?
              *Kmem::kernel_esp() : esp + 3 * sizeof (unsigned));
  }

  /*
   * Set IF bit in EFLAGS according to the context we restore
   */
  Proc::ux_set_virtual_processor_state (ctx->uc_mcontext.gregs[REG_EFL]);

  switch (*reinterpret_cast<unsigned *>(esp + 4) & 3) {

    case 0:			/* CPL0 -> Kernel */
      iret_to_kern_mode (ctx);
      break;

    default:			/* CPL3 -> User */
      iret_to_user_mode (ctx);
      break;
  }
}

IMPLEMENT
void
Usermode::segv_handler (int signal, siginfo_t *, void *ctx) {

  struct ucontext *context = reinterpret_cast<struct ucontext *>(ctx);
  unsigned int trap = context->uc_mcontext.gregs[REG_TRAPNO];
  unsigned char opcode;

  switch (signal) {

    case SIGSEGV:

      switch (trap) {

        case 0xd:					/* General protection fault */
  
          switch ((opcode = *reinterpret_cast<unsigned char *>(context->uc_mcontext.gregs[REG_EIP]))) {

            case 0xfa:					/* cli */
              sigaddset (&context->uc_sigmask, SIGIO);
              context->uc_mcontext.gregs[REG_EIP]++;
              Proc::ux_set_virtual_processor_state (Proc::processor_state() & ~EFLAGS_IF);
              return;

            case 0xfb:					/* sti */
              sigdelset (&context->uc_sigmask, SIGIO);
              context->uc_mcontext.gregs[REG_EIP]++;
              Proc::ux_set_virtual_processor_state (Proc::processor_state() | EFLAGS_IF);
              return;

            case 0xcf:					/* iret */
              do_iret (context);
              return;

            default:
              printf ("Unexpected Opcode [%08x] at EIP:%08x\n",
                      *reinterpret_cast<unsigned int *>(context->uc_mcontext.gregs[REG_EIP]),
                      context->uc_mcontext.gregs[REG_EIP]);
              break;
         }

        case 0xe:					/* Page fault */
          if (Boot_info::debug_page_faults())
            printf ("kern pagefault at EIP:%08x ESP:%08x PFA:%08lx Error:%x\n",
                    context->uc_mcontext.gregs[REG_EIP],
                    context->uc_mcontext.gregs[REG_ESP],
                    context->uc_mcontext.cr2,
                    context->uc_mcontext.gregs[REG_ERR] & ~PF_ERR_USERMODE);
          break;

        default:
          printf ("Unexpected Trap [%08x]. Trapping into kernel!\n", trap);
          break;
      }
  }

  kernel_entry (context, trap,
                context->uc_mcontext.gregs[REG_SS],
                context->uc_mcontext.gregs[REG_ESP],
                context->uc_mcontext.gregs[REG_EFL],
                0,					/* CS */
                context->uc_mcontext.gregs[REG_EIP],
                context->uc_mcontext.gregs[REG_ERR] & ~PF_ERR_USERMODE,
                context->uc_mcontext.cr2);
}

/*
 * We want to shut down due to a termination signal. In order to
 * return properly from main we have to return to the idle loop and
 * return from there. Force scheduling of the idle thread even if
 * there are other threads runnable by giving it the highest prio.
 */
IMPLEMENT
void
Usermode::term_handler (int, siginfo_t *, void *ctx) {

  struct ucontext *context = reinterpret_cast<struct ucontext *>(ctx);

  terminate (context);
}

IMPLEMENT
void
Usermode::sigio_handler (int, siginfo_t *, void *ctx) {

  struct ucontext *context = reinterpret_cast<struct ucontext *>(ctx);
  char buffer[8];
  struct pollfd pfd = (struct pollfd) { fd: sockets[0], events: POLLIN, revents: 0 };

  poll (&pfd, 1, 0);

  if (pfd.revents & POLLIN) {

    read (pfd.fd, buffer, sizeof (buffer));

    kernel_entry (context, 0x28,
                  context->uc_mcontext.gregs[REG_SS],	/* XSS */
                  context->uc_mcontext.gregs[REG_ESP],	/* ESP */
                  context->uc_mcontext.gregs[REG_EFL],	/* EFL */
                  0,					/* XCS */
                  context->uc_mcontext.gregs[REG_EIP],	/* EIP */
                  0,					/* ERR */
                  0);					/* CR2 */
  }
}

IMPLEMENT FIASCO_INIT
void
Usermode::set_signal (int sig, void (*func)(int, siginfo_t *, void *))
{
  struct sigaction action;

  sigfillset (&action.sa_mask);		/* No other signals while we run */
  action.sa_sigaction = func;
  action.sa_flags     = SA_RESTART | SA_ONSTACK | SA_SIGINFO;

  check (sigaction (sig, &action, NULL) == 0);
}

IMPLEMENT FIASCO_INIT
void
Usermode::init()
{
  stack_t stack;

  /* We want signals, aka interrupts to be delivered on an alternate stack */
  stack.ss_sp    = Kmem::phys_to_virt (8192);	/* Use third phys page as sigstack */
  stack.ss_size  = Config::PAGE_SIZE;
  stack.ss_flags = 0;
  check (sigaltstack (&stack, NULL) == 0);

  set_signal (SIGIO,	sigio_handler);
  set_signal (SIGSEGV,	segv_handler);
  set_signal (SIGINT,	term_handler);
  set_signal (SIGHUP,	term_handler);
  set_signal (SIGTERM,	term_handler);
  set_signal (SIGXCPU,	term_handler);

  signal (SIGWINCH,	SIG_IGN);
  signal (SIGPROF,	SIG_IGN);
  signal (SIGUSR1,	SIG_IGN);
  signal (SIGUSR2,	SIG_IGN);
}
