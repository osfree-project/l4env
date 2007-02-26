/*
 * Fiasco Syscall-Page Code (absolute addressing)
 */

IMPLEMENTATION [ia32-abs_syscalls]:

#include <cstdio>
#include "config.h"
#include "cpu.h"
#include "mem_layout.h"
#include "panic.h"
#include "paging.h"
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
memcpy( (char*)Mem_layout::Syscalls + OFFSET_##sysc, &sys_call_##sysc, \
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

  if (!Vmem_alloc::page_alloc((void*)Mem_layout::Syscalls,
			      Vmem_alloc::ZERO_FILL,
                              Space::Page_user_accessible | 
			      Pd_entry::global()))
    panic("FIASCO: can't allocate system-call page.\n");

  printf ("Absolute KIP Syscalls using: %s\n",
          Cpu::have_sysenter() ? "Sysenter" : "int 0x30");

  Kip::k()->sys_ipc             = Mem_layout::Syscalls;
  Kip::k()->sys_id_nearest      = Mem_layout::Syscalls + 0x100;
  Kip::k()->sys_fpage_unmap     = Mem_layout::Syscalls + 0x200;
  Kip::k()->sys_thread_switch   = Mem_layout::Syscalls + 0x300;
  Kip::k()->sys_thread_schedule = Mem_layout::Syscalls + 0x400;
  Kip::k()->sys_lthread_ex_regs = Mem_layout::Syscalls + 0x500;
  Kip::k()->sys_task_new        = Mem_layout::Syscalls + 0x600;
  Kip::k()->kip_sys_calls       = 2;

  if (Cpu::have_sysenter())
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
