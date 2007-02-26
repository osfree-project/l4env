/* *** GENERAL INCLUDES *** */
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

/* *** L4-SPECIFIC INCLUDES *** */
#include <l4/log/l4log.h>
#include <l4/thread/thread.h>
#include <l4/sys/syscalls.h>
#include <l4/l4vfs/select_listener.h>

/* *** LOCAL INCLUDES *** */
#include "socket_internal.h"

/* ******************************************************************* */

#ifdef DEBUG
# define _DEBUG 0
# define _DEBUG_ENTER 0
# define _DEBUG_SELECT 1
# define LOG_CONNECT_QUEUE(h) log_connect_queue(h)
#else
# define _DEBUG 0
# define _DEBUG_ENTER 0
# define _DEBUG_SELECT 0
# define LOG_CONNECT_QUEUE(h)
#endif

/* ******************************************************************* */

#ifndef MIN
# define MIN(a, b) ( ((a) < (b)) ? (a) : (b) )
#endif
#ifndef MAX
# define MAX(a, b) ( ((a) > (b)) ? (a) : (b) )
#endif

/* ******************************************************************* */

l4semaphore_t socket_table_lock;
l4semaphore_t addr_table_lock;

socket_desc_t socket_table[MAX_SOCKETS];
addr_entry_t  addr_table[MAX_ADDRESSES];

/* ******************************************************************* */

static void send_select_notification(socket_desc_t *s, int mode);
static int  would_block(socket_desc_t *s, int mode);

static int        bytes_to_recv(socket_desc_t *s);
static void       init_buf(socket_desc_t *s);
static inline int can_recv(socket_desc_t *s);
static inline int can_send(socket_desc_t *s);

static void deferred_close_peer(socket_desc_t *s);
static int  do_shutdown(socket_desc_t *s, int how);

static int  allocate_handle(void);
static void free_handle(int h);

static int           allocate_address(const char *addr, socket_desc_t *s);
static void          free_address(int i);
static socket_desc_t *address_owner(const char *addr);

#ifdef DEBUG 
static void log_connect_queue(socket_desc_t *s);
#endif
static connect_node_t *enqueue_connect(socket_desc_t *s, job_info_t *job);
static job_info_t     *dequeue_connect(socket_desc_t *s, job_info_t *job);

static int           enqueue_notify(notify_queue_t *queue, l4_threadid_t client);
static l4_threadid_t dequeue_notify(notify_queue_t *queue, l4_threadid_t client);
static inline int    notify_queue_is_empty(notify_queue_t *queue);

/* ******************************************************************* */

static inline socket_desc_t *socket_desc(int h) {
  return &socket_table[h];
}

static inline int socket_handle(socket_desc_t *s) {
  return (int) (s - socket_table);
}

/* ******************************************************************* */

static inline void socket_lock(socket_desc_t *s) {
  l4semaphore_down(&s->lock);
}

static inline void socket_unlock(socket_desc_t *s) {
  l4semaphore_up(&s->lock);
}

static int socket_lock_peers(socket_desc_t *s);

static inline void addr_lock(void) {
  l4semaphore_down(&addr_table_lock);
}

static inline void addr_unlock(void) {
  l4semaphore_up(&addr_table_lock);
}

/* ******************************************************************* */
/* ******************************************************************* */

int socket_internal(int domain, int type, int protocol) {

  int h, stream;

  LOGd_Enter(_DEBUG_ENTER, "");

  if (domain != PF_LOCAL)
    return -EINVAL;

  if (type == SOCK_STREAM)
    stream = 1;
  else
    return -EPROTONOSUPPORT; /* FIXME: also support type == SOCK_DGRAM */

  if (protocol != 0)
    return -EPROTONOSUPPORT;    

  h = allocate_handle();

  if (h >= 0) {
    socket_desc_t *s = socket_desc(h);
    socket_lock(s);
    s->stream = 1;
    s->flags  = 0;
    s->peer   = NULL;
    socket_unlock(s);
  } else
    return -ENFILE;

  return h;
}



int socketpair_internal(int domain, int type, int protocol, int *h0, int *h1) {

  int stream;

  LOGd_Enter(_DEBUG_ENTER, "");

  if (domain != PF_LOCAL)
    return -EINVAL;

  if (type == SOCK_STREAM)
    stream = 1;
  else
    return -EPROTONOSUPPORT; /* FIXME: also support type == SOCK_DGRAM */

  if (protocol != 0)
    return -EPROTONOSUPPORT;    


  *h0 = allocate_handle();
  if (*h0 >= 0) {
    *h1 = allocate_handle();
    if (*h1 < 0) 
      free_handle(*h0);
  }

  if (*h0 >= 0 && *h1 >= 0) {
    socket_desc_t *s0 = socket_desc(*h0);
    socket_desc_t *s1 = socket_desc(*h1);

    socket_lock(s0);
    socket_lock(s1);
    s0->stream = s1->stream = stream;
    s0->flags  = s1->flags  = 0;
    s0->state  = s1->state  = (SOCKET_STATE_CONNECT | SOCKET_STATE_HAS_PEER |
                               SOCKET_STATE_SEND | SOCKET_STATE_RECV);
    s0->peer = s1;
    s1->peer = s0;
    init_buf(s0);
    init_buf(s1);
    
    socket_unlock(s0);
    socket_unlock(s1);

  } else {
    return -ENFILE;
  }

  return 0;
}



