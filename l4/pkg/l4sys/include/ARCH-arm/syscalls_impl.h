#ifndef L4_SYSCALLS_IMPL_H
#define L4_SYSCALLS_IMPL_H

#include <l4/sys/compiler.h>

#ifndef L4_SYSCALL_MAGIC_OFFSET  
#  define L4_SYSCALL_MAGIC_OFFSET 	8
#endif
#define L4_SYSCALL_ID_NEAREST		(-0x00000008-L4_SYSCALL_MAGIC_OFFSET)
#define L4_SYSCALL_FPAGE_UNMAP		(-0x0000000C-L4_SYSCALL_MAGIC_OFFSET)
#define L4_SYSCALL_THREAD_SWITCH	(-0x00000010-L4_SYSCALL_MAGIC_OFFSET)
#define L4_SYSCALL_THREAD_SCHEDULE	(-0x00000014-L4_SYSCALL_MAGIC_OFFSET)
#define L4_SYSCALL_LTHREAD_EX_REGS	(-0x00000018-L4_SYSCALL_MAGIC_OFFSET)
#define L4_SYSCALL_CREATE_THREAD	(-0x0000001C-L4_SYSCALL_MAGIC_OFFSET)


L4_INLINE l4_threadid_t l4_myself()
{
  register l4_umword_t nil_id asm("r0") = L4_NIL_ID.raw; 
  register l4_threadid_t id   asm("r1");
  
  __asm__ __volatile__
    ("stmdb sp!, {fp} \n\t"
     "mov	lr, pc			\n\t"
     "mov	pc, %2			\n\t"
     "ldmia sp!, {fp} \n\t"
     :
     "=r"(nil_id),
     "=r"(id)
     :
     "i"(L4_SYSCALL_ID_NEAREST),
     "0"(nil_id)
     :
     "r2", "r3", "r4", "r5", "r6", "r7", 
     "r8", "r9", "r10", "r12", "r14", "memory"
     );
  
  return id;
}

L4_INLINE void 
l4_fpage_unmap(l4_fpage_t fpage, l4_umword_t mask)
{
  register l4_umword_t _fpage asm("r0") = fpage.raw;
  register l4_umword_t _mask  asm("r1") = mask;
  
  __asm__ __volatile__ 
    ("@ l4_fpage_unmap	\n\t"
     "stmdb sp!, {fp}   \n\t"
     "mov lr, pc        \n\t"
     "mov	pc, %2				\n\t"
     "ldmia sp!, {fp}   \n\t"
     :
     "=r"(_fpage),
     "=r"(_mask)
     :
     "i"(L4_SYSCALL_FPAGE_UNMAP),
     "0"(_fpage),
     "1"(_mask)
     :
     "r2", "r3", "r4", "r5", "r6", "r7", 
     "r8", "r9", "r10", "r12", "r14"
     );
}

L4_INLINE void 
l4_thread_switch(l4_threadid_t dest)
{
  register l4_umword_t _dest asm("r0") = dest.raw;
  __asm__ __volatile__ 
    ("@ l4_thread_switch	\n\t"
     "stmdb sp!, {fp}     \n\t"
     "mov	lr, pc					\n\t"
     "mov	pc, %1					\n\t"
     "ldmia sp!, {fp}     \n\t"
     : 
     "=r"(_dest)
     :
     "i"(L4_SYSCALL_THREAD_SWITCH),
     "0"(_dest) 
     :
     "r1", "r2", "r3", "r4", "r5", "r6", "r7", 
     "r8", "r9", "r10", "r12", "r14", "memory"
     );
}

L4_INLINE void 
l4_thread_ex_regs(l4_threadid_t destination,
                  l4_umword_t ip,
                  l4_umword_t sp,	
                  l4_threadid_t *preempter,
                  l4_threadid_t *pager,
                  l4_umword_t *old_cpsr,
                  l4_umword_t *old_ip,
                  l4_umword_t *old_sp)
{
  register l4_umword_t _dst   asm("r0") = destination.id.lthread;
  register l4_umword_t _ip    asm("r1") = ip;
  register l4_umword_t _sp    asm("r2") = sp;
  register l4_umword_t _pager asm("r3") = pager->raw;
  register l4_umword_t _flags asm("r4");
  
  (void)preempter;
  
  __asm__ __volatile__ 
    (	
     "stmdb sp!, {fp}   \n\t"
     "mov	lr, pc		    \n\t"
     "mov	pc, %5				\n\t"
     "ldmia sp!, {fp}   \n\t"
     :
     "=r" (_dst),
     "=r" (_ip),
     "=r" (_sp),
     "=r" (_pager),
     "=r" (_flags)
     :
     "i" (L4_SYSCALL_LTHREAD_EX_REGS),
     "0" (_dst),
     "1" (_ip),
     "2" (_sp),
     "3" (_pager)	
     :
     "r5", "r6", "r7", "r8", "r9",
     "r10", "r12", "r14", "memory"
     );
  
  if(pager) pager->raw = _pager;
  if(old_ip) *old_ip = _ip;
  if(old_sp) *old_sp = _sp;
  if(old_cpsr) *old_cpsr = _flags;
}

L4_INLINE l4_taskid_t
l4_task_new(l4_threadid_t dest,
            l4_umword_t mcp,
            l4_umword_t usp,
            l4_umword_t uip,
            l4_threadid_t pager)
{
  register l4_umword_t _dest  asm("r0") = dest.raw;
  register l4_umword_t _mcp   asm("r1") = mcp;
  register l4_umword_t _pager asm("r2") = pager.raw;
  register l4_umword_t _uip   asm("r3") = uip;
  register l4_umword_t _usp   asm("r4") = usp;
  __asm__ __volatile__ 
    ("@ l4_task_new() \n\t"
     "stmdb sp!, {fp} \n\t"
     "mov	lr, pc			\n\t"
     "mov	pc, %5			\n\t"
     "ldmia sp!, {fp} \n\t"
     :
     "=r"(_dest),
     "=r"(_mcp),
     "=r"(_pager),
     "=r"(_uip),
     "=r"(_usp)
     :
     "i" (L4_SYSCALL_CREATE_THREAD),
     "0"(_dest),
     "1"(_mcp),
     "2"(_pager),
     "3"(_uip),
     "4"(_usp)
     :
     "r5", "r6", "r7", "r8", "r9",
     "r10", "r12", "r14");

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
  
  __asm__ __volatile__ 
    ("@ l4_thread_schedule \n\t"
     "stmdb sp!, {fp}      \n\t"
     "mov	lr, pc					 \n\t"
     "mov	pc, %5					 \n\t"
     "ldmia sp!, {fp}      \n\t"
     :
     "=r"(_param),
     "=r"(_dest),
     "=r"(_preempter),
     "=r"(_t0),
     "=r"(_t1)
     :
     "i" (L4_SYSCALL_THREAD_SCHEDULE),
     "0"(_dest),
     "1"(_param),
     "2"(_preempter)
     :
     "r5", "r6", "r7", "r8", "r9", "r10", "r12", "r14"
     );
  
  old_param->raw      = _param;
  partner->raw        = _dest;
  ext_preempter->raw  = _preempter;
  
  return (l4_cpu_time_t)_t0 | (l4_cpu_time_t)_t1 << 32;
}

#endif /* L4_SYSCALLS_IMPL_H */
