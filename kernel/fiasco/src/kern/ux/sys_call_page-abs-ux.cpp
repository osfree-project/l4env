/*
 * Fiasco Syscall-Page Code (absolute addressing)
 * Architecture specific UX code.
 */

IMPLEMENTATION[abs-ux]:

#include "kmem.h"
#include "linker_syms.h"
#include "types.h"

IMPLEMENT 
void
Sys_call_page::init()
{
  Kernel_info *ki = Kmem::info();

  ki->sys_ipc             = (Mword) &_syscalls;
  ki->sys_id_nearest      = (Mword) &_syscalls + 0x100;
  ki->sys_fpage_unmap     = (Mword) &_syscalls + 0x200;
  ki->sys_thread_switch   = (Mword) &_syscalls + 0x300;
  ki->sys_thread_schedule = (Mword) &_syscalls + 0x400;
  ki->sys_lthread_ex_regs = (Mword) &_syscalls + 0x500;
  ki->sys_task_new        = (Mword) &_syscalls + 0x600;
  ki->kip_sys_calls       = 2;
}
