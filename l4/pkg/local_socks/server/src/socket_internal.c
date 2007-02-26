/* $Id$ */
/*****************************************************************************/
/**
 * \file   local_socks/server/src/socket_internal.c
 * \brief  Internal socket server implementation.
 *
 * \date   15/08/2004
 * \author Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2004-2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

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
#include <l4/util/l4_macros.h>
#include <l4/l4vfs/select_listener.h>
#include <l4/env/errno.h>

/* *** LOCAL INCLUDES *** */
#include "socket_internal.h"

/* ******************************************************************* */

#ifdef DEBUG
# define _DEBUG        1
# define _DEBUG_ENTER  1
# define _DEBUG_SELECT 1
# define _DEBUG_EVENTS 1
# define LOG_CONNECT_QUEUE(h) log_connect_queue(h)
#else
# define _DEBUG        0
# define _DEBUG_ENTER  0
# define _DEBUG_SELECT 0
# define _DEBUG_EVENTS 0
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

static void        send_select_notification(socket_desc_t *s, int mode);
static int         would_block(socket_desc_t *s, int mode);
static int         bytes_to_recv(socket_desc_t *s);
static inline int  can_recv(socket_desc_t *s);
static inline int  can_send(socket_desc_t *s);
static void        try_to_close_peer(socket_desc_t *s);
static int         do_close(socket_desc_t *s);
static int         do_shutdown(socket_desc_t *s, int how);
static inline int  blocking_operation_in_progress(socket_desc_t *s);
static inline void set_peer_socket(socket_desc_t *s, socket_desc_t *peer, int state); 
static inline void free_buf(socket_desc_t *s);

static inline int    client_owns_handle(l4_threadid_t *client, int h);
static int           allocate_handle(l4_threadid_t *owner);
static void          free_handle(int h);
static int           allocate_address(const char *addr, socket_desc_t *s);
static void          free_address(int i);
static socket_desc_t *grab_address_owner(const char *addr);

static inline socket_desc_t *grab_socket_for_client(l4_threadid_t *client, int h);
static inline int            lock_socket_peers_for_client(l4_threadid_t *client, int h);
    
#ifdef DEBUG 
static void log_connect_queue(socket_desc_t *s);
#endif
static connect_node_t *enqueue_connect(socket_desc_t *s, job_info_t *job);
static job_info_t     *dequeue_connect(socket_desc_t *s, job_info_t *job);

static int           enqueue_notify(notify_queue_t *queue, const l4_threadid_t *client);
static l4_threadid_t dequeue_notify(notify_queue_t *queue, const l4_threadid_t *client);
static inline int    notify_queue_is_empty(notify_queue_t *queue);

/* ******************************************************************* */

static inline socket_desc_t *socket_desc(int h) {
  Assert(h >= 0 && h < MAX_SOCKETS);
  return &socket_table[h];
}

static inline int socket_handle(socket_desc_t *s) {
  return (int) (s - socket_table);
}

/* ******************************************************************* */

static inline int socket_try_lock(socket_desc_t *s) {
  return l4semaphore_try_down(&s->lock);
}

static inline void socket_lock(socket_desc_t *s) {
  l4semaphore_down(&s->lock);
}

static inline void socket_unlock(socket_desc_t *s) {
  l4semaphore_up(&s->lock);
}

static inline int socket_lock_peers(socket_desc_t *s);

static inline void addr_lock(void) {
  l4semaphore_down(&addr_table_lock);
}

static inline void addr_unlock(void) {
  l4semaphore_up(&addr_table_lock);
}

/* ******************************************************************* */
/* ******************************************************************* */

int socket_internal(l4_threadid_t *client, int domain, int type, int protocol) {

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

  h = allocate_handle(client);

  if (h >= 0) {
    socket_desc_t *s = socket_desc(h);
    s->stream = 1;
    s->flags  = 0;
    s->peer   = NULL;
    socket_unlock(s);
  } else
    return -ENFILE;

  return h;
}



int socketpair_internal(l4_threadid_t *client, int domain, int type,
                        int protocol, int *h0, int *h1) {
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


  *h0 = allocate_handle(client);
  if (*h0 >= 0) {
    *h1 = allocate_handle(client);
    if (*h1 < 0) {
      free_handle(*h0);
      socket_unlock(socket_desc(*h0));
    }
  }

  if (*h0 >= 0 && *h1 >= 0) {
    socket_desc_t *s0 = socket_desc(*h0);
    socket_desc_t *s1 = socket_desc(*h1);

    s0->stream = s1->stream = stream;
    s0->flags  = s1->flags  = 0;

    set_peer_socket(s0, s1, SOCKET_STATE_CONNECT);
    set_peer_socket(s1, s0, SOCKET_STATE_CONNECT);
    
    socket_unlock(s0);
    socket_unlock(s1);
    
  } else {
    return -ENFILE;
  }

  return 0;
}



