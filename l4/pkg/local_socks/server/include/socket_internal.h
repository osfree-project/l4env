/* $Id$ */
/*****************************************************************************/
/**
 * \file   local_socks/server/include/socket_internal.h
 * \brief  Header file for internal socket server implementation.
 *
 * \date   15/08/2004
 * \author Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2004-2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __SOCKET_INTERNAL_H
#define __SOCKET_INTERNAL_H

/* *** L4-SPECIFIC INCLUDES *** */
#include <l4/semaphore/semaphore.h>
#include <l4/util/macros.h>

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
#define SOCKET_BUFFER_SIZE 32768
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
} connect_queue_t;

/* ******************************************************************* */

typedef struct buffer {
  char *bytes;
  unsigned int  num_bytes;      /* number of bytes in buffer */
  unsigned int  r_start;        /* to avoid unnecessary copying, the logical start of   */
  unsigned int  w_start;        /* the buffer may be greater than zero, which means     */
  unsigned int  read_blocked :1;/* there can be a wrap around, after less data was read */
  unsigned int  write_blocked:1;/* from the buffer than previously was written to it    */
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

/* basic socket states */
#define SOCKET_STATE_NIL        0x00000
#define SOCKET_STATE_BIND       0x00001
#define SOCKET_STATE_LISTEN     0x00002
#define SOCKET_STATE_ACCEPT     0x00004
#define SOCKET_STATE_CONNECT    0x00008

/* normal sub states indicating socket capabilities */
#define SOCKET_STATE_SEND       0x00100
#define SOCKET_STATE_RECV       0x00200
#define SOCKET_STATE_HAS_PEER   0x00400
#define SOCKET_STATE_NONBLOCK   0x00800

/* the socket has been closed */
#define SOCKET_STATE_CLOSED     0x01000

/* sub states indicating a blocking operation is in progress */
#define SOCKET_STATE_ACCEPTING  0x10000
#define SOCKET_STATE_CONNECTING 0x20000
#define SOCKET_STATE_SENDING    0x40000
#define SOCKET_STATE_RECVING    0x80000



typedef struct socket_desc {
  unsigned int  unused :16;
  unsigned int  backlog:16;      /* max. number of connect()s to be queued */
  unsigned int  used   : 1;      /* indicates, whether this socket descriptor is used */
  unsigned int  stream : 1;      /* communication type is SOCK_STREAM */
  unsigned int  state  :20; 
  unsigned int  flags  :10;
  int           addr_handle;     /* the address the socket is bound to */
  int           num_accepts;     /* th number of currently blocked accept()s */
  struct socket_desc *peer;      /* the other side of the socket */
  l4semaphore_t      lock;
  l4semaphore_t      operation_sem; /* used to block in accept()/connect() */
  l4semaphore_t      serial_r_sem;  /* used to serialize recv() operations */
  l4semaphore_t      serial_w_sem;  /* used to serialize send() operations */
  connect_queue_t    connect_queue; /* holds all pending connect() requests */
  notify_queue_t     read_notify;   /* list of clients waiting in select() for read */
  notify_queue_t     write_notify;  /* list of clients waiting in select() for write */
  notify_queue_t     except_notify; /* list of clients waiting in select() for exeptions */
  buffer_t           buf;           /* write buffer */
  l4_threadid_t      owner;         /* the client which onws the socket */
} socket_desc_t;


typedef struct addr_entry {
  int  handle:31;
  int  used  :1;
  char sun_path[MAX_ADDRESS_LEN];
} addr_entry_t;

/* ******************************************************************* */

extern socket_desc_t socket_table[];

/* ******************************************************************* */

void local_socks_init(void);
void init_job_info(job_info_t *job, l4_threadid_t *client, int type);
job_info_t *create_job_info(l4_threadid_t *client, int type);
void close_all_sockets_of_client(l4_threadid_t *client);

int socket_internal    (l4_threadid_t *client, int domain, int type, int protocol);
int socketpair_internal(l4_threadid_t *client, int domain, int type, int protocol, int *h0, int *h1);
int shutdown_internal  (l4_threadid_t *client, int h, int how);
int close_internal     (l4_threadid_t *client, int h);
int bind_internal      (l4_threadid_t *client, int h, const char *addr, int addr_len);
int listen_internal    (l4_threadid_t *client, int h, int backlog);
int connect_internal   (job_info_t *job, int h, const char *addr, int addr_len);
int accept_internal    (job_info_t *job, int h, const char *addr, int *addr_len);
int send_internal      (job_info_t *job, int h, const char *msg, int len, int flags);
int recv_internal      (job_info_t *job, int h, char *msg, int *len, int flags);
int fcntl_internal     (l4_threadid_t *client, int h, int cmd, long arg);
int ioctl_internal     (l4_threadid_t *client, int h, int cmd, char **arg, int *count);

/* ******************************************************************* */

void register_select_notify  (l4_threadid_t *client, int h, const l4_threadid_t *notfif_tid, int mode);
void deregister_select_notify(l4_threadid_t *client, int h, const l4_threadid_t *notfif_tid, int mode);

/* ******************************************************************* */

static inline void set_basic_state(socket_desc_t *s, int new_state) {
  s->state &= ~(SOCKET_STATE_BIND | SOCKET_STATE_LISTEN | SOCKET_STATE_ACCEPT | SOCKET_STATE_CONNECT);
  s->state |= new_state;
}

static inline void set_sub_state_on(socket_desc_t *s, int sub_state) {
  s->state |= sub_state;
}

static inline void set_sub_state_off(socket_desc_t *s, int sub_state) {
  s->state &= ~sub_state;  
}

static inline int is_in_sub_state(socket_desc_t *s, int sub_state) {
  return (s->state & sub_state) == sub_state;
}

#endif /* __SOCKET_INTERNAL_H */

