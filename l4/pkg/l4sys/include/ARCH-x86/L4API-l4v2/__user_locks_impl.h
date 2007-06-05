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
l4_usem_down_to(unsigned long lock, unsigned long *counter, 
                l4_timeout_s timeout)
{
  register unsigned long _lock asm ("edx") = lock;
  register unsigned long *_counter asm ("edi") = counter;
  register unsigned long _timeout asm ("ecx") = timeout.t;
  unsigned long res;

  __asm__ __volatile__(
	"decl 0(%%edi)		\n\t"
	"jge  2f		\n\t"
	PIC_ASM_SAVE
	"pushl %%ebp		\n\t"	/* save ebp, no memory references
					 ("m") after this point */
	"movl $6, %%eax		\n\t"
	L4_SYSCALL(ulock)
	"popl	 %%ebp		\n\t"	/* restore ebp, no memory references
					 ("m") before this point */
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
	"esi", "memory" PIC_CLOBBER
       );

  return res;
}

L4_INLINE void
l4_usem_up(unsigned long lock, unsigned long *counter)
{
  register unsigned long _lock asm ("edx") = lock;
  register unsigned long *_counter asm ("ecx") = counter;

  __asm__ __volatile__(
	"incl 0(%%ecx)		\n\t"
	"jg   2f		\n\t"
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
	"eax", "edi", "esi", "memory" PIC_CLOBBER
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