int shutdown_internal(l4_threadid_t *client, int h, int how) {

  int ret;

  LOGd_Enter(_DEBUG_ENTER, "h=%d", h);

  ret = lock_socket_peers_for_client(client, h);
  if (ret == 0) {

    socket_desc_t *s = socket_desc(h);
    
    if (do_shutdown(s, how) >= how)
      ret = -ENOTCONN;
    socket_unlock(s->peer);
    socket_unlock(s);

  } else if (ret != -EBADF)
    socket_unlock(socket_desc(h));
    

  return ret;
}


int close_internal(l4_threadid_t *client, int h) {

  int ret, peer_is_locked = 1;
  socket_desc_t *s;

  LOGd_Enter(_DEBUG_ENTER, "h=%d", h);

  ret = lock_socket_peers_for_client(client, h);
  if (ret == -EBADF)
    return ret;  

  if (ret < 0)
    peer_is_locked = 0;
  
  s   = socket_desc(h);
  ret = do_close(s);
  
  if (peer_is_locked)
    socket_unlock(s->peer);
  socket_unlock(s);

  return ret;
}



int bind_internal(l4_threadid_t *client, int h, const char *addr, int addr_len) {

  int i, ret;
  socket_desc_t *s = grab_socket_for_client(client, h);
  if (s == NULL)
    return -EBADF;

  if (s->state != SOCKET_STATE_NIL) {
    socket_unlock(s);
    return -EINVAL;
  }


  i = allocate_address(addr, s);
  if (i >= 0) {
    set_basic_state(s, SOCKET_STATE_BIND);
    s->addr_handle = i;
    ret            = 0;
  } else
    ret = -EADDRINUSE;

  socket_unlock(s);

  return ret;
}



int listen_internal(l4_threadid_t *client, int h, int backlog) {

  socket_desc_t *s;

  LOGd_Enter(_DEBUG_ENTER, "h=%d", h);
          
  s = grab_socket_for_client(client, h);
  if (s == NULL)
    return -EBADF;

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

  set_basic_state(s, SOCKET_STATE_LISTEN);
  if (backlog > MAX_BACKLOG) {
    backlog = MAX_BACKLOG;
    LOGd(_DEBUG, "backlog clamped to %d", MAX_BACKLOG);
  }
  s->backlog             = backlog;
  s->num_accepts         = 0;
  s->connect_queue.count = 0;
  s->connect_queue.first = s->connect_queue.last = NULL;
  s->operation_sem       = L4SEMAPHORE_LOCKED;

  addr_unlock();
  socket_unlock(s);

  return 0;
}



int connect_internal(job_info_t *job, int h, const char *addr, int addr_len) {

  socket_desc_t *s, *peer;
  int ret, block, timeout;

  LOGd_Enter(_DEBUG_ENTER, "h=%d", h);

  peer = grab_address_owner(addr);
  if (peer == NULL) {
    LOGd(_DEBUG, "unknown address '%s'", addr);
    return -EADDRNOTAVAIL;
  }
  addr_unlock(); /* locked by grab_addr_owner() */

  if ((peer->state & SOCKET_STATE_LISTEN) == 0 ||
      peer->connect_queue.count >= peer->backlog) {
    socket_unlock(peer);
    return -ECONNREFUSED;
  }

  s = grab_socket_for_client(&job->client, h);
  if (s == NULL) {
    socket_unlock(peer);
    return -EBADF;
  }
  
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
  s->peer          = peer;
  s->operation_sem = L4SEMAPHORE_LOCKED;
  set_sub_state_on(s, SOCKET_STATE_CONNECTING);

  l4semaphore_up(&peer->operation_sem);
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
    /* FIXME: CONNECT_TIMEOUT not honored, might try forever */
    return -EINPROGRESS; /* connect() asynchronously */
  }

  /* now we wait until someone accept()s our request ... */
  timeout = (l4semaphore_down_timed(&s->operation_sem, CONNECT_TIMEOUT) != 0);
  
  socket_lock(peer);
  socket_lock(s);

  if (s->peer == peer) {
    /* We were woken up, either because of a timeout or the connect()
     * operation has been aborted.
     * We were not connected, not even shortly after sem_down_timed(), so
     * this connect request will be removed from the peer's connect queue. */
    dequeue_connect(peer, job);

    /* reset our own state */
    set_basic_state(s, SOCKET_STATE_NIL);
    ret = -ETIMEDOUT;
  } else
    ret = 0;

  if (s->state & SOCKET_STATE_CLOSED) {
    /* FIXME: I could not figure out, what is supposed to happen, if
     * someone calls close() on a socket which is still connecting.
     * Atm, it is just closed when the connect() thread awakens. */
    do_close(s);
  }
    
  socket_unlock(s);
  socket_unlock(peer);
  
  return ret;
}



