
INTERFACE:

#include <sys/types.h>

class Trampoline
{
private:
  static const unsigned		syscall_opcode = 0x80cd;
  static unsigned		phys_magic;

public:
  static void	mmap		(pid_t pid, unsigned int start, size_t length,
                                 int prot, int flags, int fd, off_t offset);

  static void	munmap		(pid_t pid, unsigned int start, size_t length);

  static void	mprotect	(pid_t pid, unsigned int start, size_t length,
                                 int prot);
};

IMPLEMENTATION:

#include <cassert>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <asm/unistd.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>

#include "cpu_lock.h"
#include "emulation.h"
#include "kmem.h"
#include "lock_guard.h"

unsigned Trampoline::phys_magic = (unsigned) Kmem::phys_to_virt (Emulation::trampoline_frame);

/* 
 * Trampoline Operations
 *
 * These run on the trampoline page which is shared between all tasks and
 * mapped into their respective address space. phys_magic is the bottom of
 * the magic page where the magic code runs, phys_magic + pagesize is the
 * top of the magic page, which the code uses as stack.
 */

IMPLEMENT
void
Trampoline::mmap (pid_t pid, unsigned int start, size_t length,
                  int prot, int flags, int fd, off_t offset)
{
  Lock_guard<Cpu_lock> guard(&cpu_lock);

  struct user_regs_struct regs, stackregs;
  int i;

  ptrace (PTRACE_GETREGS, pid, NULL, &regs);

  *(unsigned int *)(phys_magic)      = syscall_opcode;
  *(unsigned int *)(phys_magic + 4)  = start;
  *(unsigned int *)(phys_magic + 8)  = length;
  *(unsigned int *)(phys_magic + 12) = prot;
  *(unsigned int *)(phys_magic + 16) = flags;
  *(unsigned int *)(phys_magic + 20) = fd;
  *(unsigned int *)(phys_magic + 24) = offset;

  stackregs     = regs;					/* Start with valid segment selectors */
  stackregs.eax = __NR_mmap;
  stackregs.ebx = Emulation::trampoline_page + 4;
  stackregs.eip = Emulation::trampoline_page;

  ptrace (PTRACE_SETREGS, pid, NULL, &stackregs);	/* Set Registers */

  /* Loop until we get SIGTRAP. We might get orphaned SIGIO in between */
  
  do {
    ptrace (PTRACE_SYSCALL, pid, NULL, NULL);		/* Kernel Entry */
    while (waitpid (pid, &i, 0) != pid);		/* Spin */
  } while (WIFSTOPPED (i) && WSTOPSIG (i) != SIGTRAP);

  do {
    ptrace (PTRACE_SYSCALL, pid, NULL, NULL);		/* Kernel Exit */
    while (waitpid (pid, &i, 0) != pid);		/* Spin */
  } while (WIFSTOPPED (i) && WSTOPSIG (i) != SIGTRAP);

  ptrace (PTRACE_GETREGS, pid, NULL, &stackregs);	/* Get Registers for return value */
  ptrace (PTRACE_SETREGS, pid, NULL, &regs);		/* Restore Registers */

  assert ((unsigned) stackregs.eax == start);
}

IMPLEMENT
void
Trampoline::munmap (pid_t pid, unsigned int start, size_t length)
{
  Lock_guard<Cpu_lock> guard(&cpu_lock);

  struct user_regs_struct regs, stackregs;
  int i;

  ptrace (PTRACE_GETREGS, pid, NULL, &regs);

  *(unsigned int *)(phys_magic) = syscall_opcode;

  stackregs     = regs;					/* Start with valid segment selectors */
  stackregs.eax = __NR_munmap;
  stackregs.ebx = start;
  stackregs.ecx = length;
  stackregs.eip = Emulation::trampoline_page;
  
  ptrace (PTRACE_SETREGS, pid, NULL, &stackregs);	/* Set Registers */

  /* Loop until we get SIGTRAP. We might get orphaned SIGIO in between */

  do {
    ptrace (PTRACE_SYSCALL, pid, NULL, NULL);		/* Kernel Entry */
    while (waitpid (pid, &i, 0) != pid);		/* Spin */
  } while (WIFSTOPPED (i) && WSTOPSIG (i) != SIGTRAP);

  do {
    ptrace (PTRACE_SYSCALL, pid, NULL, NULL);		/* Kernel Exit */
    while (waitpid (pid, &i, 0) != pid);		/* Spin */
  } while (WIFSTOPPED (i) && WSTOPSIG (i) != SIGTRAP);

  ptrace (PTRACE_GETREGS, pid, NULL, &stackregs);	/* Get Registers for return value */
  ptrace (PTRACE_SETREGS, pid, NULL, &regs);		/* Restore Registers */

  assert (stackregs.eax == 0);
}

IMPLEMENT
void
Trampoline::mprotect (pid_t pid, unsigned int start, size_t length, int prot)
{
  Lock_guard<Cpu_lock> guard(&cpu_lock);
  
  struct user_regs_struct regs, stackregs;
  int i;

  ptrace (PTRACE_GETREGS, pid, NULL, &regs);

  *(unsigned int *)(phys_magic) = syscall_opcode;

  stackregs     = regs;					/* Start with valid segment selectors */
  stackregs.eax = __NR_mprotect;
  stackregs.ebx = start;
  stackregs.ecx = length;
  stackregs.edx = prot;
  stackregs.eip = Emulation::trampoline_page;
  
  ptrace (PTRACE_SETREGS, pid, NULL, &stackregs);	/* Set Registers */

  /* Loop until we get SIGTRAP. We might get orphaned SIGIO in between */

  do {
    ptrace (PTRACE_SYSCALL, pid, NULL, NULL);		/* Kernel Entry */
    while (waitpid (pid, &i, 0) != pid);		/* Spin */
  } while (WIFSTOPPED (i) && WSTOPSIG (i) != SIGTRAP);

  do {
    ptrace (PTRACE_SYSCALL, pid, NULL, NULL);		/* Kernel Exit */
    while (waitpid (pid, &i, 0) != pid);		/* Spin */
  } while (WIFSTOPPED (i) && WSTOPSIG (i) != SIGTRAP);

  ptrace (PTRACE_GETREGS, pid, NULL, &stackregs);	/* Get Registers for return value */
  ptrace (PTRACE_SETREGS, pid, NULL, regs);		/* Restore Registers */

  assert (stackregs.eax == 0);
}
