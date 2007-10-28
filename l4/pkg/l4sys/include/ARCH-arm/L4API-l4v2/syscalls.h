/*!
 * \file    l4sys/include/ARCH-arm/L4API-l4v2/syscalls.h
 * \brief   Syscalls for ARM architecture.
 * \ingroup api_calls
 */
#ifndef __L4SYS__INCLUDE__ARCH_ARM__L4API_L4V2__SYSCALLS_H__
#define __L4SYS__INCLUDE__ARCH_ARM__L4API_L4V2__SYSCALLS_H__

#include <l4/sys/syscalls_gen.h>

#ifndef L4_SYSCALL_MAGIC_OFFSET
#  define L4_SYSCALL_MAGIC_OFFSET	8                                        ///< syscall offset
#endif
#define L4_SYSCALL_ID_NEAREST		(-0x00000008-L4_SYSCALL_MAGIC_OFFSET)    ///< id_nearest syscall entry
#define L4_SYSCALL_FPAGE_UNMAP		(-0x0000000C-L4_SYSCALL_MAGIC_OFFSET)    ///< fpage_unamp syscall entry
#define L4_SYSCALL_THREAD_SWITCH	(-0x00000010-L4_SYSCALL_MAGIC_OFFSET)    ///< thread_switch syscall entry
#define L4_SYSCALL_THREAD_SCHEDULE	(-0x00000014-L4_SYSCALL_MAGIC_OFFSET)    ///< thread_schedule syscall entry
#define L4_SYSCALL_LTHREAD_EX_REGS	(-0x00000018-L4_SYSCALL_MAGIC_OFFSET)    ///< lthread_ex_regs syscall entry
#define L4_SYSCALL_TASK_NEW		(-0x0000001C-L4_SYSCALL_MAGIC_OFFSET)    ///< task_new syscall entry

L4_INLINE int l4_nchief(l4_threadid_t destination,
                        l4_threadid_t *next_chief)
{
  register l4_umword_t destid_type asm("r0") = destination.raw;
  register l4_threadid_t next      asm("r1");

  __asm__ __volatile__
    (PIC_SAVE_ASM
     "stmdb   sp!, {fp}	\n\t"
     "mov     lr, pc	\n\t"
     "mov     pc, %2	\n\t"
     "ldmia   sp!, {fp}	\n\t"
     PIC_RESTORE_ASM
     :
     "=r"(destid_type),
     "=r"(next)
     :
     "i"(L4_SYSCALL_ID_NEAREST),
     "0"(destid_type)
     :
     "r2", "r3", "r4", "r5", "r6", "r7",
     "r8", "r9", "r12", "r14", "memory" PIC_CLOBBER
     );

  next_chief->raw = next.raw;

  return destid_type;
}

L4_INLINE l4_threadid_t l4_myself()
{
  register l4_umword_t nil_id asm("r0") = L4_NIL_ID.raw;
  register l4_threadid_t id   asm("r1");

  __asm__ __volatile__
    (PIC_SAVE_ASM
     "stmdb   sp!, {fp}	\n\t"
     "mov     lr, pc	\n\t"
     "mov     pc, %2	\n\t"
     "ldmia   sp!, {fp}	\n\t"
     PIC_RESTORE_ASM
     :
     "=r"(nil_id),
     "=r"(id)
     :
     "i"(L4_SYSCALL_ID_NEAREST),
     "0"(nil_id)
     :
     "r2", "r3", "r4", "r5", "r6", "r7",
     "r8", "r9" PIC_CLOBBER, "r12", "r14", "memory"
     );

#ifdef __cplusplus
  return l4_threadid_t::_convert(id);
#else
  return id;
#endif
}

L4_INLINE void
l4_fpage_unmap(l4_fpage_t fpage, l4_umword_t mask)
{
  register l4_umword_t _fpage asm("r0") = fpage.raw;
  register l4_umword_t _mask  asm("r1") = mask;

  __asm__ __volatile__
    ("@ l4_fpage_unmap	 \n\t"
     PIC_SAVE_ASM
     "stmdb   sp!, {fp}  \n\t"
     "mov     lr, pc     \n\t"
     "mov     pc, %2     \n\t"
     "ldmia   sp!, {fp}  \n\t"
     PIC_RESTORE_ASM
     :
     "=r"(_fpage),
     "=r"(_mask)
     :
     "i"(L4_SYSCALL_FPAGE_UNMAP),
     "0"(_fpage),
     "1"(_mask)
     :
     "r2", "r3", "r4", "r5", "r6", "r7",
     "r8", "r9" PIC_CLOBBER, "r12", "r14"
     );
}

L4_INLINE void
l4_thread_switch(l4_threadid_t dest)
{
  register l4_umword_t _dest asm("r0") = dest.raw;
  __asm__ __volatile__
    ("@ l4_thread_switch  \n\t"
     PIC_SAVE_ASM
     "stmdb   sp!, {fp}   \n\t"
     "mov     lr, pc      \n\t"
     "mov     pc, %1      \n\t"
     "ldmia   sp!, {fp}   \n\t"
     PIC_RESTORE_ASM
     :
     "=r"(_dest)
     :
     "i"(L4_SYSCALL_THREAD_SWITCH),
     "0"(_dest)
     :
     "r1", "r2", "r3", "r4", "r5", "r6", "r7",
     "r8", "r9" PIC_CLOBBER, "r12", "r14", "memory"
     );
}

