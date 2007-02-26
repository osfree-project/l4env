#ifndef __SOCKET_INTERNAL_H
#define __SOCKET_INTERNAL_H

/* *** L4-SPECIFIC INCLUDES *** */
#include <l4/semaphore/semaphore.h>

/* ******************************************************************* */

/* excerpts from dietlibc's sys/socket.h: */

/* address/protocol families */
#define AF_UNSPEC       0
#define AF_UNIX         1       /* Unix domain sockets          */
#define AF_LOCAL        1       /* POSIX name for AF_UNIX       */
#define PF_UNSPEC       AF_UNSPEC
#define PF_UNIX         AF_UNIX
#define PF_LOCAL        AF_LOCAL

/* Socket types. */
#ifdef __mips__
#define SOCK_DGRAM      1               /* datagram (conn.less) socket  */
#define SOCK_STREAM     2               /* stream (connection) socket   */
#else
#define SOCK_STREAM     1               /* stream (connection) socket   */
#define SOCK_DGRAM      2               /* datagram (conn.less) socket  */
#endif
#define SOCK_RAW        3               /* raw socket                   */
#define SOCK_RDM        4               /* reliably-delivered message   */
#define SOCK_SEQPACKET  5               /* sequential packet socket     */
#define SOCK_PACKET     10              /* linux specific way of        */
                                        /* getting packets at the dev   */
                                        /* level.  For writing rarp and */
                                        /* other similar things on the  */
                                        /* user level.                  */

/* ******************************************************************* */

#define MAX_SOCKETS   128
#define MAX_ADDRESSES  16

/* MAX_ADDRESS_LEN alias sizeof(sockaddr_un.sun_path) == 108, like in glibc */
#define MAX_ADDRESS_LEN 108

#define MAX_BACKLOG 8
#define SOCKET_BUFFER_SIZE 4096
#define CONNECT_TIMEOUT 60000

/* ******************************************************************* */

#define JOB_TYPE_CONNECT 0
#define JOB_TYPE_ACCEPT  1
#define JOB_TYPE_SEND    2
#define JOB_TYPE_RECV    3
#define JOB_TYPE_WRITE   4
#define JOB_TYPE_READ    5

typedef struct connect_in {
  int        fd;
  const char *addr;
  int        addr_len;
} connect_in_t;

typedef struct connect_out{  
  const char *addr;
  int        addr_len;
} connect_out_t;

/* ********************** */

typedef struct accept_in {  
  int        fd;
  const char *addr;
  int        addr_len;
} accept_in_t;

typedef struct accept_out {  
  int  fd;
  char *addr;
  int  addr_len;
} accept_out_t;

/* ********************** */

typedef struct send_in {  
  int  fd;
  char *msg;
  int  len;
  int  flags;
} send_in_t;

typedef struct send_out {
} send_out_t;

/* ********************** */

typedef struct recv_in {  
  int  fd;
  char *msg;
  int  len;
  int  flags;
} recv_in_t;

typedef struct recv_out {
  char *msg;
  int  len;
} recv_out_t;

/* ********************** */

typedef struct job_info {
  l4_threadid_t client;    /* thread, which made the request */
  l4_threadid_t worker;    /* server thread, which does the work */
  l4_threadid_t replier;   /* server thread, which runs the server loop */
  int           type;
  int           retval;
  int           handle;
  union {
    connect_in_t connect;
    accept_in_t  accept;
    send_in_t    send;
    recv_in_t    recv;
  } in;
  union {
    connect_out_t connect;
    accept_out_t  accept;
    send_out_t    send;
    recv_out_t    recv;
  } out;
} job_info_t;

/* ******************************************************************* */

typedef struct connect_node {
  job_info_t          *job;
  struct connect_node *next;
} connect_node_t;


typedef struct connect_queue {
  connect_node_t *first;
  connect_node_t *last;
  int            count;
  l4semaphore_t  accept_sem;
} connect_queue_t;

/* ******************************************************************* */

