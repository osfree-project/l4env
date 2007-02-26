#include <setjmp.h>

.text
.weak setjmp
.type setjmp,@function
setjmp:
.globl __setjmp
.type __setjmp,@function
__setjmp:
	movl 4(%esp), %eax

	/* Save registers.  */
	movl %ebx, (JB_BX*4)(%eax)
	movl %esi, (JB_SI*4)(%eax)
	movl %edi, (JB_DI*4)(%eax)
	movl %ebp, (JB_BP*4)(%eax)
	leal 4(%esp), %edx	/* Save SP as it will be after we return.  */
	movl 0(%esp), %ecx	/* Save PC we are returning to now.  */
	movl %edx, (JB_SP*4)(%eax)
	movl %ecx, (JB_PC*4)(%eax)
	xor %eax,%eax
	ret
	
.size __setjmp,.-__setjmp;



.text
.global longjmp
.type longjmp,@function
longjmp:
        movl 4(%esp), %ecx      /* User's jmp_buf in %ecx.  */
        movl 8(%esp), %eax      /* Second argument is return value.  */
        /* Save the return address now.  */
        movl (JB_PC*4)(%ecx), %edx
        /* Restore registers.  */
        movl (JB_BX*4)(%ecx), %ebx
        movl (JB_SI*4)(%ecx), %esi
        movl (JB_DI*4)(%ecx), %edi
        movl (JB_BP*4)(%ecx), %ebp
        movl (JB_SP*4)(%ecx), %esp
        /* Jump to saved PC.  */
        jmp *%edx
.size longjmp,.-longjmp;
