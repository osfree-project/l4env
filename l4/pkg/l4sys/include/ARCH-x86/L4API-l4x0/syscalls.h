/*
 * $Id$
 */

#ifndef __L4_SYSCALLS_H__
#define __L4_SYSCALLS_H__

#include <l4/sys/compiler.h>
#include <l4/sys/types.h>
#include <l4/sys/kdebug.h>

#define L4_FP_REMAP_PAGE	0x00	/* Page is set to read only */
#define L4_FP_FLUSH_PAGE	0x02	/* Page is flushed completly */
#define L4_FP_OTHER_SPACES	0x00	/* Page is flushed in all other */
					/* address spaces */
#define L4_FP_ALL_SPACES	0x80000000U
					/* Page is flushed in own address */ 
					/* space too */

#define L4_NC_SAME_CLAN		0x00	/* destination resides within the */
					/* same clan */
#define L4_NC_INNER_CLAN	0x0C	/* destination is in an inner clan */
#define L4_NC_OUTER_CLAN	0x04	/* destination is outside the */
					/* invoker's clan */

#define L4_CT_LIMITED_IO	0
#define L4_CT_UNLIMITED_IO	1
#define L4_CT_DI_FORBIDDEN	0
#define L4_CT_DI_ALLOWED	1

/*
 * prototypes
 */
L4_INLINE void 
l4_fpage_unmap(l4_fpage_t fpage,
	       l4_umword_t map_mask);

L4_INLINE l4_threadid_t 
l4_myself(void);

L4_INLINE int 
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
l4_thread_switch(l4_threadid_t destination);

L4_INLINE l4_cpu_time_t
l4_thread_schedule(l4_threadid_t dest,
		   l4_sched_param_t param,
		   l4_threadid_t *ext_preempter,
		   l4_threadid_t *partner,
		   l4_sched_param_t *old_param);

L4_INLINE l4_taskid_t 
l4_task_new(l4_taskid_t destination,
	    l4_umword_t mcp_or_new_chief, 
	    l4_umword_t esp,
	    l4_umword_t eip,
	    l4_threadid_t pager);

L4_INLINE void
l4_yield (void);

L4_INLINE 
void *l4_kernel_interface(void);

      


/*
 * Implementation
 */

#ifndef CONFIG_L4_CALL_SYSCALLS

# define L4_SYSCALL_id_nearest		"int $0x31 \n\t"
# define L4_SYSCALL_fpage_unmap		"int $0x32 \n\t"
# define L4_SYSCALL_thread_switch	"int $0x33 \n\t"
# define L4_SYSCALL_thread_schedule	"int $0x34 \n\t"
# define L4_SYSCALL_lthread_ex_regs	"int $0x35 \n\t"
# define L4_SYSCALL_task_new		"int $0x36 \n\t"
# define L4_SYSCALL(name)		L4_SYSCALL_ ## name

#else

# ifdef CONFIG_L4_ABS_SYSCALLS
#  define L4_SYSCALL(s) "call __L4_"#s"_direct   \n\t"
# else
#  define L4_SYSCALL(s) "call *__L4_"#s"  \n\t"
# endif

#endif

#define GCC_VERSION	(__GNUC__ * 100 + __GNUC_MINOR__)

#include <l4/sys/xadaption.h>

#ifdef PROFILE
#  error "PROFILE support for X.0 API not supported"
#else
#  if GCC_VERSION < 295
#    error gcc >= 2.95 required
#  elif GCC_VERSION < 302
#    ifdef __PIC__
#      include "syscalls-l4x0adapt-gcc295-pic.h"
#    else
#      include "syscalls-l4x0adapt-gcc295-nopic.h"
#    endif
#  else
#    ifdef __PIC__
#      include "syscalls-l4x0adapt-gcc3-pic.h"
#    else
#      include "syscalls-l4x0adapt-gcc3-nopic.h"
#    endif
#  endif
#endif

L4_INLINE void
l4_yield (void)
{
	l4_thread_switch(L4_NIL_ID);
}

#endif /* __L4_SYSCALLS_H__ */
