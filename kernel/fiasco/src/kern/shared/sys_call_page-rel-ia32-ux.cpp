/*
 * Fiasco Syscall-Page Code (relative addressing)
 * Shared between UX and native IA32.
 */

INTERFACE [{ia32,ux}-rel_syscalls]:

#include "types.h"


//---------------------------------------------------------------------------
IMPLEMENTATION [{ia32,ux}-rel_syscalls]:

#include <cstddef>
#include <cstdio>
#include <cstring>
#include "cpu.h"
#include "initcalls.h"
#include "kip.h"
#include "regdefs.h"

static inline
Mword
Sys_call_page::sys_call_addr (char *absolute)
{
  extern char sys_call_page;
  return (absolute - &sys_call_page) + offsetof (Kip, sys_calls);
}


//---------------------------------------------------------------------------
IMPLEMENTATION [{ia32,ux}-rel_syscalls]:

IMPLEMENT
void
Sys_call_page::init()
{
  extern char sys_call_page, end_of_sys_call_page;
  extern char sp_sys_ipc, sp_sys_id_nearest, sp_sys_fpage_unmap;
  extern char sp_sys_thread_switch, sp_sys_thread_schedule;
  extern char sp_sys_lthread_ex_regs, sp_sys_task_new;
  extern char sp_se_sys_ipc;

  memcpy (Kip::k()->sys_calls, &sys_call_page,
          &end_of_sys_call_page - &sys_call_page);

  printf ("Relative KIP Syscalls using: %s\n",
          Cpu::have_sysenter() ? "Sysenter" : "int 0x30");

  Kip::k()->sys_ipc             = Cpu::have_sysenter() ?
                                    sys_call_addr (&sp_se_sys_ipc) :
                                    sys_call_addr (&sp_sys_ipc);
  Kip::k()->sys_id_nearest      = sys_call_addr (&sp_sys_id_nearest);
  Kip::k()->sys_fpage_unmap     = sys_call_addr (&sp_sys_fpage_unmap);
  Kip::k()->sys_thread_switch   = sys_call_addr (&sp_sys_thread_switch);
  Kip::k()->sys_thread_schedule = sys_call_addr (&sp_sys_thread_schedule);
  Kip::k()->sys_lthread_ex_regs = sys_call_addr (&sp_sys_lthread_ex_regs);
  Kip::k()->sys_task_new        = sys_call_addr (&sp_sys_task_new);
  Kip::k()->kip_sys_calls       = 1;
}

asm (FIASCO_ASM_INITDATA

     ".p2align 4                    \n"
     "sys_call_page:                \n"

     ".p2align 4                    \n"
     "sp_sys_ipc:                   \n"
     "int $0x30                     \n"
     "ret                           \n"

     ".p2align 4                    \n"
     "sp_sys_id_nearest:            \n"
     "int $0x31                     \n"
     "ret                           \n"

     ".p2align 4                    \n"
     "sp_sys_fpage_unmap:           \n"
     "int $0x32                     \n"
     "ret                           \n"

     ".p2align 4                    \n"
     "sp_sys_thread_switch:         \n"
     "int $0x33                     \n"
     "ret                           \n"

     ".p2align 4                    \n"
     "sp_sys_thread_schedule:       \n"
     "int $0x34                     \n"
     "ret                           \n"

     ".p2align 4                    \n"  
     "sp_sys_lthread_ex_regs:       \n"
     "int $0x35                     \n"
     "ret                           \n"

     ".p2align 4                    \n"
     "sp_sys_task_new:              \n"
     "int $0x36                     \n"
     "ret                           \n"

     ".p2align 4                    \n"
     "sp_se_sys_ipc:                \n"
     "push   %ecx                   \n"
     "subl   $8,%esp                \n"
     "call 0f                       \n"
     "0:                            \n"
     "addl   $(1f-0b), (%esp)       \n"
     "mov    %esp,%ecx              \n"
     "sysenter                      \n"
     "mov    %ebp,%edx              \n"
     "1:                            \n"
     "ret                           \n"

     "end_of_sys_call_page:         \n"

     FIASCO_ASM_FINI);