int shutdown_internal(int h, int how) {

  socket_desc_t *s  = socket_desc(h);
  int           ret = 0;
  int           have_peer = 0;

  LOGd_Enter(_DEBUG_ENTER, "h=%d", h);

  if (socket_lock_peers(s) == 0)
    have_peer = 1;

  if (do_shutdown(s, how) >= how)
    ret = -ENOTCONN;

  socket_unlock(s);
  if (have_peer)
    socket_unlock(s->peer);
  return ret;
}



int close_internal(int h) {

  int ret = 0;
  socket_desc_t *s  = socket_desc(h);

  LOGd_Enter(_DEBUG_ENTER, "h=%d", h);

  if (socket_lock_peers(s) < 0) {  /* there is no peer */    

    socket_lock(s);

    if (s->state & (SOCKET_STATE_ACCEPTING | SOCKET_STATE_CONNECTING)) {
      /* FIXME: handle this properly */
      LOG("close() called while blocked in accept()/connect(); this is not implemented");
      ret = -EIO;
    } else if (s->state & (SOCKET_STATE_LISTEN | SOCKET_STATE_BIND))
      free_address(s->addr_handle);

  } else {  /* there is a peer, which means this is not an address owner */
    
    /* shutdown read/write capabilities and wake up threads, which are 
     * blocked in read/write operations on this socket */
    do_shutdown(s, 2);

    /* If there is still data in the write buffer, we defer the actual
     * freeing of the socket descriptor. This is done later by either
     * recv_internal() or close_internal() (when called for the peer socket) 
     */
    if (s->buf.num_bytes > 0)
      s->state = SOCKET_STATE_DRAIN_BUF | SOCKET_STATE_HAS_PEER;
    else {
      s->peer->peer   = NULL;
      s->peer->state &= ~SOCKET_STATE_HAS_PEER;
    }

    /* deferred freeing of peer socket descriptor necessary? */
    if (s->peer->state & SOCKET_STATE_DRAIN_BUF)
      free_handle(socket_handle(s->peer));

    socket_unlock(s->peer);
  }

  if (ret == 0 && !(s->state & SOCKET_STATE_DRAIN_BUF))
    free_handle(h);

  socket_unlock(s);
  return ret;
}



int bind_internal(int h, const char *addr, int addr_len) {

  int i, ret;
  socket_desc_t *s;

  s = socket_desc(h);
  socket_lock(s);

  if (s->state != SOCKET_STATE_NIL) {
    socket_unlock(s);
    return -EINVAL;
  }


  i = allocate_address(addr, s);
  if (i >= 0) {
    s->state       = SOCKET_STATE_BIND;
    s->addr_handle = i;
    ret            = 0;
  } else
    ret = -EADDRINUSE;

  socket_unlock(s);

  return ret;
}



int listen_internal(int h, int backlog) {

  socket_desc_t  *s = socket_desc(h);

  LOGd_Enter(_DEBUG_ENTER, "h=%d", h);

  socket_lock(s);

  if (s->state != SOCKET_STATE_BIND) {
    socket_unlock(s);
    return -EINVAL;
  }

  addr_lock();
  if (addr_table[s->addr_handle].handle != h) {
    addr_unlock();
    socket_unlock(s);
    return -EINVAL;
  }

  s->state = SOCKET_STATE_LISTEN;
  if (backlog > MAX_BACKLOG) {
    backlog = MAX_BACKLOG;
    LOGd(_DEBUG, "backlog clamped to %d", MAX_BACKLOG);
  }
  s->backlog                  = backlog;
  s->connect_queue.accept_sem = L4SEMAPHORE_LOCKED;
  s->connect_queue.count      = 0;
  s->connect_queue.first      = s->connect_queue.last = NULL;

  addr_unlock();
  socket_unlock(s);

  return 0;
}



