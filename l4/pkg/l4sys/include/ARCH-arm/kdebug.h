#ifndef __L4SYS__INCLUDE__ARCH_ARM__KDEBUG_H__
#define __L4SYS__INCLUDE__ARCH_ARM__KDEBUG_H__

#include <l4/sys/compiler.h>

#ifdef __GNUC__

#ifndef L4_SYSCALL_MAGIC_OFFSET
#  define L4_SYSCALL_MAGIC_OFFSET	8
#endif
#define L4_SYSCALL_ENTER_KDEBUG		(-0x00000020-L4_SYSCALL_MAGIC_OFFSET)

#define enter_kdebug(text...)			\
__asm__ __volatile__ (				\
    "	mov	lr, pc		\n"		\
    "	mov	pc, %0		\n"		\
    "	b	1f		\n"		\
    "	.ascii	\"" text "\"	\n"		\
    "	.byte	0		\n"		\
    "	.align	4		\n"		\
    "1:				\n"		\
    :						\
    : "i" (L4_SYSCALL_ENTER_KDEBUG)		\
    : "lr")

L4_INLINE void
outnstring(const char* x, unsigned len);

L4_INLINE void
outstring(const char *text);

L4_INLINE void
outchar(char c);

L4_INLINE void
outdec(int number);

L4_INLINE void
outhex32(int number);

L4_INLINE void
outhex20(int number);

L4_INLINE void
outhex16(int number);

L4_INLINE void
outhex12(int number);

L4_INLINE void
outhex8(int number);

L4_INLINE void
kd_display(char *text);

L4_INLINE int
l4kd_inchar(void);

L4_INLINE void
l4_sys_cli(void);

L4_INLINE void
l4_sys_sti(void);

L4_INLINE void
l4_kdebug_cache(unsigned long op, unsigned long start, unsigned long end);

EXTERN_C long int
l4_atomic_add(volatile long int* mem, long int offset) LONG_CALL;

EXTERN_C long int
l4_atomic_cmpxchg(volatile long int* mem, long int oldval, long int newval) LONG_CALL;

EXTERN_C long int
l4_atomic_cmpxchg_res(volatile long int* mem, long int oldval, long int newval) LONG_CALL;

/*
 * -------------------------------------------------------------------
 * Implementations
 */

#define __KDEBUG_ARM_PARAM_0(nr)				\
  ({								\
    register unsigned long r0 asm("r0");			\
    __asm__ __volatile__					\
      (								\
       "stmdb sp!, {r1-r12,lr}	\n\t"				\
       "mov	lr, pc		\n\t"				\
       "mov	pc, %1		\n\t"				\
       "cmp	lr, #" #nr "	\n\t"				\
       "ldmia sp!, {r1-r12,lr}	\n\t"				\
       :							\
       "=r" (r0)						\
       :							\
       "i" (L4_SYSCALL_ENTER_KDEBUG)				\
      );							\
    r0;								\
  })

#define __KDEBUG_ARM_PARAM_1(nr, p1)				\
  ({								\
    register unsigned long r0 asm("r0") = (unsigned long)(p1);	\
    __asm__ __volatile__					\
      (								\
       "stmdb sp!, {r1-r12,lr}	\n\t"				\
       "mov	lr, pc		\n\t"				\
       "mov	pc, %1		\n\t"				\
       "cmp	lr, #" #nr "	\n\t"				\
       "ldmia sp!, {r1-r12,lr}	\n\t"				\
       :							\
       "=r" (r0)						\
       :							\
       "i" (L4_SYSCALL_ENTER_KDEBUG),				\
       "0" (r0)							\
      );							\
    r0;								\
  })

#define __KDEBUG_ARM_PARAM_2(nr, p1, p2)			\
  ({								\
    register unsigned long r0 asm("r0") = (unsigned long)(p1);	\
    register unsigned long r1 asm("r1") = (unsigned long)(p2);	\
    __asm__ __volatile__					\
      (								\
       "stmdb sp!, {r2-r12,lr}	\n\t"				\
       "mov	lr, pc		\n\t"				\
       "mov	pc, %2		\n\t"				\
       "cmp	lr, #" #nr "	\n\t"				\
       "ldmia sp!, {r2-r12,lr}	\n\t"				\
       :							\
       "=r" (r0),						\
       "=r" (r1)						\
       :							\
       "i" (L4_SYSCALL_ENTER_KDEBUG),				\
       "0" (r0),						\
       "r" (r1)							\
      );							\
    r0;								\
  })

