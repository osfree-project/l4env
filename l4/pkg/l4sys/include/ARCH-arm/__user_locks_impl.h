#ifndef L4SYS_USER_LOCKS_IMPL_H__
#define L4SYS_USER_LOCKS_IMPL_H__

#include <l4/sys/kdebug.h>

// dumb, however atomic sequences are defined in kdebug.h
#include <l4/sys/compiler.h>

#ifndef L4_SYSCALL_MAGIC_OFFSET
#  define L4_SYSCALL_MAGIC_OFFSET	8
#endif
#define L4_SYSCALL_U_LOCK		(-0x00000024-L4_SYSCALL_MAGIC_OFFSET)

L4_INLINE unsigned
l4_usem_down_to(unsigned long lock, unsigned long *counter, 
                l4_timeout_s timeout)
{
  if (__builtin_expect(l4_atomic_add((long int*)counter, -1) >= 0, 1))
    return 0;

  register unsigned long _lock asm ("r1") = lock;
  register unsigned long _timeout asm ("r3") = timeout.t;
  register unsigned long res asm ("r0");

  __asm__ __volatile__
    ("@ l4_u_lock (semaphore block) \n\t"
     PIC_SAVE_ASM
     "stmdb   sp!, {fp}    \n\t"
     "mov     r0, #6       \n\t"
     "mov     lr, pc       \n\t"
     "mov     pc, %[sys]   \n\t"
     "ldmia   sp!, {fp}    \n\t"
     PIC_RESTORE_ASM
     :
     "=r"(_lock),
     "=r"(_timeout),
     "=r"(res)
     :
     "0"(_lock),
     "1"(_timeout),
     [sys] "i" (L4_SYSCALL_U_LOCK)
     :
     "r2", "r4", "r5", "r6", "r7", "r8", "r9" PIC_CLOBBER, "r12", "r14"
     );

  return res;
}

L4_INLINE void
l4_usem_up(unsigned long lock, unsigned long *counter)
{
  if (__builtin_expect(l4_atomic_add((long int*)counter, 1) > 0, 1))
    return;

  register unsigned long _lock asm ("r1") = lock;

  __asm__ __volatile__
    ("@ l4_u_lock (semaphore wake) \n\t"
     PIC_SAVE_ASM
     "stmdb   sp!, {fp}    \n\t"
     "mov     r0, #7       \n\t"
     "mov     lr, pc       \n\t"
     "mov     pc, %[sys]   \n\t"
     "ldmia   sp!, {fp}    \n\t"
     PIC_RESTORE_ASM
     :
     "=r"(_lock)
     :
     [sys] "i" (L4_SYSCALL_U_LOCK),
     "0"(_lock)
     :
     "r0", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9" PIC_CLOBBER, "r12", 
     "r14"
     );
}

L4_INLINE unsigned long
l4_ulock_generic(unsigned long op, unsigned long lock)
{
  register unsigned long _op asm ("r0") = op;
  register unsigned long _lock asm ("r1") = lock;

  __asm__ __volatile__
    ("@ l4_u_lock (generic) \n\t"
     PIC_SAVE_ASM
     "stmdb   sp!, {fp}    \n\t"
     "mov     lr, pc       \n\t"
     "mov     pc, %[sys]   \n\t"
     "ldmia   sp!, {fp}    \n\t"
     PIC_RESTORE_ASM
     :
     "=r"(_op),
     "=r"(_lock)
     :
     [sys] "i" (L4_SYSCALL_U_LOCK),
     "0"(_op),
     "1"(_lock)
     :
     "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9" PIC_CLOBBER, "r12", "r14"
     );

  return _op;
}


#endif