int connect_internal(job_info_t *job, int h, const char *addr, int addr_len) {

  socket_desc_t *s, *peer;
  int block;

  LOGd_Enter(_DEBUG_ENTER, "h=%d", h);

  peer = address_owner(addr);
  s    = socket_desc(h);

  if (peer == NULL) {
    LOGd(_DEBUG, "unknown address");
    return -EADDRNOTAVAIL;
  }

  socket_lock(peer);

  if ((peer->state & SOCKET_STATE_LISTEN) == 0 ||
      peer->connect_queue.count >= peer->backlog) {
    socket_unlock(peer);
    return -ECONNREFUSED;
  }

  socket_lock(s);

  if (s->stream == 0) {
    socket_unlock(s);
    socket_unlock(peer);
    return -EAFNOSUPPORT;
  }

  if (s->state != SOCKET_STATE_NIL) {
    socket_unlock(s);
    socket_unlock(peer);
    if (s->state & SOCKET_STATE_CONNECT)
      return -EISCONN; 
    if (s->state & SOCKET_STATE_CONNECTING)
      return -EALREADY; 
    return -EINVAL; 
  }

  job->handle = h;

  /* enqueue this request */
  enqueue_connect(peer, job);  
  s->state       = SOCKET_STATE_CONNECTING;
  s->connect_sem = L4SEMAPHORE_LOCKED;

  l4semaphore_up(&peer->connect_queue.accept_sem);
  send_select_notification(peer, SELECT_READ);

  /* decide, whether this connect() should wait for an accept() or fail
     with EINPROGRESS */
  if ((s->state & SOCKET_STATE_NONBLOCK) == 0 ||  /* <- blocking mode      */
      ((s->state & SOCKET_STATE_NONBLOCK) &&      /* <- non-blocking, but  */
       (peer->state & SOCKET_STATE_ACCEPTING) &&  /*    accept() waiting   */
       peer->connect_queue.count == 1))           /*    for this connect() */
    block = 1;
  else
    block = 0;

  socket_unlock(s);
  socket_unlock(peer);

  if (block == 0) {
    /* FIXME: CONNECT_TIMEOUT not honored, might block forever */
    return -EINPROGRESS; /* connect() asynchronously */
  }

  /* now we wait until someone accept()s our request ... */
  if (l4semaphore_down_timed(&s->connect_sem, CONNECT_TIMEOUT) != 0) {
    /* connect() timed out */
    socket_lock(peer);
    socket_lock(s);
    if (s->peer == peer) {
      /* We were not connected, not even shortly after sem_down_timed(). */
      /* Remove this socket from peer's connect queue and reset its block
       * semaphore. */
      dequeue_connect(peer, job);
      if (l4semaphore_try_down(&peer->connect_queue.accept_sem) == 0)
        LOG_Error("could not decrement accept() sockets semaphore, but I should be able to do it");

      /* reset our own state */
      s->state = SOCKET_STATE_NIL;
      socket_unlock(s);
      socket_unlock(peer);
      return -ETIMEDOUT;
    }
    socket_unlock(s);
    socket_unlock(peer);
  }

  return 0;
}



int accept_internal(job_info_t *job, int h, const char *addr, int *addr_len) {

  int new_h, ret, block;
  socket_desc_t *s, *new_s;

  LOGd_Enter(_DEBUG_ENTER, "h=%d", h);

  s = socket_desc(h);
  socket_lock(s);

  if (s->stream == 0) {
    socket_unlock(s);
    return -EAFNOSUPPORT;
  }

  if (s->state != SOCKET_STATE_LISTEN) {
    socket_unlock(s);
    return -EINVAL;
  }

  new_h = allocate_handle();
  if (new_h < 0) {
    socket_unlock(s);
    return -ENFILE;
  }
  new_s = socket_desc(new_h);
  new_s->state = SOCKET_STATE_ACCEPTING;

  /* decide, whether this accept() should wait for a connect() or fail
     with EWOULDBLOCK */
  if ((s->state & SOCKET_STATE_NONBLOCK) && s->connect_queue.count == 0)
    block = 0;
  else
    block = 1;

  socket_unlock(s);

  if (block == 0) {
    free_handle(new_h);
    return -EWOULDBLOCK;
  } else {
    /* blocks, if there is no connect() pending */
    l4semaphore_down(&s->connect_queue.accept_sem);
  }

  /* continue, now there is work to do */
  socket_lock(s);
  
  if (s->connect_queue.count > 0) {

    job_info_t    *connect_job = dequeue_connect(s, NULL);
    socket_desc_t *connect_s   = socket_desc(connect_job->handle);

    new_s->peer   = connect_s;
    new_s->state  = SOCKET_STATE_ACCEPT | SOCKET_STATE_HAS_PEER |
                    SOCKET_STATE_SEND | SOCKET_STATE_RECV;
    new_s->flags  = s->flags;
    new_s->stream = s->stream;
    init_buf(new_s);
    
    /* finalize peer socket's connect() and wake up worker thread */
    socket_lock(connect_s);
    connect_s->peer  = new_s;
    connect_s->state = SOCKET_STATE_CONNECT | SOCKET_STATE_HAS_PEER |
                       SOCKET_STATE_SEND | SOCKET_STATE_RECV;
    init_buf(connect_s);

    l4semaphore_up(&connect_s->connect_sem);
    send_select_notification(connect_s, SELECT_WRITE);
    socket_unlock(connect_s);
    
    ret = new_h;
    LOGd(_DEBUG, "accept()ed %d on %d for %d", socket_handle(connect_s), new_h, h);

  } else {
    ret = -EINVAL;
    free_handle(new_h);
    LOGd(_DEBUG, "no connect()s pending for %d, but someone woke us up", h);
  }

  socket_unlock(s);

  return ret;
}



