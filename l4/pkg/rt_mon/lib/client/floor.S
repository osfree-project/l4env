.text

.global own_floor
	.type own_floor,@function

own_floor:
	fldl 4(%esp)
	movb $0x04,%ah
	xorl %ecx,%ecx
	movb %ah,%ch
	pushl %eax
	fstcw (%esp)
	movw (%esp),%ax
	andb $0x03,%ah
	orl %ecx,%eax
	movw %ax,2(%esp)
	fldcw 2(%esp)
	frndint
	fldcw (%esp)
	popl %eax
	ret

.Lende:
.size    own_floor,.Lende-own_floor
