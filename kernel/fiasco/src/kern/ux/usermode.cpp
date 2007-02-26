
INTERFACE:

#include <csignal>				// for siginfo_t
#include "types.h"

class Thread;

class Usermode
{
public:
  static void	init();
  static void	set_signal		(int sig, bool ignore);

private:
  static const Address	magic_page;

  static void	emu_handler		(int signal, siginfo_t *, void *ctx);
  static void	int_handler		(int signal, siginfo_t *, void *ctx);
  static void	jdb_handler		(int signal, siginfo_t *, void *ctx);

  static void	iret_to_user_mode	(struct ucontext *context, Mword *kesp);
  static void	iret_to_kern_mode	(struct ucontext *context, Mword *kesp);
  static void	iret			(struct ucontext *context);

  static Mword	peek_at_addr		(pid_t pid, Address eip, unsigned n);


  static void	kernel_entry		(struct ucontext *context,
	                                 Mword trap,
					 Mword xss,
					 Mword esp,
					 Mword efl,
					 Mword xcs,
					 Mword eip,
					 Mword err,
					 Mword cr2);

  /**
   * @brief Wait for host process to stop.
   * @param pid process id to wait for.
   * @return signal the host process stopped with.
   */
  static int wait_for_stop (pid_t pid);

  /**
   * @brief Set emulated internal processor interrupt state.
   * @param mask signal mask to modify
   * @param eflags processor flags register
   */
  static void sync_interrupt_state (sigset_t *mask, Mword eflags);

  /**
   * @brief Cancel a native system call in the host.
   * @param pid process id of the host process.
   * @param regs register set at the time of the system call.
   */
  static void cancel_syscall (pid_t pid, struct user_regs_struct *regs);

public:
  /**
   * @brief Read debug register
   * @param pid process id of the host process.
   * @param reg number of debug register (0..7)
   * @param value reference to register value
   * @return 0 is ok
   */
  static int read_debug_register (pid_t pid, Mword reg, Mword &value);