int send_internal(job_info_t *job, int h, const char *msg, int len, int flags) {

  /* FIXME: there are zillions of error conditions, that could be reported */
  int ret, to_write, to_write_now;
  int has_peer, exit_loop;
  socket_desc_t *s = socket_desc(h);

  LOGd_Enter(_DEBUG_ENTER, "h=%d, len=%d", h, len);

  if (socket_lock_peers(s) < 0) {
    int st;
    socket_lock(s);
    st = s->state;
    socket_unlock(s);
    if (st & (SOCKET_STATE_CONNECT | SOCKET_STATE_ACCEPT))
      return -EPIPE; /* peer closed */
    return -ENOTCONN;
  }

  if (flags != 0)
    LOG("Support for flags != 0 not implemented, ignoring it.");

  if ( !can_send(s)) {
    int st = s->state;

    socket_unlock(s);
    socket_unlock(s->peer);

    LOGd(_DEBUG, "can't send; s->state=%d", st);
    if ((st & (SOCKET_STATE_ACCEPT | SOCKET_STATE_CONNECT)) == 0)
      return -ENOTCONN;
    if ((st & SOCKET_STATE_HAS_PEER) == 0)
      return -EPIPE;
    if ((st & SOCKET_STATE_SEND) == 0)
      return -EPIPE;
    return 0; /* shut up compiler */
  }

  to_write  = len;
  exit_loop = 0;
  has_peer  = 1;

  while (to_write > 0 && !exit_loop && can_send(s)) {

    int n0, n1;

    /* determine the number of bytes that currently fit into the buffer */
    to_write_now = MIN(to_write, SOCKET_BUFFER_SIZE - s->buf.num_bytes);

    if (to_write_now > 0) {
      /* We do up to two memcpy()s, if the buffer does not start at bytes[0], which
       * means, there might be a wrap around. n0 is the the number of bytes from
       * bytes[buf.w_start] to the end of the buffer, n1 is from bytes[0]. */
      n0 = MIN(to_write_now, SOCKET_BUFFER_SIZE - s->buf.w_start);
      n1 = to_write_now - n0;
      
      LOGd(_DEBUG, "to_write: %d of %d; w_start=%d, num=%d; n0=%d, n1=%d",
           to_write_now, len, s->buf.w_start, s->buf.num_bytes, n0, n1);
      
      memcpy(&s->buf.bytes[s->buf.w_start], msg, n0);
      if (n1 > 0)
        memcpy(&s->buf.bytes[0], msg + n0, n1);
      
      s->buf.num_bytes += to_write_now;
      msg              += to_write_now;
      to_write         -= to_write_now;
      s->buf.w_start    = (s->buf.w_start + to_write_now) % SOCKET_BUFFER_SIZE;

      /* signal reader that there's data to be read */
      if (s->buf.read_blocked)
        l4semaphore_up(&s->buf.read_sem);
      else
        send_select_notification(s->peer, SELECT_READ);

    } else {
      LOGd(_DEBUG, "buffer full, waiting ...");
    }

    if ( !can_recv(s->peer)) {
      exit_loop = 1;
    }

    if (!exit_loop && to_write > 0 &&
        ((s->state & SOCKET_STATE_NONBLOCK && s->buf.read_blocked) ||
         (s->state & SOCKET_STATE_NONBLOCK) == 0)) {
      s->buf.write_blocked = 1;
      socket_unlock(s);
      socket_unlock(s->peer);

      l4semaphore_down(&s->buf.write_sem); /* wait for signal from reader */
      
      if (socket_lock_peers(s) < 0) {
        exit_loop = 1;
        has_peer  = 0;
      } else
        s->buf.write_blocked = 0;      

    } else
      exit_loop = 1;
  }
  
  ret = len - to_write;
  LOGd(_DEBUG, "sent %d bytes (h:%d->%d); w_start=%d, num_bytes=%d",
       ret, socket_handle(s), socket_handle(s->peer), s->buf.w_start, s->buf.num_bytes);
  
  if (has_peer) {
    socket_unlock(s);
    socket_unlock(s->peer);
  }
      
  return ret;
}