#define __KDEBUG_ARM_PARAM_3(nr, p1, p2, p3)			\
  ({								\
    register unsigned long r0 asm("r0") = (unsigned long)(p1);	\
    register unsigned long r1 asm("r1") = (unsigned long)(p2);	\
    register unsigned long r2 asm("r2") = (unsigned long)(p3);	\
    __asm__ __volatile__					\
      (								\
       "stmdb sp!, {r3-r12,lr}	\n\t"				\
       "mov	lr, pc		\n\t"				\
       "mov	pc, %3		\n\t"				\
       "cmp	lr, #" #nr "	\n\t"				\
       "ldmia sp!, {r3-r12,lr}	\n\t"				\
       :							\
       "=r" (r0),						\
       "=r" (r1),						\
       "=r" (r2)						\
       :							\
       "i" (L4_SYSCALL_ENTER_KDEBUG),				\
       "0" (r0),						\
       "r" (r1),						\
       "r" (r2)							\
      );							\
    r0;								\
  })

#define __KDEBUG_ARM_PARAM_5(nr, p1, p2, p3, p4, p5)		\
  ({								\
    register unsigned long r0 asm("r0") = (unsigned long)(p1);	\
    register unsigned long r1 asm("r1") = (unsigned long)(p2);	\
    register unsigned long r2 asm("r2") = (unsigned long)(p3);	\
    register unsigned long r3 asm("r3") = (unsigned long)(p4);	\
    register unsigned long r4 asm("r4") = (unsigned long)(p5);	\
    __asm__ __volatile__					\
      (								\
       "stmdb sp!, {r5-r12,lr}	\n\t"				\
       "mov	lr, pc		\n\t"				\
       "mov	pc, %5		\n\t"				\
       "cmp	lr, #" #nr "	\n\t"				\
       "ldmia sp!, {r5-r12,lr}	\n\t"				\
       :							\
       "=r" (r0),						\
       "=r" (r1),						\
       "=r" (r2),						\
       "=r" (r3),						\
       "=r" (r4)						\
       :							\
       "i" (L4_SYSCALL_ENTER_KDEBUG),				\
       "0" (r0),						\
       "r" (r1),						\
       "r" (r2),						\
       "r" (r3),						\
       "r" (r4)							\
      );							\
    r0;								\
  })


L4_INLINE void
outnstring(const char* x, unsigned len)
{
  asm volatile ("" : : : "memory");
  __KDEBUG_ARM_PARAM_2(3, x, len);
}

L4_INLINE void
outstring(const char *text)
{
  asm volatile("" : : : "memory");
  __KDEBUG_ARM_PARAM_1(2, text);
}

L4_INLINE void
outchar(char c)
{
  __KDEBUG_ARM_PARAM_1(1, c);
}

L4_INLINE void
outdec(int number)
{
  __KDEBUG_ARM_PARAM_1(4, number);
}

L4_INLINE void
outhex32(int number)
{
  __KDEBUG_ARM_PARAM_1(5, number);
}

L4_INLINE void
outhex20(int number)
{
  __KDEBUG_ARM_PARAM_1(6, number);
}

L4_INLINE void
outhex16(int number)
{
  __KDEBUG_ARM_PARAM_1(7, number);
}

L4_INLINE void
outhex12(int number)
{
  __KDEBUG_ARM_PARAM_1(8, number);
}

L4_INLINE void
outhex8(int number)
{
  __KDEBUG_ARM_PARAM_1(9, number);
}

L4_INLINE void
kd_display(char *text)
{
  outstring(text);
}

L4_INLINE int
l4kd_inchar(void)
{
  return __KDEBUG_ARM_PARAM_0(0xd);
}

L4_INLINE void
l4_sys_cli(void)
{
  __KDEBUG_ARM_PARAM_0(0x32);
}

L4_INLINE void
l4_sys_sti(void)
{
  __KDEBUG_ARM_PARAM_0(0x33);
}

L4_INLINE void
l4_kdebug_cache(unsigned long op, unsigned long start, unsigned long end)
{
  __KDEBUG_ARM_PARAM_3(0x3f, op, start, end);
}

#endif //__GNUC__

#endif /* ! __L4SYS__INCLUDE__ARCH_ARM__KDEBUG_H__ */
