#ifndef LOCAL_IPC_ASM_H
#define LOCAL_IPC_ASM_H

#include "config_tcbsize.h"
#include "shortcut.h"

/* current utcb, current ktcb and location of the LIPC-KIP */
#define LIPC_KIP_ADDR	(user_kip_location)



/*
  Calculates the utcb pointer from a given local id given in reg
*/
.macro	LOCALID_TO_UTCB reg
	shl	$SHIFT__UTCB, \reg
	addl	$VAL__V2_UTCB_ADDR, \reg
.endm

/*
  Calculates the local id from the utcb Pointer
*/
.macro	UTCB_TO_LOCALID reg
	subl	$VAL__V2_UTCB_ADDR, \reg
	shr	$SHIFT__UTCB, \reg
.endm

// shortcut macros

// touches edx, %eax
.macro	UTCB_SENDER_OK	this_ptr, dest_ptr, this_sender_ptr, sender_ok
  //	movl    OFS__THREAD__UTCB_PTR(DST_esi), %edx

  	movl    OFS__THREAD__UTCB_PTR(\dest_ptr), %edx


	// if(state() & Thread_lipc_ready)
	testl	$Thread_lipc_ready, OFS__THREAD__STATE(\dest_ptr)
	jz	2f

	movl    OFS__UTCB__STATUSWORD(%edx), %eax

	/* mask out lock and no_lipc ready bit in the UTCB-status */
	andl    $VAL__UTCB_ADDR_MASK, %eax

	//if (utcb()->open_wait())
	testl	$VAL__UTCB_STATE_OPENWAIT, %eax
	jz	1f /* not set */
	jmp	\sender_ok

	//(space_context() == addr_space)
1:	movl	OFS__THREAD__SPACE_CONTEXT(\dest_ptr), %edx
	movl	OFS__THREAD__LOCAL_ID(\this_ptr), %ecx

	cmpl	OFS__THREAD__SPACE_CONTEXT(\this_ptr), %edx
	jne	3f

	//(id == (utcb()->partner())
	cmpl	%eax, %ecx
	jne	3f
	jmp	\sender_ok

	//else correct values still in the KTCB
	
2:	
	movl    OFS__THREAD__PARTNER (\dest_ptr), %edx

	// if DEST_esi->partner() == 0, openwait
	testl	%edx, %edx
	jne	1f
	jmp	\sender_ok

1:
	// if DEST_esi->partner() == this, wait for me
	cmpl	\this_sender_ptr, %edx
	jne	3f
	jmp	\sender_ok
3:
.endm


// touches ecx and edx
.macro	SET_SRC_LOCAL_ID src_thread, dst_thread, regs_ptr

	movl    OFS__THREAD__SPACE_CONTEXT (\src_thread), %ecx
	cmpl    OFS__THREAD__SPACE_CONTEXT (\dst_thread), %ecx

	movl	$0xffffffff, %edx
	jne     5f

	movl    OFS__THREAD__LOCAL_ID(\src_thread), %edx
	UTCB_TO_LOCALID	%edx
	
5:	movl	%edx, REG_ECX(\regs_ptr)
.endm

// scratches %edi, %edx, %ecx, %ebp, needs %esp point to the right place
.macro RELOAD_EIP_ESP_FROM_UTCB dst_thread
        testl   $Thread_utcb_ip_sp,  OFS__THREAD__STATE(\dst_thread)
        jz      1f

	// load utcb ptr
	movl    OFS__THREAD__UTCB_PTR(\dst_thread), %ecx

        andl    $(~Thread_utcb_ip_sp),  OFS__THREAD__STATE(\dst_thread)
        // we need to reload the eip and esp from the utcb

	// load from utcb
        movl    OFS__UTCB__EIP(%ecx), %edx
	// store it in the entry frame
        movl    %edx, REG_EIP(%esp)


        movl    OFS__UTCB__ESP(%ecx), %edx
        movl    %edx, REG_ESP(%esp)
1:
.endm

// scratches %edi, %edx, %ecx, %ebp
.macro STORE_EIP_ESP_IN_UTCB dst_thread
        orl   $Thread_utcb_ip_sp,  OFS__THREAD__STATE(\dst_thread)
	
	// load utcb ptr
	movl    OFS__THREAD__UTCB_PTR(\dst_thread), %ecx

	// load from stack
        movl    REG_EIP(%esp), %edx
	// store it in the utcb
        movl    %edx, OFS__UTCB__EIP(%ecx)

	// load from stack
        movl    REG_ESP(%esp), %edx
	// store it in the utcb
        movl    %edx, OFS__UTCB__ESP(%ecx)
.endm



#endif
