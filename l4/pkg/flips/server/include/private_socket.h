#include <l4/socket_linux/socket_linux.h>
#include <linux/sched.h>

typedef struct socket_private
{
    int mode;                 /* selected operations for non blocking */
    int non_block_mode;       /* currently possible non blocking operations */
    l4_threadid_t *notif_tid; /* thread id of notify thread at client side */
    wait_queue_t q;           /* our wait queue element for the socket */
} socket_private_t;

