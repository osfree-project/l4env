/*
 * Fiasco Syscall-Page Code (ABI version 4, relative addressing)
 * Shared between UX and native IA32.
 */

IMPLEMENTATION[ia32-ux-v4]:

#include <cstddef>
#include <cstdio>
#include <cstring>
#include "cpu.h"
#include "initcalls.h"
#include "linker_syms.h"
#include "kip.h"
#include "kmem.h"
#include "regdefs.h"
#include "types.h"

static inline
Mword
sys_call_addr (char *absolute)
{
  extern char sys_call_page;
  return (absolute - &sys_call_page) + offsetof (Kernel_info, sys_calls);
}

IMPLEMENT
void
Sys_call_page::init()
{
  extern char sys_call_page, end_of_sys_call_page,
    sp_sys_space_control, sp_sys_thread_control,
    sp_sys_processor_control, sp_sys_memory_control,
    sp_sys_ipc, sp_sys_lipc, sp_sys_fpage_unmap,
    sp_sys_thread_ex_regs, sp_sys_clock,
    sp_sys_thread_switch, sp_sys_thread_schedule,
    sp_se_sys_ipc;

  Kernel_info *ki = Kmem::info();

  memcpy (ki->sys_calls, &sys_call_page,
          &end_of_sys_call_page - &sys_call_page);

  printf ("Relative KIP Syscalls using: %s\n",
          0 /*(Cpu::features() & FEAT_SEP)*/ ? "Sysenter" : "int 0x30");

  ki->sys_space_control	    = sys_call_addr (&sp_sys_space_control    );
  ki->sys_thread_control    = sys_call_addr (&sp_sys_thread_control   );
  ki->sys_processor_control = sys_call_addr (&sp_sys_processor_control);
  ki->sys_memory_control    = sys_call_addr (&sp_sys_memory_control   );
  ki->sys_ipc		    = 0 /* Cpu::features() & FEAT_SEP */ 
         ? sys_call_addr (&sp_se_sys_ipc) : sys_call_addr (&sp_sys_ipc);
  ki->sys_lipc		    = sys_call_addr (&sp_sys_lipc	      );
  ki->sys_fpage_unmap	    = sys_call_addr (&sp_sys_fpage_unmap      );
  ki->sys_thread_ex_regs    = sys_call_addr (&sp_sys_thread_ex_regs   );
  ki->sys_clock		    = sys_call_addr (&sp_sys_clock	      );
  ki->sys_thread_switch	    = sys_call_addr (&sp_sys_thread_switch    );
  ki->sys_thread_schedule   = sys_call_addr (&sp_sys_thread_schedule  );
}

asm (FIASCO_ASM_INITDATA

     ".p2align 4		\n"
     "sys_call_page:		\n"

     ".p2align 4		\n"
     "sp_sys_space_control:	\n"
     "int $0x30			\n"
     "ret			\n"

     ".p2align 4		\n"
     "sp_sys_thread_control:	\n"
     "int $0x31			\n"
     "ret			\n"

     ".p2align 4		\n"
     "sp_sys_processor_control:	\n"
     "int $0x32			\n"
     "ret			\n"

     ".p2align 4		\n"
     "sp_sys_memory_control:	\n"
     "int $0x33			\n"
     "ret			\n"

     ".p2align 4		\n"
     "sp_sys_ipc:		\n"
     "int $0x34			\n"
     "ret			\n"

     ".p2align 4		\n"
     "sp_sys_lipc:		\n"
     "int $0x35			\n"
     "ret			\n"

     ".p2align 4		\n"
     "sp_sys_fpage_unmap:	\n"
     "int $0x36			\n"
     "ret			\n"

     ".p2align 4		\n"
     "sp_sys_thread_ex_regs:	\n"
     "int $0x37			\n"
     "ret			\n"

     ".p2align 4		\n"
     "sp_sys_clock:		\n"
     "int $0x38			\n"
     "ret			\n"

     ".p2align 4		\n"
     "sp_sys_thread_switch:	\n"
     "int $0x39			\n"
     "ret			\n"

     ".p2align 4		\n"
     "sp_sys_thread_schedule:	\n"
     "int $0x3A			\n"
     "ret			\n"

     ".p2align 4		\n"
     "sp_se_sys_ipc:		\n"
     "push   %ecx		\n"
     "subl   $8,%esp		\n"
     "call 0f			\n"
     "0:			\n"
     "addl   $(1f-0b), (%esp)	\n"
     "mov    %esp,%ecx		\n"
     "sysenter			\n"
     "mov    %ebp,%edx		\n"
     "1:			\n"
     "ret			\n"

     "end_of_sys_call_page:	\n"

     FIASCO_ASM_FINI);