int recv_internal(job_info_t *job, int h, char *msg, int *len, int flags) {

  /* FIXME: there are zillions of error conditions, that could be reported */
  int ret, to_read, to_read_now;
  int has_peer, exit_loop;
  socket_desc_t *s = socket_desc(h);

  LOGd_Enter(_DEBUG_ENTER, "h=%d", h);

  if (socket_lock_peers(s) < 0) {
    int st;
    socket_lock(s);
    st = s->state;
    socket_unlock(s);
    if (st & (SOCKET_STATE_CONNECT | SOCKET_STATE_ACCEPT))
      return 0; /* EOF; peer closed */
    return -ENOTCONN;
  }

  if (flags != 0)
    LOG("Support for flags != 0 not implemented, ignoring it.");

  if ( !can_recv(s)) {
    int st = s->state;

    socket_unlock(s);
    socket_unlock(s->peer);

    if ((st & (SOCKET_STATE_ACCEPT | SOCKET_STATE_CONNECT)) == 0)
      return -ENOTCONN;
    return 0; /* EOF */
  }

  to_read   = *len;
  has_peer  = 1;
  exit_loop = 0;

  while (to_read > 0 && !exit_loop && can_recv(s)) {

    int n0, n1;

    to_read_now = MIN(to_read, s->peer->buf.num_bytes);

    if (to_read_now > 0) {
      /* We do up to two memcpy()s, if the buffer does not start at bytes[0], which
       * means, there might be a wrap around. n0 is the the number of bytes from
       * bytes[buf.r_start] to the end of the buffer, n1 is from bytes[0]. */
      n0 = MIN(to_read_now, SOCKET_BUFFER_SIZE - s->peer->buf.r_start);
      n1 = to_read_now - n0;
      
      LOGd(_DEBUG, "to_read_now: %d of %d; r_start=%d, num=%d; n0=%d, n1=%d",
           to_read_now, *len, s->peer->buf.r_start, s->peer->buf.num_bytes, n0, n1);
      
      memcpy(msg, &s->peer->buf.bytes[s->peer->buf.r_start], n0);      
      if (n1 > 0)
        memcpy(msg + n0, &s->peer->buf.bytes[0], n1);
      
      s->peer->buf.num_bytes -= to_read_now;
      to_read                -= to_read_now;
      msg                    += to_read_now;
      s->peer->buf.r_start    = (s->peer->buf.r_start + to_read_now) % SOCKET_BUFFER_SIZE;

    } else if ( !(s->peer->state & SOCKET_STATE_DRAIN_BUF)) {
      LOGd(_DEBUG, "buffer empty, waiting ...");
    }

    /* signal writer that there is some space in the buffer */
    if (s->peer->buf.num_bytes < SOCKET_BUFFER_SIZE) {
      if (s->peer->buf.write_blocked)
        l4semaphore_up(&s->peer->buf.write_sem);
      else
        send_select_notification(s, SELECT_WRITE);
    }

    if (s->peer->buf.num_bytes == 0 && !can_send(s->peer))
      exit_loop = 1;

    if (!exit_loop && to_read > 0 &&
        ((s->state & SOCKET_STATE_NONBLOCK) == 0 ||
         (s->state & SOCKET_STATE_NONBLOCK && s->peer->buf.write_blocked))) {
      /* block until there is more data */
      s->peer->buf.read_blocked = 1;
      socket_unlock(s->peer);
      socket_unlock(s);

      l4semaphore_down(&s->peer->buf.read_sem); /* wait for signal from writer */

      /* regain locks; this should only fail, if someone called close() */
      if (socket_lock_peers(s) < 0) {
        has_peer  = 0;
        exit_loop = 1;
      } else
        s->peer->buf.read_blocked = 0;      

    } else
      exit_loop = 1;
  }

  ret  = *len - to_read;
  *len = ret;

  /* s->peer might not be available */
  /*LOGd(_DEBUG, "recv'ed %d bytes (h:%d<-%d); r_start=%d, num_bytes=%d",
    ret, socket_handle(s), socket_handle(s->peer), s->peer->buf.r_start,
    s->peer->buf.num_bytes);*/

  if (has_peer) {

    /* deferred freeing of peer socket descriptor necessary? */
    if (s->peer->state & SOCKET_STATE_DRAIN_BUF &&
        s->peer->buf.num_bytes == 0) {

      /* deferred_close_peer() finalizes s->peer's close(), which was deferred
       * because there was still data in its write buffer (which we read here).
       * This function will also take care of unlocking s and s->peer. */
      deferred_close_peer(s);

    } else {

      socket_unlock(s->peer);
      socket_unlock(s);
    }
  }

  return ret;
}



int fcntl_internal(int h, int cmd, long arg) {

  socket_desc_t *s = socket_desc(h);
  int ret = 0;

  LOGd_Enter(_DEBUG_ENTER, "h=%d", h);

  socket_lock(s);

  switch (cmd) {
  case F_GETFL:
    if (s->state & SOCKET_STATE_NONBLOCK)
      ret |= O_NDELAY;
    break;
  case F_SETFL:
    if (arg & O_NDELAY)
      s->state |= SOCKET_STATE_NONBLOCK;
    break;
  default:
    LOGd(_DEBUG, "unknown cmd");
    ret = -EINVAL;
  }

  socket_unlock(s);
  return ret;
}



