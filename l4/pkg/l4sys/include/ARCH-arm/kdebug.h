#ifndef L4_KDEBUG_H
#define L4_KDEBUG_H

#include <l4/sys/compiler.h>

#ifndef L4_SYSCALL_MAGIC_OFFSET 
#  define L4_SYSCALL_MAGIC_OFFSET 	8
#endif
#define L4_SYSCALL_ENTER_KDEBUG 	(-0x00000020-L4_SYSCALL_MAGIC_OFFSET)

#define enter_kdebug(text...)			\
__asm__ __volatile__ (				\
    "	mov	lr, pc		\n"		\
    "	mov	pc, %0		\n"		\
    "	b	1f		\n"		\
    "	.ascii	\"" text "\"	\n"		\
    "	.byte	0		\n"		\
    "	.align	2		\n"		\
    "1:				\n"		\
    :						\
    : "i" (L4_SYSCALL_ENTER_KDEBUG)		\
    : "lr")

L4_INLINE void 
outnstring(const char* x, unsigned len);

L4_INLINE void 
outnstring(const char* x, unsigned len)
{
  register unsigned r0 asm("r0") = (unsigned)x;
  register unsigned r1 asm("r1") = len;
  __asm__ __volatile__ 
    (
     "stmdb sp!, {r0-r12,lr} \n\t"
     "	mov	lr, pc		\n"
     "	mov	pc, %2		\n"
     "	cmp	lr, #3		\n"
     "ldmia sp!, {r0-r12,lr} \n\t"
     : 
     "=r"(r0),
     "=r"(r1)
     : 
     "i" (L4_SYSCALL_ENTER_KDEBUG), 
     "0"(r0), 
     "1"(r1)
     : 
     "r2","r3","r4","r5","r6","r7",
     "r8","r9","r10","r12","r14"
     );
}


#endif /* L4_KDEBUG_H */