int accept_internal(job_info_t *job, int h, const char *addr, int *addr_len) {

  int new_h, block, accepted = 0;
  socket_desc_t *s, *new_s;

  LOGd_Enter(_DEBUG_ENTER, "h=%d", h);

  s = grab_socket_for_client(&job->client, h);
  if (s == NULL)
    return -EBADF;
  
  if (s->stream == 0) {
    socket_unlock(s);
    return -EAFNOSUPPORT;
  }

  if (s->state != SOCKET_STATE_LISTEN) {
    socket_unlock(s);
    return -EINVAL;
  }

  new_h = allocate_handle(&job->client);
  if (new_h < 0) {
    socket_unlock(s);
    return -ENFILE;
  }
  
  new_s = socket_desc(new_h);
  set_sub_state_on(new_s, SOCKET_STATE_ACCEPTING);

  /* decide, whether this accept() should wait for a connect() or fail
     with EWOULDBLOCK */
  if ((s->state & SOCKET_STATE_NONBLOCK) && s->connect_queue.count == 0)
    block = 0;
  else {
    block = 1;
    s->num_accepts++;
    if (s->num_accepts == 1)
      set_sub_state_on(s, SOCKET_STATE_ACCEPTING);
  }

  socket_unlock(s);
  socket_unlock(new_s);

  if (block == 0) {
    free_handle(new_h);
    socket_unlock(new_s);
    return -EWOULDBLOCK;
  }

  do {
    /* there may be more than one iterations of this loop, if another thread
     * accepted a connection while this thread was blocked and a second
     * connection request was dequeued because of a timeout */
    
    /* blocks, if there is no connect() pending */
    l4semaphore_down(&s->operation_sem);
    
    /* continue, now there is work to do */
    socket_lock(new_s);
    socket_lock(s);

    if (new_s->state & SOCKET_STATE_CLOSED) {
      /* this can only be caused by a client that has terminated */
      do_close(new_s);
      accepted = 1;

    } else if (s->connect_queue.count > 0) {

      job_info_t    *connect_job = dequeue_connect(s, NULL);
      socket_desc_t *connect_s   = socket_desc(connect_job->handle);

      /* finalize accept() for new socket */
      set_peer_socket(new_s, connect_s, SOCKET_STATE_ACCEPT);

      /* finalize peer socket's connect() ... */
      socket_lock(connect_s);
      set_peer_socket(connect_s, new_s, SOCKET_STATE_CONNECT);

      /* ... wake the blocked connect() thread (or let select() return) */
      l4semaphore_up(&connect_s->operation_sem);
      send_select_notification(connect_s, SELECT_WRITE);

      socket_unlock(connect_s);
      socket_unlock(new_s);
      
      s->num_accepts--;
      if (s->num_accepts == 0)
        set_sub_state_off(s, SOCKET_STATE_ACCEPTING);

      accepted = 1;
      
      LOGd(_DEBUG, "accept()ed %d on %d for %d", socket_handle(connect_s), new_h, h);
      
    } else
      socket_unlock(s);
    
  } while ( !accepted);
  
  if (s->num_accepts == 0 && (s->state & SOCKET_STATE_CLOSED))
    do_close(s);
  
  socket_unlock(s);

  return new_h;
}



