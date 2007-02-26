
	.section .rospace, "a", @nobits
	.align	4096
	.size	io_info, 4096
	.type	io_info, @object
	.globl	io_info
io_info:
	.space	4

	.size	jiffies, 4
	.globl	jiffies
	.weak	jiffies
jiffies:
	.space	4

	.size	HZ, 4
	.globl	HZ
	.weak	HZ
HZ:
	.space	4

	.size	xtime, 8
	.globl	xtime
	.weak	xtime
xtime:
	.space	8

	.space	4096-(4+4+4+8)

