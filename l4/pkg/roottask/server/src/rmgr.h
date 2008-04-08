#ifndef RMGR_H
#define RMGR_H

#include <l4/sys/compiler.h>
#include <l4/sys/kernel.h>
#include <l4/util/mb_info.h>
#include <l4/rmgr/proto.h>

extern l4_threadid_t     myself;	/* the rmgr threadid */
extern l4_threadid_t     my_pager;	/* the pager threadid */
extern l4_threadid_t     my_preempter;	/* the preempter threadid */
extern l4_threadid_t     _rmgr_super_id;/* resource manager threadid */
extern l4_threadid_t     _rmgr_pager_id;/* page fault handler threadid */
extern int               l4_version;	/* version of the l4 kernel */
extern int               ux_running;
extern int               quiet;
extern l4_kernel_info_t *kip;
extern l4_addr_t         mem_lower;	/* see multiboot info */

/* number of small address spaces configured at boot */
extern unsigned small_space_size;
extern unsigned debug_log_mask;
extern unsigned debug_log_types;

void rmgr_main(int memdump) L4_NORETURN;

/* max number of tasks rmgr can handle */
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
#  define RMGR_TASK_MAX (1L << 8)
#else
#  define RMGR_TASK_MAX (1L << 11)
#endif

#define RMGR_IO_MAX	L4_IOPORT_MAX	/* max number of IO ports */
#define RMGR_SMALL_MAX	128		/* max number of small spaces */
#define RMGR_CFG_MAX	128		/* max number of configurations */

/* x = 0..17  -> threadno 16..33*/
#define LTHREAD_NO_IRQ(x) (RMGR_IRQ_LTHREAD + (x))

#endif
