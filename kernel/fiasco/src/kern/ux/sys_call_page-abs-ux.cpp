/*
 * Fiasco Syscall-Page Code (absolute addressing)
 * Architecture specific UX code.
 */

IMPLEMENTATION [ux-abs_syscalls]:

#include "kmem.h"
#include "mem_layout.h"
#include "types.h"

IMPLEMENT 
void
Sys_call_page::init()
{
  Kip *ki = Kip::k();

  ki->sys_ipc             = Mem_layout::Syscalls;
  ki->sys_id_nearest      = Mem_layout::Syscalls + 0x100;
  ki->sys_fpage_unmap     = Mem_layout::Syscalls + 0x200;
  ki->sys_thread_switch   = Mem_layout::Syscalls + 0x300;
  ki->sys_thread_schedule = Mem_layout::Syscalls + 0x400;
  ki->sys_lthread_ex_regs = Mem_layout::Syscalls + 0x500;
  ki->sys_task_new        = Mem_layout::Syscalls + 0x600;
  ki->kip_sys_calls       = 2;
}
