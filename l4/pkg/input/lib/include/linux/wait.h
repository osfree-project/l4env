#ifndef _LINUX_WAIT_H
#define _LINUX_WAIT_H

struct __wait_queue_entry_t {
	l4_threadid_t tid;
	struct __wait_queue_entry_t *next;
};

struct __wait_queue_head_t {
	struct __wait_queue_entry_t *first;
	struct __wait_queue_head_t  *next;
};

typedef struct __wait_queue_entry_t wait_queue_entry_t;
typedef struct __wait_queue_head_t wait_queue_head_t;

extern l4_threadid_t wait_thread;

void wake_up(wait_queue_head_t *wq);

#define init_waitqueue_head(wq)						\
	do {								\
		(wq)->first = 0;					\
		(wq)->next  = 0;					\
	} while (0)

#define wait_event_timeout(wq, condition, timeout)			\
({									\
	unsigned stop, end = jiffies + timeout;				\
	l4_umword_t dummy;						\
	l4_msgdope_t result;						\
	wait_queue_entry_t wqe;						\
	while (!(condition) && (end - jiffies < 1000000000))		\
	{								\
		l4_ipc_call(wait_thread,				\
			    L4_IPC_SHORT_MSG, (l4_umword_t)&(wq),	\
					      (l4_umword_t)&(wqe),	\
			    L4_IPC_SHORT_MSG, &dummy, &dummy,		\
		  L4_IPC_NEVER, &result);				\
		if (L4_IPC_IS_ERROR(result))				\
			enter_kdebug("wait_timeout");			\
 	}								\
	stop = jiffies;							\
	if (stop < end)							\
		end -= stop;						\
	else								\
		end = 0;						\
	end;								\
})

#endif
