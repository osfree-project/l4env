#if 0
#define ABS_FUNC(s,value) \
.global s;          \
.type s, #function; \
.size s, 12;        \
s = value

ABS_FUNC(l4_atomic_add, 0xffffe000)
ABS_FUNC(l4_atomic_cmpxchg, 0xffffe100)
ABS_FUNC(l4_atomic_cmpxchg_res, 0xffffe200)
#endif


/* Using the above gives problems when using it in shared libs
 * due to relocation which actually shouldn't happen */

#define JUMP_CODE(name)			\
	.global name;			\
	name:				\
		ldr	pc, .LC##name;	\
		/* Will return to our caller */

JUMP_CODE(l4_atomic_add)
JUMP_CODE(l4_atomic_cmpxchg)
JUMP_CODE(l4_atomic_cmpxchg_res)
	

.LCl4_atomic_add:         .word 0xffffe000
.LCl4_atomic_cmpxchg:     .word 0xffffe100
.LCl4_atomic_cmpxchg_res: .word 0xffffe200