int send_internal(job_info_t *job, int h, const char *msg, int len, int flags) {

  int ret, to_write, to_write_now;
  int exit_loop;
  socket_desc_t *s;

  LOGd_Enter(_DEBUG_ENTER, "h=%d, len=%d", h, len);

  ret = lock_socket_peers_for_client(&job->client, h);
  if (ret == -EBADF)
    return ret;
  
  s = socket_desc(h);

  if (ret < 0) {
    if (s->state & (SOCKET_STATE_CONNECT | SOCKET_STATE_ACCEPT))
      ret = -EPIPE;    /* peer closed */
    else
      ret = -ENOTCONN; /* not connected */    
    socket_unlock(s);
    return ret;
  }

  if (flags != 0)
    LOG("Support for flags != 0 not implemented, ignoring it.");

  if ( !can_send(s))
    return -EPIPE; /* socket is shutdown for send() */
 
  to_write  = len;
  exit_loop = 0;

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

      /* block until there is some room in the buffer */
      set_sub_state_on(s, SOCKET_STATE_SENDING);
      s->buf.write_blocked = 1;
      socket_unlock(s);
      socket_unlock(s->peer);

      l4semaphore_down(&s->buf.write_sem); /* wait for signal from reader */
      
      set_sub_state_off(s, SOCKET_STATE_SENDING);
      if (socket_lock_peers(s) == 0) {
        s->buf.write_blocked = 0;      
      } else {
        LOG_Error("failed to regain locks for send");
        return len - to_write;
      }

    } else
      exit_loop = 1;
  }
  
  ret = len - to_write;
  LOGd(_DEBUG, "sent %d bytes (h:%d->%d); w_start=%d, num_bytes=%d",
       ret, socket_handle(s), socket_handle(s->peer), s->buf.w_start, s->buf.num_bytes);
 
  if (s->state & SOCKET_STATE_CLOSED)
    do_close(s);
  
  socket_unlock(s->peer);
  socket_unlock(s);
      
  return ret;
}


int recv_internal(job_info_t *job, int h, char *msg, int *len, int flags) {

  int ret, to_read, to_read_now;
  int exit_loop;
  socket_desc_t *s, *peer;

  LOGd_Enter(_DEBUG_ENTER, "h=%d", h);

  ret = lock_socket_peers_for_client(&job->client, h);
  if (ret == -EBADF)
    return ret;
  
  s = socket_desc(h);

  if (ret < 0) {
    if (s->state & (SOCKET_STATE_CONNECT | SOCKET_STATE_ACCEPT))
      ret = 0;         /* peer closed -> EOF */
    else
      ret = -ENOTCONN; /* not connected */    
    socket_unlock(s);
    return ret;
  }

  if (flags != 0)
    LOG("Support for flags != 0 not implemented, ignoring it.");

  if ( !can_recv(s)) {
    socket_unlock(s);
    socket_unlock(s->peer);
    return 0; /* shutdown for read -> EOF */
  }

  to_read   = *len;
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

    } else if ( !(s->peer->state & SOCKET_STATE_CLOSED)) {
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
      set_sub_state_on(s, SOCKET_STATE_RECVING);
      s->peer->buf.read_blocked = 1;
      socket_unlock(s->peer);
      socket_unlock(s);

      l4semaphore_down(&s->peer->buf.read_sem); /* wait for signal from writer */
  
      set_sub_state_off(s, SOCKET_STATE_RECVING);
      if (socket_lock_peers(s) == 0) {
        s->peer->buf.read_blocked = 0;      
      } else {
        LOG_Error("failed to regain locks for recv");
        return *len - to_read;
      }
    } else
      exit_loop = 1;
  }

  ret  = *len - to_read;
  *len = ret;

  LOGd(_DEBUG, "recv'ed %d bytes (h:%d<-%d); r_start=%d, num_bytes=%d",
       ret, h, socket_handle(s->peer), s->peer->buf.r_start,
       s->peer->buf.num_bytes);
  
  peer = s->peer; /* try_to_close_peer() can set s->peer=NULL */
  if (s->peer->state & SOCKET_STATE_CLOSED)
    try_to_close_peer(s);
  if (s->state & SOCKET_STATE_CLOSED)
    do_close(s);
  
  socket_unlock(peer);
  socket_unlock(s);

  return ret;
}



int fcntl_internal(l4_threadid_t *client, int h, int cmd, long arg) {

  socket_desc_t *s;
  int ret = 0;

  LOGd_Enter(_DEBUG_ENTER, "h=%d", h);

  s = grab_socket_for_client(client, h);
  if (s == NULL)
    return -EBADF;
  
  switch (cmd) {
  case F_GETFL:
    if (s->state & SOCKET_STATE_NONBLOCK)
      ret |= O_NDELAY;
    break;
  case F_SETFL:
    if (arg & O_NDELAY)
      set_sub_state_on(s, SOCKET_STATE_NONBLOCK);
    else
      set_sub_state_off(s, SOCKET_STATE_NONBLOCK);
    break;
  default:
    LOGd(_DEBUG, "unknown cmd");
    ret = -EINVAL;
  }

  socket_unlock(s);
  return ret;
}



