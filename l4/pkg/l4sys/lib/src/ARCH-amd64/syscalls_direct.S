
__l4sys_direct_syscalls = 0xffffffffeacff000
.hidden __l4sys_direct_syscalls

	.macro	direct_syscall syscall,offset
\syscall = __l4sys_direct_syscalls + \offset
.global \syscall
	.endm

	direct_syscall	__l4sys_fpage_unmap_direct, 0x200
	direct_syscall	__l4sys_id_nearest_direct, 0x100
	direct_syscall	__l4sys_ipc_direct, 0x000
	direct_syscall	__l4sys_lthread_ex_regs_direct, 0x500
	direct_syscall	__l4sys_privctrl_direct, 0x700
	direct_syscall	__l4sys_task_new_direct, 0x600
	direct_syscall	__l4sys_thread_schedule_direct, 0x400
	direct_syscall	__l4sys_thread_switch_direct, 0x300