int ioctl_internal(int h, int cmd, char **arg, int *count) {

  socket_desc_t *s = socket_desc(h);
  /* long *long_arg_ptr; */
  int  *int_arg_ptr;
  int  ret = 0;

  LOGd_Enter(_DEBUG_ENTER, "h=%d", h);

  switch (cmd) {

  case FIONREAD:
    int_arg_ptr = (int *) *arg;         /* out parameter is an int */
    *count      = sizeof(*int_arg_ptr); /* with this size */

    if (socket_lock_peers(s) == 0) {
      *int_arg_ptr = bytes_to_recv(s);
      LOGd(_DEBUG_SELECT, "%d bytes available for reading on %d", *int_arg_ptr, h);
      socket_unlock(s->peer);
      socket_unlock(s);
    } else
      ret = -EINVAL;
    break;

  default:
    LOGd(_DEBUG, "unknown cmd");
    ret = -EINVAL;
  }

  return ret;
}

/* ******************************************************************* */
/* ******************************************************************* */

void register_select_notify(int h, l4_threadid_t client, int mode) {

  socket_desc_t *s = socket_desc(h);
  int notify_now_mode = 0;
  int peer_locked = 1;

  LOGd_Enter(_DEBUG_ENTER, "h=%d", h);

  if (socket_lock_peers(s) < 0) {
    socket_lock(s);
    peer_locked = 0;
  }

  LOGd(_DEBUG_SELECT, "notification request from %d.%d; mode=%d; h=%d ",
       client.id.task, client.id.lthread, mode, h);

  if (mode & SELECT_READ) {
    if (enqueue_notify(&s->read_notify, client) == 0 &&
        !would_block(s, SELECT_READ))
      notify_now_mode |= SELECT_READ;
  }

  if (mode & SELECT_WRITE) {
    if (enqueue_notify(&s->write_notify, client) == 0 &&
        !would_block(s, SELECT_WRITE))
      notify_now_mode |= SELECT_WRITE;
  }

  if (mode & SELECT_EXCEPTION) {
    /* FIXME: do we need this? */
    LOG("select() for exception fds not implemented; ignoring it");
  }

  if (notify_now_mode) {
    /* FIXME: We still have the lock here, but we do not send the
     * notification with send_timeout = 0. That's bad. */
    send_select_notification(socket_desc(h), notify_now_mode);
  }

  socket_unlock(s);
  if (peer_locked)
    socket_unlock(s->peer);
}



void deregister_select_notify(int h, l4_threadid_t client, int mode) {

  socket_desc_t *s = socket_desc(h);

  LOGd_Enter(_DEBUG_ENTER, "h=%d", h);
  socket_lock(s);

  if (mode & SELECT_READ)
    dequeue_notify(&s->read_notify, client);
  if (mode & SELECT_WRITE)
    dequeue_notify(&s->write_notify, client);
  if (mode & SELECT_EXCEPTION)
    dequeue_notify(&s->except_notify, client);

  socket_unlock(s);
}



static void send_select_notification(socket_desc_t *s, int mode) {

  l4_threadid_t  c;
  notify_queue_t *q;
  int h = socket_handle(s);

  switch (mode) {
  case SELECT_READ:
    q = &s->read_notify;
    break;
  case SELECT_WRITE:
    q = &s->write_notify;
    break;
  case SELECT_EXCEPTION:
    q = &s->except_notify;
    break;
  default:
    LOG("invalid mode for select() notification");
    return;
  }

  while ( !notify_queue_is_empty(q)) {

    c = dequeue_notify(q, L4_INVALID_ID);
    l4vfs_select_listener_send_notification(c, h, mode);

    LOGd(_DEBUG_SELECT, "sent select() notification to %d.%d; h=%d; mode=%d",
         c.id.task, c.id.lthread, h, mode);
  }
}

/* ******************************************************************* */
/* ******************************************************************* */

static int socket_lock_peers(socket_desc_t *s) {

  socket_desc_t *s0, *s1;

  s0 = s;
  while (1) {

    l4semaphore_down(&s0->lock);

    if ((s0->state & SOCKET_STATE_HAS_PEER) == 0) {
      socket_unlock(s0);
      return -1; /* not connected ...  */
    }

    s1 = s0->peer;
    if (s1 == NULL) {
      l4semaphore_up(&s0->lock);
      return -1; /* no peer */
    }

    if (l4semaphore_try_down(&s1->lock)) {

      if (s0 == s1->peer && (s1->state & SOCKET_STATE_HAS_PEER)) {
        return 0; /* success */
      }

      /* not connected */
      //LOG_Error("inconsistant state: s0=%p, s1=%p, s0->peer=%p, s1->peer=%p, h0=%d, h1=%d, s0->state=%x, s1->state=%x",
      //    s0, s1, s0->peer, s1->peer, socket_handle(s0), socket_handle(s1), s0->state, s1->state);
      l4semaphore_up(&s0->lock);
      l4semaphore_up(&s1->lock);
      return -1; 
    }

    l4semaphore_up(&s0->lock);
    s0 = s1;
  }
}


