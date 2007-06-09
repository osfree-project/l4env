#ifndef L4SYS_USER_LOCKS_IMPL_H__
#define L4SYS_USER_LOCKS_IMPL_H__

#include <l4/sys/syscall-invoke.h>

#ifdef __PIC__
# define PIC_ASM_SAVE    "pushl %%ebx \n\t"
# define PIC_ASM_RESTORE "popl  %%ebx \n\t"
# define PIC_CLOBBER
#else
# define PIC_ASM_SAVE
# define PIC_ASM_RESTORE
# define PIC_CLOBBER , "ebx"
#endif


L4_INLINE unsigned
l4_usem_down_to(unsigned long ksem, l4_u_semaphore_t *sem, l4_timeout_s timeout)
{
  register unsigned long _lock asm ("edx") = ksem;
  register l4_u_semaphore_t *_counter asm ("esi") = sem;
  register unsigned long _timeout asm ("ecx") = timeout.t;
  unsigned long res;

  __asm__ __volatile__(
        "1: xorl %%eax, %%eax	\n\t"
	"   decl 0(%%esi)	\n\t"
	"   jge  2f		\n\t"
	PIC_ASM_SAVE
	"   pushl %%ebp		\n\t"	/* save ebp, no memory references
					 ("m") after this point */
	"   movl $6, %%eax	\n\t"
	L4_SYSCALL(ulock)
	"   popl	 %%ebp	\n\t"	/* restore ebp, no memory references
					 ("m") before this point */
	"   cmpl $1, %%eax	\n\t"
	"   je 1b		\n\t"
	PIC_ASM_RESTORE
	"2:			\n\t"
       :
	"=r" (_lock),
	"=r" (_counter),
	"=r" (_timeout),
	"=a" (res)
       :
	"0" (_lock),
	"1" (_counter),
	"2" (_timeout)
       :
	"edi", "memory" PIC_CLOBBER
       );

  return res;
}

L4_INLINE void
l4_usem_up(unsigned long ksem, l4_u_semaphore_t *sem)
{
  register unsigned long _lock asm ("edx") = ksem;
  register l4_u_semaphore_t *_counter asm ("esi") = sem;

  __asm__ __volatile__(
	"incl 0(%%esi)		\n\t"
	"testb $1, 4(%%esi)	\n\t"
	"jz   2f		\n\t"
	PIC_ASM_SAVE
	"pushl %%ebp		\n\t"	/* save ebp, no memory references
					 ("m") after this point */
	"movl $7, %%eax		\n\t"
	L4_SYSCALL(ulock)
	"popl	 %%ebp		\n\t"	/* restore ebp, no memory references
					 ("m") before this point */
	PIC_ASM_RESTORE
	"2:			\n\t"
       :
	"=r" (_lock),
	"=r" (_counter)
       :
	"0" (_lock),
	"1" (_counter)
       :
	"eax", "ecx", "edi", "memory" PIC_CLOBBER
       );

  return;
}


L4_INLINE unsigned long
l4_ulock_generic(unsigned long op, unsigned long lock)
{
register unsigned long _op asm ("eax") = op;
register unsigned long _lock asm ("edx") = lock;

__asm__ __volatile__(
	PIC_ASM_SAVE
	"pushl %%ebp		\n\t"	/* save ebp, no memory references
					 ("m") after this point */
	L4_SYSCALL(ulock)
	"popl	 %%ebp		\n\t"	/* restore ebp, no memory references
					 ("m") before this point */
	PIC_ASM_RESTORE
       :
	"=r" (_op),
	"=r" (_lock)
       :
	"0" (_op),
	"1" (_lock)
       :
	"ecx", "edi", "esi" PIC_CLOBBER
       );

return _op;
}

#endif
