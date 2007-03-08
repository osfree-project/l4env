/* generate uniform stack layout before calling exception handlers
 * (some have an error code on the stack, some don't) */

#define __ASSEMBLY__
#include "ptrace.h" /* ptregs , offsets */
//#include <linux/linkage.h>
#include <l4/sys/compiler.h>


.file "int_entry.S"
.text


	.globl regs
	.globl idt_thread_id
	
	
/* divide error */
entry0:	
    	pushl	$0		/* push missing error code */
	pushl	$0
	jmp	__handle_generalized_exception

/* debug exception */
entry1:
    	pushl	$0		/* push missing error code */
	pushl	$1
	jmp	__handle_generalized_exception

/* int 2 nmi */
entry2:
	pushl	$0		/* push missing error code */
	pushl	$2
	jmp	__handle_generalized_exception

/* breakpoint */
entry3:
	pushl	$0		/* push missing error code */
	pushl	$3
	jmp	__handle_generalized_exception

/* overflow */
entry4:
	pushl	$0		/* push missing error code */
	pushl	$4
	jmp	__handle_generalized_exception

/* boundscheck */
entry5:
	pushl	$0		/* push missing error code */
	pushl	$5
	jmp	__handle_generalized_exception

/* invalid opcode */
entry6:
	pushl	$0		/* push missing error code */
	pushl	$6
	jmp	__handle_generalized_exception

/* int 7 */
entry7:
	pushl	$0		/* push missing error code */
	pushl	$7
	jmp	__handle_generalized_exception

/* double fault */
entry8:
	pushl	$8
	jmp	__handle_generalized_exception

/* coprocessor segment overrun */
entry9:
	pushl	$0		/* push missing error code */
	pushl	$9
	jmp	__handle_generalized_exception

/* invalid tss */
entry10:
	pushl	$10
	jmp	__handle_generalized_exception

/* segment not present */
entry11:
	pushl	$11
	jmp	__handle_generalized_exception

/* stack exception */
entry12:
	pushl	$12
	jmp	__handle_generalized_exception

/* general protection */
entry13:
	pushl	$13
	jmp	__handle_generalized_exception
	
/* page fault */
entry14:
	pushl	$14
	jmp	__handle_generalized_exception

/* int 15 */
entry15:
	pushl	$15
	jmp	__handle_generalized_exception


/* coprocessor error */
entry16:
	pushl	$0		/* push missing error code */
	pushl	$16
	jmp	__handle_generalized_exception

entry17:
	pushl	$0		/* push missing error code */
	pushl	$17
	jmp	__handle_generalized_exception

entry18:
	pushl	$0		/* push missing error code */
	pushl	$18
	jmp	__handle_generalized_exception

entry19:
	pushl	$0		/* push missing error code */
	pushl	$19
	jmp	__handle_generalized_exception


__handle_generalized_exception:	

	movl	%eax,regs+(ORIG_EAX*4)
	movl	%eax,regs+(EAX*4)
	movl 	%ebx,regs+(EBX*4)
	movl	%ecx,regs+(ECX*4)
	movl	%edx,regs+(EDX*4)
	movl	%esi,regs+(ESI*4)
	movl 	%edi,regs+(EDI*4)
	movl	%ebp,regs+(EBP*4)
	
	popl	%eax
	movl	%eax,regs+(EXCEPTION_NUMBER*4)
	
/* skipped segment regs in LINUX_ON_L4! */		
	
/* errorcode, eip, cs and EFL are on the stack */
	movl	(%esp),%eax
	movl	4(%esp),%ebx
	movl	%eax,regs+(ERROR_CODE*4)
	movl	%ebx,regs+(EIP*4)
	movl	8(%esp),%eax
	movl	12(%esp),%ebx
	movl	%eax,regs+(CS*4)
	movl	%ebx,regs+(EFL*4)

/* remove exception frame and save _user_ esp in ptregs*/

	addl	$16, %esp
	movl 	%esp,regs+(UESP*4)
    
	xorl	%eax, %eax
	xorl	%ecx, %ecx
	xorl	%edx, %edx
	xorl	%ebx, %ebx
	xorl	%ebp, %ebp
	movl	idt_thread_id, %esi
	movl	idt_thread_id+4, %edi

    	int	$0x30
/*
 * write everything back from ptregs to registers
 * and simulate iret. Note that return value from syscall
 * is in ptregs.eax!
 * esp is set back to user esp (saved in ptregs)
 */


/* restore_regs function.
   restores content of registers from saved copy within
   exclusive page and uses a previously set up stack frame to return
*/
/*
 * write everything back from ptregs to registers
 * and simulate iret. Note that return value from syscall
 * is in ptregs.eax!
 * esp is set back to user esp (saved in ptregs)
 */

__restore_regs:	

 	movl	regs+(UESP*4), %esp
    	movl	regs+(ECX*4),%ecx

/*	subl	$12, %esp
*/
	pushl   regs+(EFL*4)
	pushl	regs+(CS*4)
	pushl	regs+(EIP*4)

    	movl	regs+(EDX*4),%edx

    	movl	regs+(ESI*4),%esi
	movl 	regs+(EDI*4),%edi

      	movl	regs+(EBP*4),%ebp
	movl 	regs+(EBX*4),%ebx
    
 	movl	regs+(EAX*4),%eax

	iret


    .data
    .align  4
    .globl  idt_address_table
idt_address_table:  
    .long entry0
    .long entry1
    .long entry2
    .long entry3
    .long entry4
    .long entry5
    .long entry6
    .long entry7
    .long entry8
    .long entry9
    .long entry10
    .long entry11
    .long entry12
    .long entry13
    .long entry14
    .long entry15
    .long entry16
    .long entry17
    .long entry18
    .long entry19