int ioctl_internal(l4_threadid_t *client, int h, int cmd, char **arg, int *count) {

  socket_desc_t *s;
  /* long *long_arg_ptr; */
  int  *int_arg_ptr;
  int  ret, lock_peers_ret;

  LOGd_Enter(_DEBUG_ENTER, "h=%d", h);

  lock_peers_ret = lock_socket_peers_for_client(client, h);
  if (lock_peers_ret == -EBADF)
    return lock_peers_ret;
  
  s = socket_desc(h);
  
  switch (cmd) {

  case FIONREAD:
    int_arg_ptr = (int *) *arg;         /* out parameter is an int */
    *count      = sizeof(*int_arg_ptr); /* with this size */

    if (lock_peers_ret == 0) { /* we need the peer! */
      *int_arg_ptr = bytes_to_recv(s);
      LOGd(_DEBUG_SELECT, "%d bytes available for reading on %d", *int_arg_ptr, h);
      ret = 0;
    } else
      ret = -EINVAL;
    break;

  default:
    LOGd(_DEBUG, "unknown cmd %d", cmd);
    ret = -EINVAL;
  }

  if (lock_peers_ret == 0)
    socket_unlock(s->peer);
  socket_unlock(s);

  return ret;
}

/* ******************************************************************* */
/* ******************************************************************* */