L4_INLINE void
l4_thread_ex_regs_sc(l4_umword_t val0,
                     l4_umword_t ip,
                     l4_umword_t sp,
                     l4_threadid_t *preempter,
                     l4_threadid_t *pager,
                     l4_umword_t *old_cpsr,
                     l4_umword_t *old_ip,
                     l4_umword_t *old_sp)
{
  register l4_umword_t _dst   asm("r0") = val0;
  register l4_umword_t _ip    asm("r1") = ip;
  register l4_umword_t _sp    asm("r2") = sp;
  register l4_umword_t _pager asm("r3") = pager->raw;
  register l4_umword_t _flags asm("r4");
  register l4_umword_t _prmpt asm("r5") = preempter->raw;

  __asm__ __volatile__
    (
     PIC_SAVE_ASM
     "stmdb   sp!, {fp, r12} \n\t"
     "mov     lr, pc         \n\t"
     "mov     pc, %6         \n\t"
     "ldmia  sp!, {fp, r12}  \n\t"
     PIC_RESTORE_ASM
     :
     "=r" (_dst),
     "=r" (_ip),
     "=r" (_sp),
     "=r" (_pager),
     "=r" (_flags),
     "=r" (_prmpt)
     :
     "i" (L4_SYSCALL_LTHREAD_EX_REGS),
     "0" (_dst),
     "1" (_ip),
     "2" (_sp),
     "3" (_pager),
     "5" (_prmpt)
     :
     "r6", "r7", "r8", "r9" PIC_CLOBBER, "r14", "memory"
     );

  if(pager) pager->raw = _pager;
  if(preempter) preempter->raw = _prmpt;
  if(old_ip) *old_ip = _ip;
  if(old_sp) *old_sp = _sp;
  if(old_cpsr) *old_cpsr = _flags;
}

L4_INLINE l4_taskid_t
l4_task_new_sc(l4_threadid_t dest,
               l4_umword_t mcp_or_new_chief_and_flags,
               l4_umword_t usp,
               l4_umword_t uip,
               l4_threadid_t pager)
{
  register l4_umword_t _dest  asm("r0") = dest.raw;
  register l4_umword_t _mcp   asm("r1") = mcp_or_new_chief_and_flags;
  register l4_umword_t _pager asm("r2") = pager.raw;
  register l4_umword_t _uip   asm("r3") = uip;
  register l4_umword_t _usp   asm("r4") = usp;

  __asm__ __volatile__
    ("@ l4_task_new()   \n\t"
     PIC_SAVE_ASM
     "stmdb   sp!, {fp} \n\t"
     "mov     lr, pc    \n\t"
     "mov     pc, %[syscall]    \n\t"
     "ldmia   sp!, {fp} \n\t"
     PIC_RESTORE_ASM
     :
     "=r"(_dest),
     "=r"(_mcp),
     "=r"(_pager),
     "=r"(_uip),
     "=r"(_usp)
     :
     [syscall] "i" (L4_SYSCALL_TASK_NEW),
     "0"(_dest),
     "1"(_mcp),
     "2"(_pager),
     "3"(_uip),
     "4"(_usp)
     :
     "r5", "r6", "r7", "r8", "r9" PIC_CLOBBER,
     "r12", "r14");

  return (l4_taskid_t){raw:_dest};
}

L4_INLINE l4_cpu_time_t
l4_thread_schedule(l4_threadid_t dest,
                   l4_sched_param_t param,
                   l4_threadid_t *ext_preempter,
                   l4_threadid_t *partner,
                   l4_sched_param_t *old_param)
{
  register l4_umword_t _param     asm("r0") = param.raw;
  register l4_umword_t _dest      asm("r1") = dest.raw;
  register l4_umword_t _preempter asm("r2") = ext_preempter->raw;
  register l4_umword_t _t0        asm("r3");
  register l4_umword_t _t1        asm("r4");

  if (_param != -1UL)
    _param &= 0xfff0ffff;

  __asm__ __volatile__
    ("@ l4_thread_schedule \n\t"
     PIC_SAVE_ASM
     "stmdb   sp!, {fp}    \n\t"
     "mov     lr, pc       \n\t"
     "mov     pc, %5       \n\t"
     "ldmia   sp!, {fp}    \n\t"
     PIC_RESTORE_ASM
     :
     "=r"(_param),
     "=r"(_dest),
     "=r"(_preempter),
     "=r"(_t0),
     "=r"(_t1)
     :
     "i" (L4_SYSCALL_THREAD_SCHEDULE),
     "0"(_param),
     "1"(_dest),
     "2"(_preempter)
     :
     "r5", "r6", "r7", "r8", "r9" PIC_CLOBBER, "r12", "r14"
     );

  old_param->raw      = _param;
  partner->raw        = _dest;
  ext_preempter->raw  = _preempter;

  return (l4_cpu_time_t)_t0 | (l4_cpu_time_t)_t1 << 32;
}

L4_INLINE int
l4_privctrl(l4_umword_t cmd,
            l4_umword_t param)
{
  return -1;
}

#include <l4/sys/syscalls-impl.h>

#endif /* ! __L4SYS__INCLUDE__ARCH_ARM__L4API_L4V2__SYSCALLS_H__ */
