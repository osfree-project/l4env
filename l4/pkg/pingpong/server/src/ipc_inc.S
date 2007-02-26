
	.globl	PREFIX(l4_ipc_call_asm)
	.align	16
PREFIX(l4_ipc_call_asm:)
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	push	%ebp
	movl	%eax,%esi
	movl	%edx,%edi
	movl	%ecx,%eax
	movl	20(%esp),%edx
	movl	24(%esp),%ebx
	movl	28(%esp),%ebp
	movl	40(%esp),%ecx
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
	movl	32(%esp),%edi
	movl	36(%esp),%esi
	movl	44(%esp),%ecx
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
	movl	%edx,%edi
	movl	%ecx,%eax
	movl	20(%esp),%edx
	movl	24(%esp),%ebx
	movl	32(%esp),%ebp
	movl	44(%esp),%ecx
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
	movl	28(%esp),%ecx
	movl	%esi,(%ecx)
	movl	%edi,4(%ecx)
	movl	36(%esp),%edi
	movl	40(%esp),%esi
	movl	48(%esp),%ecx
	movl	%edx,(%edi)
	movl	%ebx,(%esi)
	movl	%eax,(%ecx)
	andl	$0xf0,%eax
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret

