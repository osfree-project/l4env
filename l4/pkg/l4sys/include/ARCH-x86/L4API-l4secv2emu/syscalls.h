/* $Id$ */
/*****************************************************************************/
/**
 * \file    l4sys/include/ARCH-x86/L4API-l4v2/syscalls.h
 * \brief   L4 System calls (except for IPC)
 * \ingroup api_calls
 */
/*****************************************************************************/
#ifndef __L4_SYSCALLS_H__
#define __L4_SYSCALLS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <l4/sys/types.h>
//#include <l4/sys/kdebug.h>

/*****************************************************************************
 *** Syscall arguments
 *****************************************************************************/

/* l4_fpage_unmap */
#define L4_FP_REMAP_PAGE        0x00    /**< Set page to read only
                                         **  \ingroup api_calls_other
                                         **  \hideinitializer
                                         **/
#define L4_FP_FLUSH_PAGE        0x02    /**< Flush page completely
                                         **  \ingroup api_calls_other
                                         **  \hideinitializer
                                         **/

#define L4_FP_OTHER_SPACES      0x00    /**< Flush page in all other
                                         **  address spaces
                                         **  \ingroup api_calls_other
                                         **  \hideinitializer
                                         **/
#define L4_FP_ALL_SPACES        0x80000000U
                                        /**< Flush page in own address
                                         **  space too
                                         **  \ingroup api_calls_other
                                         **  \hideinitializer
                                         **/

/* l4_nchief return values */
#define L4_NC_SAME_CLAN         0x00    /**< Destination resides within the
                                         **  same clan
                                         **  \ingroup api_calls_other
                                         **  \hideinitializer
                                         **/
#define L4_NC_INNER_CLAN        0x0C    /**< Destination is in an inner clan
                                         **  \ingroup api_calls_other
                                         **  \hideinitializer
                                         **/
#define L4_NC_OUTER_CLAN        0x04    /**< Destination is outside the
                                         **  invoker's clan
                                         **  \ingroup api_calls_other
                                         **  \hideinitializer
                                         **/

/*****************************************************************************
 *** Prototypes
 *****************************************************************************/

/**
 * Unmap flexpage
 * \ingroup api_calls_other
 *
 * \param   fpage        Flexpage descriptor
 * \param   map_mask     Unmap mask (unmap type and unmap location can be
 *                       combined)
 *                       - Unmap type:
 *                         - #L4_FP_REMAP_PAGE remap all pages read-only.
 *                           Pages already mapped read-only are not affected.
 *                         - #L4_FP_FLUSH_PAGE completely unmap pages
 *                       - Unmap location:
 *                         - #L4_FP_OTHER_SPACES unmap pages in all address
 *                           spaces into which pages have been mapped directly
 *                           or indirectly. The pages in the own address space
 *                           remain mapped.
 *                         - #L4_FP_ALL_SPACES additionally, also unmap the
 *                           pages in the own address space.
 *
 * The l4_fpage_unmap() system call unmaps the specified flex page
 * in all address spaces into which the invoker mapped it directly or
 * indirectly.
 */
void
l4_fpage_unmap(l4_fpage_t fpage,
	       l4_umword_t map_mask);

l4_threadid_t
l4_myself(void);

L4_INLINE l4_threadid_t
l4_myself_noprof(void);

int
l4_nchief(l4_threadid_t destination,
	  l4_threadid_t *next_chief);

L4_INLINE void
l4_thread_ex_regs(l4_threadid_t destination,
		  l4_umword_t eip,
		  l4_umword_t esp,
		  l4_threadid_t *preempter,
		  l4_threadid_t *pager,
		  l4_umword_t *old_eflags,
		  l4_umword_t *old_eip,
		  l4_umword_t *old_esp);

L4_INLINE void
l4_thread_ex_regs_flags(l4_threadid_t destination,
                        l4_umword_t eip,
                        l4_umword_t esp,
                        l4_threadid_t *preempter,
                        l4_threadid_t *pager,
                        l4_umword_t *old_eflags,
                        l4_umword_t *old_eip,
                        l4_umword_t *old_esp,
		        unsigned long flags);

enum {
  /* The target thread will raise an exception immediately just before
   * returning to userland. */
  L4_THREAD_EX_REGS_RAISE_EXCEPTION = 1 << 28,
  /* The target thread is handled as alien, that is every L4 syscall
   * will generate an exception IPC. */
  L4_THREAD_EX_REGS_ALIEN           = 1 << 29,
  /* Don't cancel ongoing IPC. */
  L4_THREAD_EX_REGS_NO_CANCEL       = 1 << 30,
  /* Reserved for backward compatibility with older L4 versions. */
  L4_THREAD_EX_REGS_VM86            = 1 << 31,
};


L4_INLINE void
l4_inter_task_ex_regs(l4_threadid_t destination,
		      l4_umword_t eip,
		      l4_umword_t esp,
		      l4_threadid_t *preempter,
		      l4_threadid_t *pager,
		      l4_umword_t *old_eflags,
		      l4_umword_t *old_eip,
		      l4_umword_t *old_esp,
		      unsigned long flags);


L4_INLINE l4_threadid_t
l4_thread_ex_regs_pager(l4_threadid_t destination);


void
l4_thread_switch(l4_threadid_t destination);


void
l4_yield(void);

l4_cpu_time_t
l4_thread_schedule(l4_threadid_t dest,
		   l4_sched_param_t param,
		   l4_threadid_t *ext_preempter,
		   l4_threadid_t *partner,
		   l4_sched_param_t *old_param);


l4_taskid_t
l4_task_new(l4_taskid_t destination,
	    l4_umword_t mcp_or_new_chief,
	    l4_umword_t esp,
	    l4_umword_t eip,
	    l4_threadid_t pager);
enum {
  L4_TASK_NEW_RAISE_EXCEPTION = 1 << 30,
  L4_TASK_NEW_ALIEN           = 1 << 31,
};


/**
 * Return pointer to Kernel Interface Page
 * \ingroup api_calls_other
 */
L4_INLINE void *
l4_kernel_interface(void);

int
l4_privctrl(l4_umword_t cmd,
	    l4_umword_t param);

/*****************************************************************************
 *** Implementation
 *****************************************************************************/


L4_INLINE l4_threadid_t
l4_myself_noprof(void)
{
    return l4_myself();
}


/* ================ lthread_ex_regs variants =================== */


void 
__do_l4_thread_ex_regs(l4_umword_t val0,
                       l4_umword_t eip,
                       l4_umword_t esp, 
                       l4_threadid_t *preempter,
                       l4_threadid_t *pager,
                       l4_umword_t *old_eflags,
                       l4_umword_t *old_eip,
                       l4_umword_t *old_esp);

L4_INLINE void
l4_thread_ex_regs(l4_threadid_t destination,
		  l4_umword_t eip,
		  l4_umword_t esp,
		  l4_threadid_t *preempter,
		  l4_threadid_t *pager,
		  l4_umword_t *old_eflags,
		  l4_umword_t *old_eip,
		  l4_umword_t *old_esp)
{
  __do_l4_thread_ex_regs(destination.id.lthread,
                         eip, esp, preempter, pager,
                         old_eflags, old_eip, old_esp);
}

/*
 * L4 lthread_ex_regs with additional flags
 */
L4_INLINE void
l4_thread_ex_regs_flags(l4_threadid_t destination,
                        l4_umword_t eip,
                        l4_umword_t esp,
                        l4_threadid_t *preempter,
                        l4_threadid_t *pager,
                        l4_umword_t *old_eflags,
                        l4_umword_t *old_eip,
                        l4_umword_t *old_esp,
                        unsigned long flags)
{
  __do_l4_thread_ex_regs(destination.id.lthread | flags,
                         eip, esp, preempter, pager,
                         old_eflags, old_eip, old_esp);
}

/*
 * L4 lthread_inter_task_ex_regs
 */
L4_INLINE void
l4_inter_task_ex_regs(l4_threadid_t destination,
		      l4_umword_t eip,
		      l4_umword_t esp,
		      l4_threadid_t *preempter,
		      l4_threadid_t *pager,
		      l4_umword_t *old_eflags,
		      l4_umword_t *old_eip,
		      l4_umword_t *old_esp,
		      unsigned long flags)
{
  __do_l4_thread_ex_regs(destination.id.lthread
                          | (destination.id.task << 7) | flags,
                         eip, esp, preempter, pager,
                         old_eflags, old_eip, old_esp);
}

/*
 * Special version of l4_thread_ex_regs to get the pager of a thread
 */
L4_INLINE l4_threadid_t
l4_thread_ex_regs_pager(l4_threadid_t destination)
{
  l4_umword_t dummy;
  l4_threadid_t preempter = L4_INVALID_ID;
  l4_threadid_t pager     = L4_INVALID_ID;

  l4_thread_ex_regs_flags(destination, (l4_umword_t)-1, (l4_umword_t)-1,
                          &preempter, &pager, &dummy, &dummy, &dummy,
                          L4_THREAD_EX_REGS_NO_CANCEL);
  return pager;
}


/* ============================================================= */

L4_INLINE void
l4_yield(void)
{
  l4_thread_switch(L4_NIL_ID);
}

L4_INLINE void *
l4_kernel_interface(void)
{
  void *ret;
  asm (" lock; nop " : "=a"(ret) );
  return ret;
}

#ifdef __cplusplus
}
#endif

//#endif

#endif /* !__L4_SYSCALLS_H__ */
