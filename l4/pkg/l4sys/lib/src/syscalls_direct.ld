__L4_direct_syscalls = 0xeacff000;	

__L4_ipc_direct             = __L4_direct_syscalls + 0x000;
__L4_id_nearest_direct      = __L4_direct_syscalls + 0x100;
__L4_fpage_unmap_direct     = __L4_direct_syscalls + 0x200;
__L4_thread_switch_direct   = __L4_direct_syscalls + 0x300;
__L4_thread_schedule_direct = __L4_direct_syscalls + 0x400;
__L4_lthread_ex_regs_direct = __L4_direct_syscalls + 0x500;
__L4_task_new_direct        = __L4_direct_syscalls + 0x600;