  /**
   * @brief Write debug register
   * @param pid process id of the host process.
   * @param reg number of debug register (0..7)
   * @param value register value to be written.
   * @return 0 is ok
   */
  static int write_debug_register (pid_t pid, Mword reg, Mword value);
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
#include <signal.h>
#include <ucontext.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include "undef_page.h"

#include "boot_info.h"
#include "config.h"
#include "config_tcbsize.h"
#include "emulation.h"
#include "fpu.h"
#include "globals.h"
#include "initcalls.h"
#include "linker_syms.h"
#include "pic.h"
#include "processor.h"
#include "regdefs.h"
#include "thread.h"
#include "thread_util.h"

const Address Usermode::magic_page =
     (Address) Kmem::phys_to_virt (Emulation::trampoline_frame);

Proc::Status volatile Proc::virtual_processor_state = 0;

IMPLEMENT
Mword
Usermode::peek_at_addr (pid_t pid, Address eip, unsigned n)
{
  Mword val;

  switch (n)
    {
      case 1:
        switch (eip & 3)
          {
            case 0:  val = ptrace (PTRACE_PEEKTEXT, pid, eip & ~3, NULL); break;
            case 1:  val = ptrace (PTRACE_PEEKTEXT, pid, eip & ~3, NULL) >> 8; break;
            case 2:  val = ptrace (PTRACE_PEEKTEXT, pid, eip & ~3, NULL) >> 16; break;
            default: val = ptrace (PTRACE_PEEKTEXT, pid, eip & ~3, NULL) >> 24; break;
          }
        return val & 0xff;

      case 2:
        switch (eip & 3)
          {
            case 0:  val = ptrace (PTRACE_PEEKTEXT, pid, eip & ~3, NULL); break;
            case 1:  val = ptrace (PTRACE_PEEKTEXT, pid, eip & ~3, NULL) >> 8; break;
            case 2:  val = ptrace (PTRACE_PEEKTEXT, pid, eip & ~3, NULL) >> 16; break;
            default: val = ptrace (PTRACE_PEEKTEXT, pid, eip, NULL); break;
          }
        return val & 0xffff;

      case 3:
        switch (eip & 3)
          {
            case 0:  val = ptrace (PTRACE_PEEKTEXT, pid, eip & ~3, NULL); break;
            case 1:  val = ptrace (PTRACE_PEEKTEXT, pid, eip & ~3, NULL) >> 8; break;
            default: val = ptrace (PTRACE_PEEKTEXT, pid, eip, NULL); break;
          }
        return val & 0xffffff;

      default:
        return ptrace (PTRACE_PEEKTEXT, pid, eip, NULL);
    }
}

IMPLEMENT inline NEEDS [<cassert>, <sys/types.h>, <sys/wait.h>]
int
Usermode::wait_for_stop (pid_t pid)
{
  int status;
  pid_t p;
  
  p = waitpid (pid, &status, 0);

  assert (p == pid && WIFSTOPPED (status));

  return WSTOPSIG (status);
}

IMPLEMENT
void
Usermode::sync_interrupt_state (sigset_t *mask, Mword eflags)
{
  Proc::ux_set_virtual_processor_state (eflags);

  if (!mask)
    return;

  if (eflags & EFLAGS_IF)
    { 
      sigdelset (mask, SIGIO);
      sigdelset (mask, SIGINT);
    }
  else
    {
      sigaddset (mask, SIGIO);
      sigaddset (mask, SIGINT);
    }
}

IMPLEMENT inline NEEDS [<sys/ptrace.h>, <sys/user.h>, "undef_page.h"]
void
Usermode::cancel_syscall (pid_t pid, struct user_regs_struct *regs)
{
  ptrace (PTRACE_POKEUSER, pid, offsetof (struct user, regs.orig_eax), -1);
  ptrace (PTRACE_SYSCALL, pid, NULL, NULL);

  wait_for_stop (pid);

  regs->eax = regs->orig_eax;
  ptrace (PTRACE_POKEUSER, pid, offsetof (struct user, regs.eax), regs->eax);
}

IMPLEMENT
int
Usermode::read_debug_register (pid_t pid, Mword reg, Mword &value)
{
  if (reg > 7)
    return 0;

  int ret = ptrace (PTRACE_PEEKUSER, pid, 
		    offsetof (struct user, u_debugreg[reg]), NULL);
  if (ret == -1 && errno == -1)
    return 0;

  value = ret;
  return 1;
}

IMPLEMENT
int
Usermode::write_debug_register (pid_t pid, Mword reg, Mword value)
{
  if (reg > 7)
    return 0;

  int ret = ptrace (PTRACE_POKEUSER, pid,
		    offsetof (struct user, u_debugreg[reg]), value);
  return ret == -1 ? 0 : 1;
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
 * @param trap Trap number that caused kernel entry (0xffffffff == shutdown)
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
                        Mword trap,
                        Mword xss,
                        Mword esp,
                        Mword efl,
                        Mword xcs,
                        Mword eip,
                        Mword err,
                        Mword cr2)
{
  Mword *kesp = (xcs & 3) == 2
              ? (Mword *) *Kmem::kernel_esp() - 5
              : (Mword *) context->uc_mcontext.gregs[REG_ESP] - 3;

  // Make sure the kernel stack is sane
  assert ((Mword) kesp >= Kmem::mem_tcbs &&
          (Mword) kesp <  Kmem::mem_tcbs + (Config::thread_block_size << 18));

  // Make sure the kernel stack has enough space
  assert ((Mword) kesp % THREAD_BLOCK_SIZE > THREAD_BLOCK_SIZE / 4);

  efl &= ~EFLAGS_IF;

  switch (xcs & 3)
    {
      case 2:
        *(kesp + 4) = xss;
        *(kesp + 3) = esp;

      case 0:
        *(kesp + 2) = efl | (Proc::processor_state() & EFLAGS_IF);
        *(kesp + 1) = xcs;
        *(kesp + 0) = eip;
    }

  switch (trap)
    {
      case 0xe:					// Page Fault
        Emulation::set_fault_addr (cr2);
      case 0x8:					// Double Fault
      case 0xa:					// Invalid TSS
      case 0xb:					// Segment Not Present
      case 0xc:					// Stack Fault
      case 0xd:					// General Protection Fault
      case 0x11:				// Alignment Check
        *--kesp = err;
    }

  context->uc_mcontext.gregs[REG_ESP] = (Mword) kesp;
  context->uc_mcontext.gregs[REG_EIP] = Emulation::idt_vector (trap);
  context->uc_mcontext.gregs[REG_EFL] = efl;
  sync_interrupt_state (&context->uc_sigmask, efl);

  // Make sure interrupts are off
  assert (!Proc::interrupts());
}

/**
 * IRET to a user context.
 * We restore the saved context on the stack, namely EIP, CS, EFLAGS, ESP, SS.
 * Additionally all register values are transferred to the task's register set.
 * @param ctx Kern context during iret
 */
IMPLEMENT
void
Usermode::iret_to_user_mode (struct ucontext *context, Mword *kesp)
{
  struct user_regs_struct regs;
  struct ucontext *task_context;
  unsigned int trap, error, fault;
  long int opcode;
  int stop, irq_pend;
  Thread *t = Thread::lookup (context_of (kesp));
  pid_t pid = t->space_context()->pid();

  Pic::set_owner (pid);

  /*
   * If there are any interrupts pending up to this point, don't start the task
   * but let it enter kernel immediately. Any interrupts occuring beyond this
   * point will go directly to the task.
   */
  if ((irq_pend = Pic::irq_pending()) != -1) {

    Pic::eat (irq_pend);

    Pic::set_owner (Boot_info::pid());

    kernel_entry (context,
	          Pic::map_irq_to_gate (irq_pend),
                  *(kesp + 4),			/* XSS */
                  *(kesp + 3),			/* ESP */
                  *(kesp + 2),			/* EFL */
                  2,				/* XCS */
                  *(kesp + 0),			/* EIP */
                  0,				/* ERR */
                  0);				/* CR2 */
    return;
  }

#if 0
   memset ((void *) magic_page, 0, Config::PAGE_SIZE);
#endif      
  
  // Restore these from the kernel stack (iret context)
  regs.eip    = *(kesp + 0);
  regs.xcs    = *(kesp + 1);
  regs.eflags = *(kesp + 2);
  regs.esp    = *(kesp + 3);
  regs.xss    = *(kesp + 4);

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

  Fpu::restore_state (t->fpu_state());

  while (1) {
  
    ptrace (Boot_info::is_native (t->id().task())
            ? PTRACE_CONT 
            : PTRACE_SYSCALL, pid, NULL, NULL);

    stop = wait_for_stop (pid);

    ptrace (PTRACE_GETREGS, pid, NULL, &regs);

    if (stop == SIGWINCH || stop == SIGTERM || stop == SIGINT)
      continue;

    else if (stop == SIGSEGV) {

      if (regs.eip >= (int) &_syscalls &&
          regs.eip <  (int) &_syscalls + 0x700 &&
          (regs.eip & 0xff) == 0) {

        trap  = 0x30 + ((regs.eip - (int) &_syscalls) >> 8);
        error = 0;
        fault = 0;

        // Get call return address from stack and adjust stack
        regs.eip = peek_at_addr (pid, regs.esp, 4);
        regs.esp += 4;

      } else if (((opcode = peek_at_addr (pid, regs.eip, 2)) & 0xff) == 0xcd &&
                  (trap = (opcode >> 8) & 0xff) >= 0x30 && trap <= 0x36) {
 
        error = 0;
        fault = 0;
        regs.eip += 2;

      } else {

        /* Setup task sighandler code */
        memcpy ((void *) magic_page, &_task_sighandler_start,
             &_task_sighandler_end - &_task_sighandler_start);

        ptrace (PTRACE_CONT, pid, NULL, SIGSEGV);

        wait_for_stop (pid);

        // See corresponding code in sighandler.S
        task_context = reinterpret_cast<struct ucontext *>
                   (magic_page + *reinterpret_cast<Address *>(magic_page + 0x100));

        trap  = task_context->uc_mcontext.gregs[REG_TRAPNO];	/* trapno */
        error = task_context->uc_mcontext.gregs[REG_ERR];	/* error_code */
        fault = task_context->uc_mcontext.cr2;			/* fault addr */

        switch (trap) {

          case 0xe:				/* Usermode Page Fault */
            error |= PF_ERR_USERADDR;
            break;

          case 0xd:				/* Usermode Protection Fault */
            switch (peek_at_addr (pid, regs.eip, 1)) {

              case 0xfa:	/* cli */
		Pic::set_owner (Boot_info::pid());
                regs.eip++;
                regs.eflags &= ~EFLAGS_IF;
                sync_interrupt_state (0, regs.eflags);
                if (ptrace (PTRACE_SETREGS, pid, NULL, &regs) == -1)
                  perror ("ptrace/SETREGS");
                continue;

              case 0xfb:	/* sti */
		Pic::set_owner (pid);
                regs.eip++;
                regs.eflags |= EFLAGS_IF;
                sync_interrupt_state (0, regs.eflags);
                if (ptrace (PTRACE_SETREGS, pid, NULL, &regs) == -1)
                  perror ("ptrace/SETREGS");
                continue;
            }
            /* Fall through */

          default:				/* Usermode Exception */
            break;
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

    } else if (stop == SIGTRAP) {
    
      if (peek_at_addr (pid, regs.eip - 2, 2) == 0x80cd)
        {
          cancel_syscall (pid, &regs);

          kernel_entry (context, 0xd,
                        regs.xss,		/* XSS */
                        regs.esp,		/* ESP */
                        regs.eflags,		/* EFL */
                        2,			/* XCS */
                        regs.eip - 2,		/* EIP */
                        0x80 << 3 | 2,		/* ERR */
                        0);			/* CR2 */
        }
      else
        {
          kernel_entry (context, 0x3,
                        regs.xss,		/* XSS */
                        regs.esp,		/* ESP */
                        regs.eflags,		/* EFL */
                        2,			/* XCS */
                        regs.eip,		/* EIP */
                        0,			/* ERR */
                        0);			/* CR2 */
        }
      break;

    } else if (stop == SIGILL) {
      
      kernel_entry (context, 0x6,
                    regs.xss,		/* XSS */
                    regs.esp,		/* ESP */
                    regs.eflags,	/* EFL */
                    2,			/* XCS */
                    regs.eip,		/* EIP */
                    0,			/* ERR */
                    0);			/* CR2 */
                    
      break;

    } else if (stop == SIGFPE) {
      
      kernel_entry (context, 0x10,
                    regs.xss,		/* XSS */
                    regs.esp,		/* ESP */
                    regs.eflags,	/* EFL */
                    2,			/* XCS */
                    regs.eip,		/* EIP */
                    0,			/* ERR */
                    0);			/* CR2 */
                    
      break;

    } else if (stop == SIGIO) {

      /*
       * Interrupts that were trapped on the task are silently discared.
       * Restart task.
       */
      if ((irq_pend = Pic::irq_pending()) == -1)
        continue;

      Pic::eat (irq_pend);

      kernel_entry (context,
	            Pic::map_irq_to_gate (irq_pend),
                    regs.xss,		/* XSS */
                    regs.esp,		/* ESP */
                    regs.eflags,	/* EFL */
                    2,			/* XCS */
                    regs.eip,		/* EIP */
                    0,			/* ERR */
                    0);			/* CR2 */
                    
      break;

    } else
      printf ("Unexpected signal %d to pid %d\n", stop, pid);
  }

  Pic::set_owner (Boot_info::pid());

  if (Pic::irq_pending() != -1)
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
Usermode::iret_to_kern_mode (struct ucontext *context, Mword *kesp)
{
  context->uc_mcontext.gregs[REG_EIP]  = *(kesp + 0);
  context->uc_mcontext.gregs[REG_EFL]  = *(kesp + 2);
  context->uc_mcontext.gregs[REG_ESP] += 3 * sizeof (Mword);
}

/**
 * Emulate IRET instruction.
 * Depending on the value of the saved code segment (CS) on the kernel stack
 * we return to kernel mode (CPL == 0) or user mode (CPL == 2).
 * @param ctx Kern context during iret
 */
IMPLEMENT
void
Usermode::iret (struct ucontext *context)
{
  Mword *kesp = (Mword *) context->uc_mcontext.gregs[REG_ESP];

  sync_interrupt_state (&context->uc_sigmask, *(kesp + 2));

  switch (*(kesp + 1) & 3)
    {
      case 0:			/* CPL 0 -> Kernel */
        iret_to_kern_mode (context, kesp);
        break;

      case 2:			/* CPL 2 -> User */
        iret_to_user_mode (context, kesp);
        break;
    }
}

IMPLEMENT
void
Usermode::emu_handler (int, siginfo_t *, void *ctx)
{
  struct ucontext *context = reinterpret_cast<struct ucontext *>(ctx);
  unsigned int trap = context->uc_mcontext.gregs[REG_TRAPNO];
  unsigned char opcode;

  if (trap == 0xd)	/* General protection fault */
    {
      switch (opcode = *reinterpret_cast<unsigned char *>
             (context->uc_mcontext.gregs[REG_EIP]))
        {
          case 0xfa:					/* cli */
            context->uc_mcontext.gregs[REG_EIP]++;
            context->uc_mcontext.gregs[REG_EFL] &= ~EFLAGS_IF;
            sync_interrupt_state (&context->uc_sigmask,
                                   context->uc_mcontext.gregs[REG_EFL]);
            return;

          case 0xfb:					/* sti */
            context->uc_mcontext.gregs[REG_EIP]++;
            context->uc_mcontext.gregs[REG_EFL] |= EFLAGS_IF;
            sync_interrupt_state (&context->uc_sigmask,
                                   context->uc_mcontext.gregs[REG_EFL]);
            return;

          case 0xcf:					/* iret */
            iret (context);
            return;
        }
    }

  kernel_entry (context, trap,
                context->uc_mcontext.gregs[REG_SS],
                context->uc_mcontext.gregs[REG_ESP],
                context->uc_mcontext.gregs[REG_EFL],
                0,					/* Fake CS */
                context->uc_mcontext.gregs[REG_EIP],
                context->uc_mcontext.gregs[REG_ERR] & ~PF_ERR_USERMODE,
                context->uc_mcontext.cr2);
}

IMPLEMENT
void
Usermode::int_handler (int, siginfo_t *, void *ctx)
{
  struct ucontext *context = reinterpret_cast<struct ucontext *>(ctx);
  int irq;

  if ((irq = Pic::irq_pending()) == -1)
    return;

  Pic::eat (irq);

  kernel_entry (context,
                Pic::map_irq_to_gate (irq),
                context->uc_mcontext.gregs[REG_SS],	/* XSS */
                context->uc_mcontext.gregs[REG_ESP],	/* ESP */
                context->uc_mcontext.gregs[REG_EFL],	/* EFL */
                0,					/* XCS */
                context->uc_mcontext.gregs[REG_EIP],	/* EIP */
                0,					/* ERR */
                0);					/* CR2 */
}

IMPLEMENT
void
Usermode::jdb_handler (int sig, siginfo_t *, void *ctx)
{
  struct ucontext *context = reinterpret_cast<struct ucontext *>(ctx);

  /*
   * If a SIGSEGV is pending at the same time as SIGINT, i.e. because
   * someone pressed Ctrl-C on an sti instruction, SIGINT will be delivered
   * first. Since we warp to a different execution path the pending SIGSEGV
   * will then hit an innocent instruction elsewhere with fatal consequences.
   * Therefore a pending SIGSEGV must be cancelled - it will later reoccur.
   */
   
  set_signal (SIGSEGV, true);

  kernel_entry (context, sig == SIGTRAP ? 3 : 1,
                context->uc_mcontext.gregs[REG_SS],	/* XSS */
                context->uc_mcontext.gregs[REG_ESP],	/* ESP */
                context->uc_mcontext.gregs[REG_EFL],	/* EFL */
                0,					/* XCS */
                context->uc_mcontext.gregs[REG_EIP],	/* EIP */
                0,					/* ERR */
                0);					/* CR2 */
}

IMPLEMENT
void
Usermode::set_signal (int sig, bool ignore)
{
  void (*func)(int, siginfo_t *, void *);
  
  switch (sig)
    {
      case SIGINT:
      case SIGTRAP:
      case SIGTERM:
      case SIGXCPU:	func = jdb_handler;	break;
      case SIGIO:	func = int_handler;	break;
      case SIGSEGV:	func = emu_handler;	break;
      
      case SIGWINCH:
      case SIGPROF:
      case SIGHUP:
      case SIGUSR1:
      case SIGUSR2:	ignore = true;		// fall through
      
      default:		func = 0;		break;
    }

  if (ignore)			// Cancel a signal by ignoring it and then
    signal (sig, SIG_IGN);	// reinstalling the handler. See POSIX 3.3.1.3
  
  if (func)
    {
      struct sigaction action;

      sigfillset (&action.sa_mask);	/* No other signals while we run */
      action.sa_sigaction = func;
      action.sa_flags     = SA_RESTART | SA_ONSTACK | SA_SIGINFO;

      check (sigaction (sig, &action, NULL) == 0);
    }
}

IMPLEMENT FIASCO_INIT
void
Usermode::init()
{
  int i;
  stack_t stack;

  /* We want signals, aka interrupts to be delivered on an alternate stack */
  stack.ss_sp    = Kmem::phys_to_virt (Emulation::sigstack_start_frame);
  stack.ss_size  = Emulation::sigstack_end_frame - Emulation::sigstack_start_frame;
  stack.ss_flags = 0;
  check (sigaltstack (&stack, NULL) == 0);

  /* Setup signal handlers */
  for (i = 0; i < SIGRTMIN; i++)
    set_signal (i, false);
}
