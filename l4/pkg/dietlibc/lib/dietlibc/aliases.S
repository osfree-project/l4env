
#ifdef ARCH_arm
#define JUMP b
#elif defined(ARCH_x86) || defined(ARCH_amd64)
#define JUMP jmp
#else
#error Define JUMP instruction
#endif

#define poor_weak_alias(wsym,sym)		\
	.text					;\
	.type wsym,%function			;\
	.weak sym				;\
	.global wsym				;\
	wsym:					;\
		JUMP sym			;\
	.previous;

poor_weak_alias(__libc_open,  open)
poor_weak_alias(__libc_read,  read)
poor_weak_alias(__libc_write, write)
poor_weak_alias(__libc_close, close)
poor_weak_alias(if_freenameindex, free)
