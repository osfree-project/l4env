
	.section .rospace, "a", @nobits
	.align	4096
	.size	kip_area, 4096
	.type	kip_area, %object
	.globl	kip_area
kip_area:
	.space	4096