typedef struct buffer {
  char bytes[SOCKET_BUFFER_SIZE];
  int  num_bytes;      /* number of bytes in buffer */
  int  r_start;        /* to avoid unnecessary copying, the logical start of   */
  int  w_start;        /* the buffer may be greater than zero, which means     */
  int  read_blocked:1; /* there can be a wrap around, after less data was read */
  int  write_blocked:1;/* from the buffer than previously was written to it    */
  l4semaphore_t read_sem;
  l4semaphore_t write_sem;
} buffer_t;

/* ******************************************************************* */

typedef struct notify_node {
  struct notify_node *next;
  l4_threadid_t      client;
  int                notified;
} notify_node_t;

typedef struct notify_queue {
  notify_node_t *first;
  notify_node_t *last;
} notify_queue_t;

/* ******************************************************************* */

#define SOCKET_STATE_NIL        0x0000
#define SOCKET_STATE_BIND       0x0001
#define SOCKET_STATE_LISTEN     0x0002
#define SOCKET_STATE_ACCEPTING  0x0004
#define SOCKET_STATE_ACCEPT     0x0008
#define SOCKET_STATE_CONNECTING 0x0010
#define SOCKET_STATE_CONNECT    0x0020
#define SOCKET_STATE_SEND       0x0040
#define SOCKET_STATE_RECV       0x0080
#define SOCKET_STATE_DRAIN_BUF  0x0800
#define SOCKET_STATE_HAS_PEER   0x1000
#define SOCKET_STATE_NONBLOCK   0x2000


typedef struct socket_desc {
  unsigned int  unused :16;      
  unsigned int  backlog:16;      /* max. number of connect()s to be queued */
  unsigned int  used   : 1;      /* indicates, whether this socket descriptor is used */
  unsigned int  stream : 1;      /* communication type is SOCK_STREAM */
  unsigned int  state  :14; 
  unsigned int  flags  :16;
  int           addr_handle;     /* the address the socket is bound to */
  struct socket_desc *peer;         /* the other side of the socket */
  l4semaphore_t      lock;
  connect_queue_t    connect_queue;
  l4semaphore_t      connect_sem;   /* used to block/wake up a connect() worker thread*/
  buffer_t           buf;           /* write buffer */
  notify_queue_t     read_notify;   /* list of clients waiting in select() for read */
  notify_queue_t     write_notify;  /* list of clients waiting in select() for write */
  notify_queue_t     except_notify; /* list of clients waiting in select() for exeptions */
} socket_desc_t;


typedef struct addr_entry {
  int  handle;
  int  ref_count;
  char sun_path[MAX_ADDRESS_LEN];
} addr_entry_t;

/* ******************************************************************* */

extern socket_desc_t socket_table[];

/* ******************************************************************* */

void local_socks_init(void);
job_info_t *create_job_info(int type, l4_threadid_t client);

static inline int client_owns_handle(l4_threadid_t client, int h);
static inline int handle_is_valid(int h);

int socket_internal    (int domain, int type, int protocol);
int socketpair_internal(int domain, int type, int protocol, int *h0, int *h1);
int shutdown_internal  (int h, int how);
int close_internal     (int h);
int bind_internal      (int h, const char *addr, int addr_len);
int listen_internal    (int h, int backlog);
int connect_internal   (job_info_t *job, int h, const char *addr, int addr_len);
int accept_internal    (job_info_t *job, int h, const char *addr, int *addr_len);
int send_internal      (job_info_t *job, int h, const char *msg, int len, int flags);
int recv_internal      (job_info_t *job, int h, char *msg, int *len, int flags);
int fcntl_internal     (int h, int cmd, long arg);
int ioctl_internal     (int h, int cmd, char **arg, int *count);

/* ******************************************************************* */

void register_select_notify  (int h, l4_threadid_t client, int mode);
void deregister_select_notify(int h, l4_threadid_t client, int mode);

/* ******************************************************************* */
/* ******************************************************************* */

static inline int client_owns_handle(l4_threadid_t client, int h) {
  return 1;  /* FIXME: well, doing something useful here, would be nice */
}

static inline int handle_is_valid(int h) {
  if (h >= 0 && h < MAX_SOCKETS && socket_table[h].used)
    return 1;
  return 0;
}

#endif /* __SOCKET_INTERNAL_H */

