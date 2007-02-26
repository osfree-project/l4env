#include "asm.h"
#include "config_gdt.h"
#include "idt_init.h"
#include "local_ipc_asm.h"
#include "low_level.h"
#include "tcboffset.h"
	
/*
	To get a idea how it works:
	When a thread goes to the IPC wait state, the ipc code writes in the
	UTCB of	this thread, i m waiting on X, or i m in openwait.

	When a Thread A wants to make a LIPC to B, then it checks B's UTCB,
	if B waits for A or B is in Openwait.
	If yes, then A will proceed with LIPC and if not, then it will
	use the normal kernel IPC path.

	How the code works:
	First the LIPC code checks if we are using fpu, if yes -> kernel,
	then if B waits for me, or B is in a openwait.
	This code is called the restart block.
	If a timer interrupt occurs in this block, and doing some stuff with
	threads, then the EIP of this thread is reset to the start of the 
	LIPC code, to do the check again.
	This is necessary because some status of B may have be changed.
	
	If this check says ok, then then the code goes to the forward block.
	If a timer interrupt occurs in this block, the kernel finishes the LIPC,
	setting the state of the source & target thread and then does the 
	real work.
	To help the kernel and for code simplification, i don't want to have a
	big switch, that identifies what to do for every valid eip. The kernel 
	checks the register ecx (current UTCB), eax (target UTCB), and before
	_asm_recv_desc_invalid also esi, it contains the receiving state of
	source thread, openwait or closed wait.

	The fixup code can be found in shared/thread_ia32-ux-v2-lipc.cpp

	To help a currently running thread to find its UTCB, a pointer
	to the current UTCB is saved in a fixed memory location, accessible
	via gs:	0, updated on every contextswitch.
*/

	
/* TODO: replacement of %gs:0 with a fixed address can speed up the LIPC path */

.globl _asm_lipc_code_start
.globl _asm_lipc_code_end

.align 8	
_asm_lipc_code_start:

	/* load current UTCB ptr */
	 movl	VAL__CURRENT_UTCBPTR, %ecx

	/* calc target UTCB ptr */
	LOCALID_TO_UTCB	%eax

	/* calc ipc partner UTCB address */
	LOCALID_TO_UTCB	%esi

_asm_restart_point:

	movl	OFS__UTCB__STATUSWORD(%eax), %ebp

	/* local send allowed? */
	cmpl	$0, OFS__UTCB__SND_STATUS(%ecx)
	/*
	hack to avoid the unfair send_queue bug:
	someone is waiting for us, so we dont need the automatic
   	switch_to to the receiver. because the receiver can do
	LIPC again, and then do_receive gets never called, because
	the stack gets overwritten
	*/
	jne	local_ipc_failed_deceit

	/* wait for me? */
	cmpl	%ecx, %ebp
	je	2f

	/* open wait test
	we want the openwait, the lock & fpu bit.
	This can be optimized to a single compare,
	if the receiver stores always 
	(_v2_utcb_start_addr + UTCB_ASM_STATE_OPENWAIT)
	in the UTCB.
	*/
	andl	$(VAL__UTCB_STATE_OPENWAIT | ~VAL__UTCB_ADDR_MASK), %ebp
	cmpl	$(VAL__UTCB_STATE_OPENWAIT), %ebp

	jne	local_ipc_failed

2:
	/* store the eip my UTCB */
	movl	%edi, OFS__UTCB__EIP(%ecx)
	/* store the esp in my UTCB */
	movl	%esp, OFS__UTCB__ESP(%ecx)

	/* the kernel needs eax, ecx and for the beginning esi */
_asm_forward_point:
	/* set the rcv descriptor supplied in esi to my ipc state */
	movl	%esi, OFS__UTCB__STATUSWORD(%ecx)	

	/* switch stack */
        movl	OFS__UTCB__ESP(%eax), %esp

        /* set the target runable, can be optimized away */
	movl	$VAL__UTCB_STATE_NO_LIPC, OFS__UTCB__STATUSWORD(%eax)
	
	/* ok the kernel knows here my status word is set,
	and i want to touch esi */	
