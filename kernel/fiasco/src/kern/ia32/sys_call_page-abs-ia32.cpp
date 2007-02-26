/*
 * Fiasco Syscall-Page Code (absolute addressing)
 */

IMPLEMENTATION[abs-ia32]:

#include <cstdio>
#include "config.h"
#include "cpu.h"
#include "kmem.h"
#include "linker_syms.h"
#include "panic.h"
#include "regdefs.h"
#include "space.h"
#include "types.h"
#include "vmem_alloc.h"

#define OFFSET_ipc             0x000
#define OFFSET_se_ipc          0x000
#define OFFSET_id_nearest      0x100
#define OFFSET_fpage_unmap     0x200
#define OFFSET_thread_switch   0x300
#define OFFSET_thread_schedule 0x400
#define OFFSET_lthread_ex_regs 0x500
#define OFFSET_task_new        0x600

#define SYSCALL_SYMS(sysc) \
extern char sys_call_##sysc, sys_call_##sysc##_end

#define COPY_SYSCALL(sysc) \
memcpy( &_syscalls + OFFSET_##sysc, &sys_call_##sysc, \
        &sys_call_##sysc##_end- &sys_call_##sysc )


IMPLEMENT 
void
Sys_call_page::init()
{
  SYSCALL_SYMS(ipc);
  SYSCALL_SYMS(se_ipc);
  SYSCALL_SYMS(id_nearest);
  SYSCALL_SYMS(fpage_unmap);
  SYSCALL_SYMS(thread_switch);
  SYSCALL_SYMS(thread_schedule);
  SYSCALL_SYMS(lthread_ex_regs);
  SYSCALL_SYMS(task_new);

  Kernel_info *ki = Kmem::info();

  if (!Vmem_alloc::page_alloc(&_syscalls, 0, Vmem_alloc::ZERO_FILL,
                              Space::Page_user_accessible | Kmem::pde_global()))
    panic("FIASCO: can't allocate system-call page.\n");

  printf ("Absolute KIP Syscalls using: %s\n",
          (Cpu::features() & FEAT_SEP) ? "Sysenter" : "int 0x30");

  ki->sys_ipc             = (Mword) &_syscalls;
  ki->sys_id_nearest      = (Mword) &_syscalls + 0x100;
  ki->sys_fpage_unmap     = (Mword) &_syscalls + 0x200;
  ki->sys_thread_switch   = (Mword) &_syscalls + 0x300;
  ki->sys_thread_schedule = (Mword) &_syscalls + 0x400;
  ki->sys_lthread_ex_regs = (Mword) &_syscalls + 0x500;
  ki->sys_task_new        = (Mword) &_syscalls + 0x600;
  ki->kip_sys_calls       = 2;

  if(Cpu::features() & FEAT_SEP)
    COPY_SYSCALL(se_ipc);
  else
    COPY_SYSCALL(ipc);

  COPY_SYSCALL(id_nearest);
  COPY_SYSCALL(fpage_unmap);
  COPY_SYSCALL(thread_switch);
  COPY_SYSCALL(thread_schedule);
  COPY_SYSCALL(lthread_ex_regs);
  COPY_SYSCALL(task_new);

}

asm (FIASCO_ASM_INITDATA

     "sys_call_ipc:                 \n"
     "int $0x30                     \n"
     "ret                           \n"
     "sys_call_ipc_end:             \n"

     "sys_call_id_nearest:          \n"
     "int $0x31                     \n"
     "ret                           \n"
     "sys_call_id_nearest_end:      \n"

     "sys_call_fpage_unmap:         \n"
     "int $0x32                     \n"
     "ret                           \n"
     "sys_call_fpage_unmap_end:     \n"

     "sys_call_thread_switch:       \n"
     "int $0x33                     \n"
     "ret                           \n"
     "sys_call_thread_switch_end:   \n"

     "sys_call_thread_schedule:     \n"
     "int $0x34                     \n"
     "ret                           \n"
     "sys_call_thread_schedule_end: \n"

     "sys_call_lthread_ex_regs:     \n"
     "int $0x35                     \n"
     "ret                           \n"
     "sys_call_lthread_ex_regs_end: \n"

     "sys_call_task_new:            \n"
     "int $0x36                     \n"
     "ret                           \n"
     "sys_call_task_new_end:        \n"

     "sys_call_se_ipc:              \n"
     "0:                            \n"
     "push   %ecx                   \n"
     "subl   $8,%esp                \n"
     "pushl  $(1f-0b+_syscalls)     \n"
     "mov    %esp,%ecx              \n"
     "sysenter                      \n"
     "mov    %ebp,%edx              \n"
     "1:                            \n"
     "ret                           \n"
     "sys_call_se_ipc_end:          \n"

     FIASCO_ASM_FINI);