static int would_block(socket_desc_t *s, int mode) {

  if (mode == SELECT_READ) {
    /* accept() */
    if (s->state & SOCKET_STATE_LISTEN)
      return (s->connect_queue.count > 0) ? 0 : 1;

    /* recv() */
    else if (can_recv(s)) {

      if (s->peer == NULL)
        return 0; /* FIXME: connection reset; how is this to be handled in select()? */
      
      if ((s->peer->buf.num_bytes > 0) && !s->peer->buf.read_blocked)
        return 0;

    } else
      return 0;
  }

  if (mode == SELECT_WRITE) {
    /* (async) connect() is done */
    if (s->state & SOCKET_STATE_CONNECT)
      return 0;

    /* send() */
    else if (can_send(s)) {
      if (s->buf.num_bytes < SOCKET_BUFFER_SIZE && !s->buf.write_blocked)
        return 0;
    } else
      return 0;
  }

  return 1;
}


static int bytes_to_recv(socket_desc_t *s) {

  if (can_recv(s))
    return s->peer->buf.num_bytes;

  return 0;
}


static void init_buf(socket_desc_t *s) {

  s->buf.read_sem  = L4SEMAPHORE_LOCKED;
  s->buf.write_sem = L4SEMAPHORE_LOCKED;
  s->buf.num_bytes = 0;
  s->buf.r_start   = 0;
  s->buf.w_start   = 0;
  s->buf.write_blocked = 0;
  s->buf.read_blocked  = 0;
}


static inline int can_recv(socket_desc_t *s) {

  int state = s->state;

  if ((state & SOCKET_STATE_HAS_PEER) &&
      (state & SOCKET_STATE_RECV))
    return 1;

  return 0;
}


static inline int can_send(socket_desc_t *s) {

  int state = s->state;

  if ((state & SOCKET_STATE_HAS_PEER) &&
      (state & SOCKET_STATE_SEND))
    return 1;

  return 0;
}


static int do_shutdown(socket_desc_t *s, int how) {

  int err = 0;

  if (how == 0 || how == 2) {
    if (s->state & SOCKET_STATE_RECV)
      s->state &= ~SOCKET_STATE_RECV;
    else
      err += 1;
  }

  if (how == 1 || how == 2) {
    if (s->state & SOCKET_STATE_SEND)
      s->state &= ~SOCKET_STATE_SEND;
    else
      err += 1;
  }

  if (s->buf.read_blocked)
    l4semaphore_up(&s->buf.read_sem);
  if (s->peer->buf.write_blocked)
    l4semaphore_up(&s->peer->buf.write_sem);

  return err;
}


static void deferred_close_peer(socket_desc_t *s) {

  socket_desc_t *p;
  
  p         = s->peer;
  s->peer   = p->peer = NULL;
  s->state &= ~SOCKET_STATE_HAS_PEER;
  p->state &= ~SOCKET_STATE_HAS_PEER;
  
  /* perform the normal unlock() operations */
  socket_unlock(p);
  socket_unlock(s);
  
  /* We must not grab addr lock while holding any socket lock! */
  socket_lock(p);

  free_handle(socket_handle(p));

  socket_unlock(p);
}

/* ******************************************************************* */
/* ******************************************************************* */

static int allocate_handle(void) {
  
  int i = 0;

  l4semaphore_down(&socket_table_lock);
  while (i < MAX_SOCKETS && socket_table[i].used)
    i++;

  if (i < MAX_SOCKETS) {
    socket_table[i].used  = 1;
    socket_table[i].lock  = L4SEMAPHORE_UNLOCKED;
    socket_table[i].state = SOCKET_STATE_NIL;
  } else
    i = -1;
  l4semaphore_up(&socket_table_lock);

  return i;
}

static void free_handle(int h) {

  l4semaphore_down(&socket_table_lock);
  if (h >= 0 && h < MAX_SOCKETS) {
    socket_table[h].used  = 0;
    socket_table[h].state = SOCKET_STATE_NIL;
  } else
    LOG_Error("Illegal socket handle");
  l4semaphore_up(&socket_table_lock);
}

/* ******************************************************************* */

static int allocate_address(const char *addr, socket_desc_t *s) {

  int i = 0;

  l4semaphore_down(&addr_table_lock);
  while (i < MAX_ADDRESSES && addr_table[i].ref_count > 0) {
    if (strncmp(addr, addr_table[i].sun_path, MAX_ADDRESS_LEN) == 0) {
      /* address already in use */
      l4semaphore_up(&addr_table_lock);
      return -1;
    }
    i++;
  }

  if (i < MAX_ADDRESSES) {
    strncpy(addr_table[i].sun_path, addr, MAX_ADDRESS_LEN - 1);
    addr_table[i].sun_path[MAX_ADDRESS_LEN - 1] = 0;
    addr_table[i].ref_count = 1;
    addr_table[i].handle    = socket_handle(s);
  } else
    i = -1;

  l4semaphore_up(&addr_table_lock);
  return i;
}

static void free_address(int i) {

  l4semaphore_down(&addr_table_lock);
  if (addr_table[i].ref_count > 0)
    addr_table[i].ref_count--;
  else
    LOG_Error("address already unused");
  l4semaphore_up(&addr_table_lock);
}