_asm_rcv_desc_invalid:
	
	/* compatibility stuff, 
	old code wants the source 64bit v2 id in edi:esi */
        movl	(OFS__UTCB__UTCB_ID+4)(%ecx), %edi
	movl	OFS__UTCB__UTCB_ID(%ecx), %esi
	
	/* preload the new eip */
	movl	OFS__UTCB__EIP(%eax), %ebp

	/* ok switching to the new thread
	setting the UTCB_ptr to the new UTCB */
        movl	%eax, VAL__CURRENT_UTCBPTR


_asm_finish_point:
	/* fixme, compat. stuff, "old" code wants a result */
        movl	$RETURN_DOPE, %eax

	/* set the source */
	UTCB_TO_LOCALID	%ecx

	/* ok jump to the preloaded eip */
	jmp	*%ebp 




local_ipc_failed: 

	/* fake call entry, pushing the supplied eip */
        pushl	%edi

	/* no openwait */
        xorl	%ebp, %ebp /* rcv desc for int30 */
	
	/* openwait? */
	testl	$VAL__UTCB_STATE_OPENWAIT, %esi
	jz	3f
	
	/* ok thread wants openwait */	
	movl	$0x1, %ebp

3:	movl	OFS__UTCB__UTCB_ID(%eax), %esi 
        movl	(OFS__UTCB__UTCB_ID+4)(%eax), %edi 
 
        movl	$0, %eax   /* send desc*/ 
        movl	$0, %ecx /* timeout*/ 
                         
        int	$0x30
	ret

/* disable switch_to in ipc_unlock in the kernel IPC path
   proceed directly to do_receive() */
local_ipc_failed_deceit: 

	/* fake call entry, pushing the supplied eip */
        pushl	%edi

	/* no openwait */
        xorl	%ebp, %ebp /* rcv desc for int30 */
	
	/* openwait? */
	testl	$VAL__UTCB_STATE_OPENWAIT, %esi
	jz	3f
	
	/* ok thread wants openwait */	
	movl	$0x1, %ebp
 
3:	movl	OFS__UTCB__UTCB_ID(%eax), %esi 
        movl	(OFS__UTCB__UTCB_ID+4)(%eax), %edi 
 
        movl	$0x1, %eax   /* send desc*/ 
        movl	$0, %ecx /* timeout*/ 
                         
        int	$0x30
	ret

local_ipc_end:


_asm_lipc_code_end:


/* some offsets for the fixupcode */
_lipc_restart_point_offset = (_asm_restart_point	- _asm_lipc_code_start)
_lipc_forward_point_offset = (_asm_forward_point	- _asm_lipc_code_start)
_lipc_rcv_desc_invalid    = (_asm_rcv_desc_invalid	- _asm_lipc_code_start)
_lipc_finish_point_offset  = (_asm_finish_point		- _asm_lipc_code_start)

.globl	_lipc_forward_point_offset
.globl	_lipc_restart_point_offset;
.globl	_lipc_rcv_desc_invalid 
.globl  _lipc_finish_point_offset

/* the dummy iret routine which is called from the faked kernel ipc stack */
.globl _asm_user_invoke_from_localipc

/* stacklayout should be this
	SS		44
	ESP		40
	EFLAGS		36
	CS		32
	EIP		28	

	EAX	24
	EBP	20
	EBX	16
	EDI	12
	ESI	 8
	EDX	 4
	ECX	 0
*/

_asm_user_invoke_from_localipc:
	ESP_TO_TCB_AT %ebx
	RESET_THREAD_IPC_MASK_AT %ebx
	RELOAD_EIP_ESP_FROM_UTCB %ebx	/* scratches %edi, %edx, %ecx, %ebp */
	CHECK_SANITY $3			/* scratches ecx */
	RESET_USER_SEGMENTS $3,in_cli	// scratches ecx
	RESTORE_STATE
	popl				%eax
	iret