static int would_block(socket_desc_t *s, int mode) {

  if (mode == SELECT_READ) {
    /* accept() */
    if (s->state & SOCKET_STATE_LISTEN)
      return (s->connect_queue.count > 0) ? 0 : 1;

    /* recv() */
    else if (can_recv(s)) {

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


static inline int can_recv(socket_desc_t *s) {

  int state = s->state;

  if ((state & (SOCKET_STATE_ACCEPT | SOCKET_STATE_CONNECT)) &&
      (state & SOCKET_STATE_HAS_PEER) &&
      (state & SOCKET_STATE_RECV))
    return 1;

  return 0;
}


static inline int can_send(socket_desc_t *s) {

  int state = s->state;

  if ((state & (SOCKET_STATE_ACCEPT | SOCKET_STATE_CONNECT)) &&
      (state & SOCKET_STATE_HAS_PEER) &&
      (state & SOCKET_STATE_SEND))
    return 1;

  return 0;
}


static int do_shutdown(socket_desc_t *s, int how) {

  int err = 0;

  if (how == 0 || how == 2) {
    if (s->state & SOCKET_STATE_RECV)
      set_sub_state_off(s, SOCKET_STATE_RECV);
    else
      err += 1;
  }

  if (how == 1 || how == 2) {
    if (s->state & SOCKET_STATE_SEND)
      set_sub_state_off(s, SOCKET_STATE_SEND);
    else
      err += 1;
  }

  if (s->buf.read_blocked)
    l4semaphore_up(&s->buf.read_sem);
  if (s->peer->buf.write_blocked)
    l4semaphore_up(&s->peer->buf.write_sem);

  return err;
}


static int do_close(socket_desc_t *s) {

  int do_free = 0;

  if ( !(s->state & SOCKET_STATE_CLOSED) &&
       blocking_operation_in_progress(s)) {
    
    set_sub_state_on(s, SOCKET_STATE_CLOSED);
    return 0;

  } else if (s->state & SOCKET_STATE_HAS_PEER) {

    /* there is a peer, which means this is not an address owner */    
    
    /* shutdown read/write capabilities and wake up threads, which are 
     * blocked in read/write operations on this socket's peer */
    do_shutdown(s, 2);

    if (s->peer->state & SOCKET_STATE_CLOSED) {
      
      /* deferred freeing of peer socket descriptor necessary? */
      free_buf(s->peer);
      free_handle(socket_handle(s->peer));

    } else if (s->buf.num_bytes > 0) {
      
      /* If there is still data in the write buffer, we defer the actual
       * freeing of the socket descriptor. This is done later by either
       * recv_internal() or close_internal() (when called for the peer
       * socket, also see the if-branch above) 
       */
      s->state = SOCKET_STATE_CLOSED | SOCKET_STATE_HAS_PEER;
      
    } else {
      s->peer->peer = NULL;
      set_sub_state_off(s->peer, SOCKET_STATE_HAS_PEER);
      do_free = 1;
    }

  } else {
    
    if (s->state & (SOCKET_STATE_LISTEN | SOCKET_STATE_BIND))
      free_address(s->addr_handle); /* address owner! */
    
    do_free = 1;
  }

  if (do_free) {
    free_buf(s);
    free_handle(socket_handle(s));
  }

  return 0;
}


static void try_to_close_peer(socket_desc_t *s) {

  socket_desc_t *p;
  
  /* deferred freeing of peer socket descriptor necessary? */
  if (s->peer->buf.num_bytes == 0) {

    LOGd_Enter(_DEBUG_ENTER, "h=%d", socket_handle(s));
    
    p       = s->peer;
    s->peer = p->peer = NULL;
    set_sub_state_off(s, SOCKET_STATE_HAS_PEER);
    set_sub_state_off(p, SOCKET_STATE_HAS_PEER);
  
    free_buf(p);
    free_handle(socket_handle(p));
  }
}


static inline int blocking_operation_in_progress(socket_desc_t *s) {

  return (s->state & (SOCKET_STATE_CONNECTING | SOCKET_STATE_ACCEPTING |
                      SOCKET_STATE_SENDING | SOCKET_STATE_RECVING)) != 0;
}

/* ******************************************************************* */

static inline void init_buf(socket_desc_t *s) {

  s->buf.bytes = CORBA_alloc(SOCKET_BUFFER_SIZE);
  if (s->buf.bytes == NULL) {
    /* FIXME */
    LOG_Error("Failed to allocate socket buffer!");    
  }
  s->buf.read_sem      = L4SEMAPHORE_LOCKED;
  s->buf.write_sem     = L4SEMAPHORE_LOCKED;
  s->buf.num_bytes     = 0;
  s->buf.r_start       = 0;
  s->buf.w_start       = 0;
  s->buf.write_blocked = 0;
  s->buf.read_blocked  = 0;
  s->serial_r_sem      = L4SEMAPHORE_UNLOCKED;
  s->serial_w_sem      = L4SEMAPHORE_UNLOCKED;
}


static inline void free_buf(socket_desc_t *s) {

  if (s->buf.bytes) {
    CORBA_free(s->buf.bytes);
    s->buf.bytes = NULL;
  }
}


static inline void set_peer_socket(socket_desc_t *s, socket_desc_t *peer, int state) {  
  
  s->peer = peer;
  set_basic_state(s, state);
  set_sub_state_on(s, SOCKET_STATE_HAS_PEER | SOCKET_STATE_SEND | SOCKET_STATE_RECV);
  set_sub_state_off(s, SOCKET_STATE_ACCEPTING | SOCKET_STATE_CONNECTING);
  init_buf(s);
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
/* ******************************************************************* */

void register_select_notify(l4_threadid_t *client, int h,
                            const l4_threadid_t *notif_tid, int mode) {

  socket_desc_t *s = socket_desc(h);
  int ret;
  int notify_now_mode = 0;
  int peer_locked     = 1;

  LOGd_Enter(_DEBUG_ENTER, "h=%d", h);

  ret = lock_socket_peers_for_client(client, h);
  if (ret == -EBADF)
    return;
  
  if (ret < 0) {
    /* ATTENTION: would_block() must not assume that a valid peer
     * is locked, but only 's' (atm., this is true, as it can
     * determine on its own whether or not there is a peer) */
    peer_locked = 0;
  }

  LOGd(_DEBUG_SELECT, "notification request from "l4util_idfmt
                      "; mode=%d; h=%d ", l4util_idstr(*notif_tid), mode, h);

  if (mode & SELECT_READ) {
    if (enqueue_notify(&s->read_notify, notif_tid) == 0 &&
        !would_block(s, SELECT_READ))
      notify_now_mode |= SELECT_READ;
  }

  if (mode & SELECT_WRITE) {
    if (enqueue_notify(&s->write_notify, notif_tid) == 0 &&
        !would_block(s, SELECT_WRITE))
      notify_now_mode |= SELECT_WRITE;
  }

  if (mode & SELECT_EXCEPTION) {
    /* FIXME: do we need this? */
    LOG("select() for exception fds not implemented; ignoring it");
  }

  if (notify_now_mode) {
    /* FIXME: We still have the lock here, but we do not send the
     * notification with send_timeout != 0. That's bad. */
    send_select_notification(socket_desc(h), notify_now_mode);
  }

  socket_unlock(s);
  if (peer_locked)
    socket_unlock(s->peer);
}



void deregister_select_notify(l4_threadid_t *client, int h,
                              const l4_threadid_t *notif_tid, int mode) {

  socket_desc_t *s = grab_socket_for_client(client, h);

  LOGd_Enter(_DEBUG_ENTER, "h=%d", h);

  if (s == NULL)
    return;

  if (mode & SELECT_READ)
    dequeue_notify(&s->read_notify, notif_tid);
  if (mode & SELECT_WRITE)
    dequeue_notify(&s->write_notify, notif_tid);
  if (mode & SELECT_EXCEPTION)
    dequeue_notify(&s->except_notify, notif_tid);

  socket_unlock(s);
}



static void send_select_notification(socket_desc_t *s, int mode) {

  l4_threadid_t  c;
  notify_queue_t *q;
  int h = socket_handle(s);

  while (mode) {
  
    int m;
      
    if (mode & SELECT_READ) {
      mode &= ~SELECT_READ;
      m     = SELECT_READ;
      q     = &s->read_notify;

    } else if (mode & SELECT_WRITE) {
      mode &= ~SELECT_WRITE;
      m     = SELECT_WRITE;
      q     = &s->write_notify;

    } else if (mode & SELECT_EXCEPTION) {
      mode &= ~SELECT_EXCEPTION;
      m     = SELECT_EXCEPTION;
      q     = &s->except_notify;

    } else {
      enter_kdebug();
      return; /* should never happen */
    }

    while ( !notify_queue_is_empty(q)) {
      
      c = dequeue_notify(q, NULL);
      l4vfs_select_listener_send_notification(c, h, m);
      
      LOGd(_DEBUG_SELECT, "sent select() notification to "l4util_idfmt
                          "; h=%d; mode=%d", l4util_idstr(c), h, m);
    }
  }
}

/* ******************************************************************* */

static int enqueue_notify(notify_queue_t *queue, const l4_threadid_t *client) {

  /* returns 0, if enqueued as first element in queue, !=0 else */

  notify_node_t *n = (notify_node_t *) CORBA_alloc(sizeof(notify_node_t));

  if (n == NULL)
    LOG_Error("malloc() failed");

  n->client   = *client;
  n->notified = 0;
  n->next     = NULL;
  
  if (queue->last) {
    queue->last->next = n;
    queue->last       = n;
  } else
    queue->last = queue->first = n;

  return (n == queue->first) ? 0 : 1;
}


static l4_threadid_t dequeue_notify(notify_queue_t *queue, const l4_threadid_t *client) {

  l4_threadid_t c;
  notify_node_t *n, *n_prev;

  n      = queue->first;
  n_prev = n;

  /* find client if specified, use first in queue otherwise */
  if (client) {

    while (n && !l4_thread_equal(n->client, *client)) {
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

static inline int handle_is_valid(int h) {
  return (h >= 0 && h < MAX_SOCKETS);
}


static inline int client_owns_handle(l4_threadid_t *client, int h) {
  socket_desc_t *s = socket_desc(h);  
  return (l4_task_equal(s->owner, *client) && s->used &&
          (s->state & SOCKET_STATE_CLOSED) == 0);
}


static inline socket_desc_t *grab_socket_for_client(l4_threadid_t *client, int h) {

  if (handle_is_valid(h)) {

    socket_desc_t *s = socket_desc(h);
    socket_lock(s);

    if (client_owns_handle(client, h))
      return s;
    socket_unlock(s);
  }  
  return NULL;
}


static int lock_socket_peers_for_client(l4_threadid_t *client, int h) {

  if (handle_is_valid(h)) {

    socket_desc_t *s = socket_desc(h);
    int ret = socket_lock_peers(s);

    if (ret == 0) {

      if (client_owns_handle(client, h))
        return 0;

      socket_unlock(s->peer);
      socket_unlock(s);
      return -EBADF;
    }

    /* must be locked for further error handling, execpt for
     * the EBADF case */
    socket_lock(s);
    if (client_owns_handle(client, h))
      return ret;

    socket_unlock(s);
  }
  return -EBADF;
}


static inline int socket_lock_peers(socket_desc_t *s) {

  socket_desc_t *s0, *s1;

  s0 = s;
  while (1) {

    socket_lock(s0);

    if ((s0->state & SOCKET_STATE_HAS_PEER) == 0) {
      socket_unlock(s0);
      return -ENOTCONN; /* not connected ...  */
    }

    s1 = s0->peer;
    if (s1 == NULL) {
      socket_unlock(s0);
      return -ENOTCONN; /* no peer */
    }

    if (socket_try_lock(s1)) {

      if (s0 == s1->peer && (s1->state & SOCKET_STATE_HAS_PEER)) {
        return 0; /* success */
      }

      /* not connected */
      //LOG_Error("inconsistant state: s0=%p, s1=%p, s0->peer=%p, s1->peer=%p, "
      //          "h0=%d, h1=%d, s0->state=%x, s1->state=%x",
      //          s0, s1, s0->peer, s1->peer, socket_handle(s0), socket_handle(s1),
      //          s0->state, s1->state);
      socket_unlock(s1);
      socket_unlock(s0);
      return -ENOTCONN;
    }

    socket_unlock(s0);
    s0 = s1;
  }
}

/* ******************************************************************* */
/* ******************************************************************* */

static int allocate_handle(l4_threadid_t *owner) {
  
  int i = 0;

  l4semaphore_down(&socket_table_lock);
  while (i < MAX_SOCKETS && socket_table[i].used)
    i++;

  if (i < MAX_SOCKETS) {
    socket_table[i].used  = 1;
    socket_table[i].state = SOCKET_STATE_NIL;
    socket_table[i].owner = *owner;
    socket_table[i].buf.bytes     = NULL;
    socket_table[i].lock          = L4SEMAPHORE_LOCKED;
    socket_table[i].operation_sem = L4SEMAPHORE_LOCKED;
  } else
    i = -1;
  l4semaphore_up(&socket_table_lock);

  return i;
}


static void free_handle(int h) {

  Assert(h >= 0 && h < MAX_SOCKETS);

  l4semaphore_down(&socket_table_lock);
  socket_table[h].used = 0;
  l4semaphore_up(&socket_table_lock);
}

/* ******************************************************************* */

static int allocate_address(const char *addr, socket_desc_t *s) {

  int i = 0;

  l4semaphore_down(&addr_table_lock);
  while (i < MAX_ADDRESSES && addr_table[i].used > 0) {
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
    addr_table[i].used   = 1;
    addr_table[i].handle = socket_handle(s);
  } else
    i = -1;

  l4semaphore_up(&addr_table_lock);
  return i;
}


static void free_address(int i) {

  l4semaphore_down(&addr_table_lock);
  Assert(addr_table[i].used);
  addr_table[i].used = 0;
  l4semaphore_up(&addr_table_lock);
}


static socket_desc_t *grab_address_owner(const char *addr) {

  int i = 0;

  l4semaphore_down(&addr_table_lock);
  while (i < MAX_ADDRESSES  &&
         strncmp(addr, addr_table[i].sun_path, MAX_ADDRESS_LEN) != 0)
    i++;

  if (i < MAX_ADDRESSES && addr_table[i].used) {
    socket_desc_t *s = socket_desc(addr_table[i].handle);
    socket_lock(s);
    if (s->used && s->addr_handle == i)
      return s;
    socket_unlock(s);
  }

  l4semaphore_up(&addr_table_lock);
  return NULL;
}

/* ******************************************************************* */

void init_job_info(job_info_t *job, l4_threadid_t *client, int type) {

  job->type    = type;
  job->replier = l4_myself();
  job->client  = *client;  
}


job_info_t *create_job_info(l4_threadid_t *client, int type) {

  job_info_t *job = (job_info_t *) CORBA_alloc(sizeof(*job));
  init_job_info(job, client, type);
  return job;
}


void close_all_sockets_of_client(l4_threadid_t *client) {

  int i;
  
  LOGd(_DEBUG_EVENTS, "closing all sockets owned by "l4util_idfmt,
       l4util_idstr(*client));
  
  for (i = 0; i < MAX_SOCKETS; i++) {
    
    int ret = close_internal(client, i);  
    if (ret == 0)
      LOGd(_DEBUG_EVENTS, "successfully closed socket %d", i);
    else if (ret == -EBADF)
      LOGd(_DEBUG_EVENTS, "socket %d not owned by "l4util_idfmt, i,
           l4util_idstr(*client));
    else
      LOGd(_DEBUG_EVENTS, "error closing socket %d: err=%d ('%s')",
           i, ret, l4env_strerror(ret));
  }
}

/* ******************************************************************* */
/* ******************************************************************* */

void local_socks_init(void) {

  int i;

  for (i = 0; i < MAX_SOCKETS; i++) {
    socket_table[i].used = 0;
    socket_table[i].lock = L4SEMAPHORE_UNLOCKED;
  }

  for (i = 0; i < MAX_ADDRESSES; i++)
    addr_table[i].used = 0;

  socket_table_lock = L4SEMAPHORE_UNLOCKED;
  addr_table_lock   = L4SEMAPHORE_UNLOCKED;
}

