
#define ASSEMBLER

	/* dummy functions to make the linker happy, they are never used */
	.globl	leave_from_sysenter_by_iret
leave_from_sysenter_by_iret:
	.globl	do_sysenter
do_sysenter:
	.globl	do_sysenter_log
do_sysenter_log:
	.globl	do_sysenter_c
do_sysenter_c:


