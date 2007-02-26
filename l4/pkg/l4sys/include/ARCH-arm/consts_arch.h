#ifndef L4_CONSTS_ARCH_H
#define L4_CONSTS_ARCH_H

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

enum {
  L4_TASK_NEW_IPC_MONITOR     = 1 << 29,
  L4_TASK_NEW_RAISE_EXCEPTION = 1 << 30,
  L4_TASK_NEW_ALIEN           = 1 << 31,
  L4_TASK_NEW_NR_OF_FLAGS     = 3,
  L4_TASK_NEW_FLAGS_MASK      = ((1 << L4_TASK_NEW_NR_OF_FLAGS) - 1)
                                  << (sizeof(l4_umword_t) * 8 - L4_TASK_NEW_NR_OF_FLAGS),
};

enum {
  L4_THREAD_EX_REGS_RAISE_EXCEPTION = 1 << 28,
  L4_THREAD_EX_REGS_ALIEN           = 1 << 29,
  L4_THREAD_EX_REGS_NO_CANCEL       = 1 << 30,
};

#endif /* L4_CONSTS_ARCH_H */
