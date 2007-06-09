
	.globl	PREFIX(l4_ipc_call_asm)
	.align	16
PREFIX(l4_ipc_call_asm:)
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	push	%ebp
	movl	%eax,%esi
	movl	%edx,%eax
	movl	%ecx,%edx
	movl	20(%esp),%ebx
	movl	24(%esp),%ebp
	movl	36(%esp),%ecx
	sub	%edi,%edi
	andl	$0xfffffffe,%ebp
#ifdef KIP_SYSCALLS
	call	__l4sys_ipc_direct
#else
#ifndef L4V2_IPC_SYSENTER
	int	$0x30
#else
	pushl	%ecx
	push	%ebp
	push	$0x1b
	pushl	$1f
	movl	%esp,%ecx
	sysenter
	movl	%ebp,%edx
1:
#endif
#endif
	movl	28(%esp),%edi
	movl	32(%esp),%esi
	movl	40(%esp),%ecx
	movl	%edx,(%edi)
	movl	%ebx,(%esi)
	movl	%eax,(%ecx)
	andl	$0xf0, %eax
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret

	.globl	PREFIX(l4_ipc_reply_and_wait_asm)
	.align	16
PREFIX(l4_ipc_reply_and_wait_asm:)
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	push	%ebp
	movl	%eax,%esi
	movl	%edx,%eax
	movl	%ecx,%edx
	movl	20(%esp),%ebx
	movl	28(%esp),%ebp
	movl	40(%esp),%ecx
	sub	%edi,%edi
	orl	$0x00000001,%ebp
#ifdef KIP_SYSCALLS
	call	__l4sys_ipc_direct
#else
#ifndef L4V2_IPC_SYSENTER
	int	$0x30
#else
	pushl	%ecx
	pushl	%ebp
	pushl	$0x1b
	pushl	$1f
	movl	%esp,%ecx
	sysenter
	movl	%ebp,%edx
1:
#endif
#endif
	movl	24(%esp),%ecx
	movl	%esi,(%ecx)
	movl	32(%esp),%edi
	movl	36(%esp),%esi
	movl	44(%esp),%ecx
	movl	%edx,(%edi)
	movl	%ebx,(%esi)
	movl	%eax,(%ecx)
	andl	$0xf0,%eax
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret

#include <l4/sys/ipc-invoke.h>

/* Put everything needed for the test task to one page */
	.p2align 12
	.global PREFIX(task_new_page_start)
PREFIX(task_new_page_start):
	.global	PREFIX(task_stack_compact)
PREFIX(task_stack_compact):
	.space 1024
	.global	PREFIX(task_ping_id_compact)
PREFIX(task_ping_id_compact):
	.space 4
	.global PREFIX(syscall_task_new_test_task_compact)
PREFIX(syscall_task_new_test_task_compact):
	pushl %ebp
	xorl  %eax,%eax
	xorl  %ecx,%ecx
	xorl  %edi,%edi
	movl  PREFIX(task_ping_id_compact),%esi
	orl   $-1, %ebp
	IPC_SYSENTER_ASM
	popl  %ebp

	xorl  $-1,%eax
	xorl  %ecx,%ecx
	xorl  %esi,%esi
	pushl %ebp
	xorl  %ebp,%ebp
	IPC_SYSENTER_ASM
	.global PREFIX(task_new_page_end)
PREFIX(task_new_page_end):
