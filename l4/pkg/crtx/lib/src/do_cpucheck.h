
/*
 * Check the CPU type. 
 */

	.section .cpucheck,"a",@nobits
	
	.weak	__cpu_model_4
	.weak	__cpu_model_5
	.weak	__cpu_model_6
	.weak	__cpu_model_7

	.text

	/* save spilled registers */
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx

	/* check if cpuid instruction is available by trying to
	 * change the cpuid detection flag in the eflags */
	pushfl
	popl	%eax
	movl	%eax,%ebx
	xorl	$0x200000,%eax
	pushl	%eax
	popfl
	pushfl
	popl	%eax
	cmp	%eax,%ebx

	/* flag changeable? no => i386 */
	mov	$3,%ah
	jz	3f

	/* check cpuid level */
	movl	$0,%eax
	cpuid

	/* cpuid level less than 1? yes => i486 */
	cmpl	$0,%eax
	movb	$4,%ah
	jz	3f

	/* get model */
	movl	$1,%eax
	cpuid
	and	$0x0f,%ah

3:	/* check required model */
	movb	$3,%al		/* we need at least model 3 */

34:	cmpl	$__cpu_model_4,%ecx
	je	35f
	movb	$4,%al		/* we need at least model 4 */

35:	cmpl	$__cpu_model_5,%ecx
	je	36f
	movb	$5,%al		/* we need at least model 5 */
	
36:	cmpl	$__cpu_model_6,%ecx
	je	37f
	movb	$6,%al		/* we need at least model 6 */
	
37:	cmpl	$__cpu_model_7,%ecx
	je	38f
	movb	$7,%al		/* we need at least model 7 */
	
38:	cmpb	%al,%ah
	jae	9f

30:	/* available model < needed model */
	movb	$100,%ah
	mulb	%ah
	andl	$0x0000ffff,%eax
	add	$86,%eax

4:	int	$3
	nop
	jmp	5f
	.ascii	"This program requires "
5:	int	$3
	nop
	jmp	6f
	.ascii	"at least an i"
6:	int	$3
	cmpb	$11,%al
	int	$3
	nop
	jmp	7f
	.ascii	" processor!\r\n\0"
7:	int	$3
	jmp	8f
	.ascii	" PANIC "

8:	jmp	4b

9:	/* restore spilled registers */
	pop	%edx
	pop	%ecx
	pop	%ebx
	pop	%eax

