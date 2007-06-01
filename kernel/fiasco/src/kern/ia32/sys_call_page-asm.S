
#include "tcboffset.h"

	.section ".initcall.data","aw"

sys_call_ipc:
	int	$0x30
	ret
sys_call_ipc_end:

sys_call_id_nearest:
	int	$0x31
	ret
sys_call_id_nearest_end:

sys_call_fpage_unmap:
	int	$0x32
	ret
sys_call_fpage_unmap_end:

sys_call_thread_switch:
	int	$0x33
	ret
sys_call_thread_switch_end:

sys_call_thread_schedule:
	int	$0x34
	ret
sys_call_thread_schedule_end:

sys_call_lthread_ex_regs:
	int	$0x35
	ret
sys_call_lthread_ex_regs_end:

sys_call_task_new:
	int	$0x36
	ret
sys_call_task_new_end:

sys_call_privctrl:
	int	$0x37
	ret
sys_call_privctrl_end:

sys_call_ulock:
	int	$0x39
	ret
sys_call_ulock_end:

sys_call_se_ipc:
	0:
	push	%ecx
	subl	$4,%esp
	pushl	$0x1b
	pushl	$(1f-0b+VAL__MEM_LAYOUT__SYSCALLS)
	mov	%esp,%ecx
	sysenter
	mov	%ebp,%edx
	1:
	ret
sys_call_se_ipc_end:

	.global	sys_call_ipc
	.global	sys_call_ipc_end
	.global	sys_call_id_nearest
	.global	sys_call_id_nearest_end
	.global	sys_call_fpage_unmap
	.global	sys_call_fpage_unmap_end
	.global	sys_call_thread_switch
	.global	sys_call_thread_switch_end
	.global	sys_call_thread_schedule
	.global	sys_call_thread_schedule_end
	.global	sys_call_lthread_ex_regs
	.global	sys_call_lthread_ex_regs_end
	.global	sys_call_task_new
	.global	sys_call_task_new_end
	.global sys_call_privctrl
	.global sys_call_privctrl_end
	.global sys_call_ulock
	.global sys_call_ulock_end
	.global	sys_call_se_ipc
	.global	sys_call_se_ipc_end

	.previous
