#ifndef L4SYS_USER_LOCKS_IMPL_H__
#define L4SYS_USER_LOCKS_IMPL_H__

# define L4_SYSCALL(name)                 L4_SYSCALL_ ## name
# define L4_SYSCALL_ulock                 "int $0x39 \n\t"

#ifdef __PIC__
# define PIC_ASM_SAVE    "push %%rbx \n\t"
# define PIC_ASM_RESTORE "pop  %%rbx \n\t"
# define PIC_CLOBBER
#else
# define PIC_ASM_SAVE
# define PIC_ASM_RESTORE
# define PIC_CLOBBER , "rbx"
#endif


L4_INLINE unsigned
l4_usem_down_to(unsigned long lock, l4_u_semaphore_t *sem, l4_timeout_s timeout)
{
  register unsigned long _lock asm ("rdx") = lock;
  register l4_u_semaphore_t *_counter asm ("rcx") = sem;
  register unsigned long _timeout asm ("rdi") = timeout.t;
  unsigned long res;

  __asm__ __volatile__(
        "1: xor %%rax, %%rax	\n\t"
	"   decq 0(%%rcx)	\n\t"
	"   jge  2f		\n\t"
	PIC_ASM_SAVE
	"   push %%rbp		\n\t"	/* save ebp, no memory references
				 ("m") after this point */
	"   mov $6, %%rax	\n\t"
	L4_SYSCALL(ulock)
	"   pop	 %%rbp		\n\t"	/* restore ebp, no memory references
					 ("m") before this point */
	"   cmpq $1, %%rax	\n\t"
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
	"rsi", "memory" PIC_CLOBBER
       );

  return res;
}

L4_INLINE void
l4_usem_up(unsigned long lock, l4_u_semaphore_t *sem)
{
  register unsigned long _lock asm ("rdx") = lock;
  register l4_u_semaphore_t *_counter asm ("rcx") = sem;

  __asm__ __volatile__(
	"incq 0(%%rcx)		\n\t"
	"testb $1, 8(%%rcx)	\n\t"
	"jz   2f		\n\t"
	PIC_ASM_SAVE
	"push %%rbp		\n\t"	/* save ebp, no memory references
					 ("m") after this point */
	"movq $7, %%rax		\n\t"
	L4_SYSCALL(ulock)
	"pop	 %%rbp		\n\t"	/* restore ebp, no memory references
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
	"rax", "rdi", "rsi", "memory" PIC_CLOBBER
       );

  return;
}


L4_INLINE unsigned long
l4_ulock_generic(unsigned long op, unsigned long lock)
{
register unsigned long _op asm ("rax") = op;
register unsigned long _lock asm ("rdx") = lock;

__asm__ __volatile__(
	PIC_ASM_SAVE
	"push %%rbp		\n\t"	/* save ebp, no memory references
					 ("m") after this point */
	L4_SYSCALL(ulock)
	"pop	 %%rbp		\n\t"	/* restore ebp, no memory references
					 ("m") before this point */
	PIC_ASM_RESTORE
       :
	"=r" (_op),
	"=r" (_lock)
       :
	"0" (_op),
	"1" (_lock)
       :
	"rcx", "rdi", "rsi" PIC_CLOBBER
       );

return _op;
}

#endif
