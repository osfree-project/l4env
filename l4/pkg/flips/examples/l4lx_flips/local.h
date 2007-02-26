#include <l4/sys/types.h>
#include <l4/l4vfs/types.h>

/* THREAD ENTRY */
typedef struct {
	l4_threadid_t worker_tid; /* worker thread id */
	l4_threadid_t owner_tid;  /* owner thread id == the client */
	object_handle_t fd;       /* object_handle for the client file object */
	void *private_data;       /* pointer to private data of each thread entry */
} thread_entry_t;

typedef struct notify_private
{
	int mode;                 /* selected non-blocking operations */
	l4_threadid_t notif_tid;  /* notify listener thread id */
} notif_private_t;

/** THREAD TABLE MANIPULATION **/
thread_entry_t tt_get_entry(int, l4_threadid_t);
int tt_get_entry_id(int, l4_threadid_t);
int tt_get_next_free_entry(void);
void tt_free_entry(int, l4_threadid_t);
void tt_fill_entry(int, thread_entry_t);
void tt_init(void);

void * tt_entry_get_private_data(thread_entry_t *);
void tt_entry_set_private_data(thread_entry_t *, void *);

/** NOTIFY INTERFACE **/
void internal_notify_request(int, int, l4_threadid_t *, l4_threadid_t *);
void internal_notify_clear(int, int, l4_threadid_t *, l4_threadid_t *);