static socket_desc_t *address_owner(const char *addr) {

  int i = 0;
  socket_desc_t *s = NULL;

  l4semaphore_down(&addr_table_lock);
  while (i < MAX_ADDRESSES  &&
         strncmp(addr, addr_table[i].sun_path, MAX_ADDRESS_LEN) != 0)
    i++;

  if (i < MAX_ADDRESSES && addr_table[i].ref_count > 0)
    s = socket_desc(addr_table[i].handle);

  l4semaphore_up(&addr_table_lock);
  return s;
}

/* ******************************************************************* */

job_info_t *create_job_info(int type, l4_threadid_t client) {

  job_info_t *job_info = (job_info_t *) CORBA_alloc(sizeof(job_info_t));

  job_info->type    = type;
  job_info->replier = l4_myself();
  job_info->client  = client;

  return job_info;
}

/* ******************************************************************* */

#ifdef DEBUG 
static void log_connect_queue(socket_desc_t *s) {
  
  connect_node_t *n;

  n = s->connect_queue.first;
  while (n) {
    LOGd(_DEBUG, "queued for %d: %d", socket_handle(s), n->job->handle);
    n = n->next;
  }
}
#endif

static connect_node_t *enqueue_connect(socket_desc_t *s, job_info_t *job) {

  connect_node_t *n = (connect_node_t *) CORBA_alloc(sizeof(connect_node_t));

  if (n == NULL)
    return NULL;

  n->job  = job;
  n->next = NULL;
  
  if (s->connect_queue.last) {
    s->connect_queue.last->next = n;
    s->connect_queue.last       = n;
  } else
    s->connect_queue.last = s->connect_queue.first = n;

  s->connect_queue.count++;
  LOG_CONNECT_QUEUE(s);

  return n;
}

static job_info_t *dequeue_connect(socket_desc_t *s, job_info_t *job) {

  job_info_t     *j;
  connect_node_t *n, *n_prev;

  n      = s->connect_queue.first;
  n_prev = n;

  /* if job is NULL, we dequeue the first job in the queue */
  if (job) {

    while (n && n->job != job) {
      n_prev = n;
      n = n->next;    
    }
  }

  /* dequeue the requested node */
  if (n) {
    if (n == s->connect_queue.first)
      s->connect_queue.first = n->next;
    else
      n_prev->next = n->next;
    
    if (n == s->connect_queue.last) {
      s->connect_queue.last = n_prev;
      if (s->connect_queue.first == NULL)
        s->connect_queue.last = NULL;
    }

    j = n->job;
    CORBA_free(n);
    s->connect_queue.count--;

  } else {    
    LOG_Error("no such connect request was queued");
    j = NULL;
  }

  LOG_CONNECT_QUEUE(s);
  return j;
}

/* ******************************************************************* */

static int enqueue_notify(notify_queue_t *queue, l4_threadid_t client) {

  /* returns 0, if enqueued as first element in queue, !=0 else */

  notify_node_t *n = (notify_node_t *) CORBA_alloc(sizeof(notify_node_t));

  if (n == NULL)
    LOG_Error("malloc() failed");

  n->client   = client;
  n->notified = 0;
  n->next     = NULL;
  
  if (queue->last) {
    queue->last->next = n;
    queue->last       = n;
  } else
    queue->last = queue->first = n;

  return (n == queue->first) ? 0 : 1;
}

static l4_threadid_t dequeue_notify(notify_queue_t *queue, l4_threadid_t client) {

  l4_threadid_t c;
  notify_node_t *n, *n_prev;

  n      = queue->first;
  n_prev = n;

  /* find client if specified, use first in queue otherwise */
  if ( !l4_thread_equal(client, L4_INVALID_ID)) {

    while (n && !l4_thread_equal(n->client, client)) {
      n_prev = n;
      n = n->next;    
    }
  }

  if (n) {
    c = n->client;
    if (n == queue->first)
      queue->first = n->next;
    else
      n_prev->next = n->next;
    
    if (n == queue->last) {
      queue->last = n_prev;
      if (queue->first == NULL)
        queue->last = NULL;
    }

    CORBA_free(n);

  } else {
    //LOGd(_DEBUG, "client %d.%d notify request already removed from queue %p",
    //     client.id.task, client.id.lthread, queue);
    return L4_INVALID_ID;
  }
  
  return c;
}


static inline int notify_queue_is_empty(notify_queue_t *queue) {

  return queue->first == NULL;
}

/* ******************************************************************* */
/* ******************************************************************* */

void local_socks_init(void) {

  int i;

  for (i = 0; i < MAX_SOCKETS; i++)
    socket_table[i].used  = 0;

  for (i = 0; i < MAX_ADDRESSES; i++)
    addr_table[i].ref_count = 0;

  socket_table_lock = L4SEMAPHORE_UNLOCKED;
  addr_table_lock   = L4SEMAPHORE_UNLOCKED;
}

