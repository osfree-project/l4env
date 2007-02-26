/* *** GENERAL INCLUDES *** */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/* *** L4-SPECIFIC INCLUDES *** */
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/env/errno.h>
#include <l4/thread/thread.h>

/* *** LOCAL INCLUDES *** */
#include "local_socks-server.h"
#include "local_socks-client.h"
#include "socket_internal.h"

/* ******************************************************************* */

#ifdef DEBUG
# define _DEBUG 1
# define _DEBUG_WORKER 1
#else
# define _DEBUG 0
# define _DEBUG_WORKER 0
#endif

/* Operations which can block (connect(), recv(), ...) are performed
 * in a separate worker thread and then the results are sent to the
 * approriate server_loop()-thread afterwards, so that it can finally
 * reply to the client.
 *
 * Since creating worker threads using l4thread_create() is very
 * expensive, we always keep a pool of worker threads at hand. After
 * performing their job they block in an IPC operation. Wakening them
 * requires much less time than creating new worker threads. You can
 * still disable the use of worker threads for send()/recv()
 * operations, though. You will get the highest possible performance
 * for these functions in turn. However, other threads in a client
 * process may not be able to perform socket operation for a long
 * time, if one thread of that process called send() for example and
 * this call blocked (as there is only one server thread for each
 * client due to L4VFS' architecture). connect() and accept() will
 * still be performed in separate worker threads.
 */
//#define NO_WORKER_THREAD

/* ******************************************************************* */

/* max. number of idle worker to keep for each client connection */
#define MAX_IDLE_WORKERS 2

/* ******************************************************************* */

const int l4thread_max_threads = 64;
l4_ssize_t l4libc_heapsize = 4 * 1048576;

/* ******************************************************************* */

typedef struct idle_workers {
  l4_uint32_t num_idle;       /* number of threads that are idle or
                               * will be idle after delivering their
                               * result */
  l4_uint32_t num_idle_ready; /* number of idle threads, that are
                               * actually idle (result delivered) */
  l4_threadid_t workers[MAX_IDLE_WORKERS]; /* holds 'num_idle_ready'
                                            * idle workers */
} idle_workers_t;

static int idle_workers_key = -L4_ENOKEY;

/* ******************************************************************* */

static void            worker_thread(void *p);
static idle_workers_t *my_idle_workers(void);
static l4thread_t      start_worker(job_info_t *j);
static int             exit_worker(job_info_t *j);
static void            set_idle_worker_ready(l4_threadid_t worker);
static void            reply_to_client(job_info_t *j);

static void server_thread(void *param);

static void free_old_dice_buffers(CORBA_Server_Environment *env);

/* ******************************************************************* */
/* ******************************************************************* */

l4_int32_t
l4vfs_common_io_fcntl_component(CORBA_Object _dice_corba_obj,
                                object_handle_t fd,
                                l4_int32_t cmd,
                                l4_int32_t *arg,
                                CORBA_Server_Environment *_dice_corba_env)
{
  int ret;

  if ( !handle_is_valid(fd) ||
       !client_owns_handle(*_dice_corba_obj, fd))
    return -EBADF;
  
  ret = fcntl_internal(fd, cmd, *arg);
  return ret;
}



l4vfs_ssize_t
l4vfs_common_io_read_component(CORBA_Object _dice_corba_obj,
                               object_handle_t fd,
                               l4_int8_t **buf,
                               l4vfs_size_t *count,
                               l4_int16_t *_dice_reply,
                               CORBA_Server_Environment *_dice_corba_env)
{
  if ( !handle_is_valid(fd) ||
       !client_owns_handle(*_dice_corba_obj, fd))
    return -EBADF;
  
#ifdef NO_WORKER_THREAD
  return recv_internal(NULL, fd, *buf, count, 0);

#else
  {
    l4thread_t th;
    job_info_t *j;

    *_dice_reply     = DICE_NO_REPLY;
    
    j                = create_job_info(JOB_TYPE_READ, *_dice_corba_obj);
    j->in.recv.fd    = fd;
    j->in.recv.msg   = *buf;
    j->in.recv.len   = *count;
    j->in.recv.flags = 0;
    
    free_old_dice_buffers(_dice_corba_env);
    th = start_worker(j);
    return (th > 0) ? 0 : -1;
  }
#endif
}



l4vfs_ssize_t
l4vfs_common_io_write_component(CORBA_Object _dice_corba_obj,
                                object_handle_t fd,
                                const l4_int8_t *buf,
                                l4vfs_size_t *count,
                                l4_int16_t *_dice_reply,
                                CORBA_Server_Environment *_dice_corba_env)
{
  if ( !handle_is_valid(fd) ||
       !client_owns_handle(*_dice_corba_obj, fd))
    return -EBADF;
  
#ifdef NO_WORKER_THREAD
  return send_internal(NULL, fd, buf, *count, 0);

#else
  {
    l4thread_t th;
    job_info_t *j;
    
    *_dice_reply     = DICE_NO_REPLY;
    
    j                = create_job_info(JOB_TYPE_WRITE, *_dice_corba_obj);
    j->in.send.fd    = fd;
    /* FIXME: I don't know how to make server_loop() always allocate a
     * new recv_buffer. For the moment we copy it. */
    j->in.send.msg   = (char *) CORBA_alloc(*count);
    memcpy(j->in.send.msg, buf, *count);
    j->in.send.len   = *count;
    j->in.send.flags = 0;
    
    th = start_worker(j);
    return (th > 0) ? 0 : -1;
  }
#endif
}



l4_int32_t
l4vfs_common_io_close_component(CORBA_Object _dice_corba_obj,
                                object_handle_t object_handle,
                                CORBA_Server_Environment *_dice_corba_env)
{
  int ret;

  if ( !handle_is_valid(object_handle) ||
       !client_owns_handle(*_dice_corba_obj, object_handle))
    return -EBADF;
  
  ret = close_internal(object_handle);
  return ret;
}



l4_int32_t
l4vfs_common_io_ioctl_component(CORBA_Object _dice_corba_obj,
                                object_handle_t fd,
                                l4_int32_t cmd,
                                l4_int8_t **arg,
                                l4vfs_size_t *count,
                                CORBA_Server_Environment *_dice_corba_env)
{
  if ( !handle_is_valid(fd) ||
       !client_owns_handle(*_dice_corba_obj, fd))
    return -EBADF;

  return ioctl_internal(fd, cmd, (char **) arg, count);
}


l4_int32_t
l4vfs_net_io_accept_component(CORBA_Object _dice_corba_obj,
                              object_handle_t fd,
                              l4_int8_t addr[16],
                              l4_int32_t *addrlen,
                              l4_int32_t *actual_len,
                              l4_int16_t *_dice_reply,
                              CORBA_Server_Environment *_dice_corba_env)
{
  l4thread_t th;
  job_info_t *j;

  if ( !handle_is_valid(fd) ||
       !client_owns_handle(*_dice_corba_obj, fd))
    return -EBADF;
  
  *_dice_reply = DICE_NO_REPLY;

  j                     = create_job_info(JOB_TYPE_ACCEPT, *_dice_corba_obj);
  j->in.accept.fd       = fd;
  j->in.accept.addr     = addr;
  j->in.accept.addr_len = *addrlen;

  th = start_worker(j);
  return (th > 0) ? 0 : -1;
}



l4_int32_t
l4vfs_net_io_bind_component(CORBA_Object _dice_corba_obj,
                            object_handle_t fd,
                            const l4_int8_t addr[16],
                            l4_int32_t addrlen,
                            CORBA_Server_Environment *_dice_corba_env)
{
  int ret;

  if ( !handle_is_valid(fd) ||
       !client_owns_handle(*_dice_corba_obj, fd))
    return -EBADF;
  
  ret = bind_internal(fd, addr, addrlen);

  return ret;
}



l4_int32_t
l4vfs_net_io_connect_component(CORBA_Object _dice_corba_obj,
                               object_handle_t fd,
                               const l4_int8_t addr[16],
                               l4_int32_t addrlen,
                               l4_int16_t *_dice_reply,
                               CORBA_Server_Environment *_dice_corba_env)
{
  l4thread_t th;
  job_info_t *j;
  char *addr_buf;

  if ( !handle_is_valid(fd) ||
       !client_owns_handle(*_dice_corba_obj, fd))
    return -EBADF;
  
  *_dice_reply = DICE_NO_REPLY;

  /* FIXME: I don't know how to make server_loop() always allocate a
   * new recv_buffer. For the moment we copy it. */
  j                      = create_job_info(JOB_TYPE_CONNECT, *_dice_corba_obj);
  j->in.connect.fd       = fd;
  addr_buf               = (char *) CORBA_alloc(addrlen);
  memcpy(addr_buf, addr, addrlen);
  j->in.connect.addr     = addr_buf;
  j->in.connect.addr_len = addrlen;

  th = start_worker(j);
  return (th > 0) ? 0 : -1;
}



l4_int32_t
l4vfs_net_io_listen_component(CORBA_Object _dice_corba_obj,
                              object_handle_t fd,
                              l4_int32_t backlog,
                              CORBA_Server_Environment *_dice_corba_env)
{
  int ret;

  if ( !handle_is_valid(fd) ||
       !client_owns_handle(*_dice_corba_obj, fd))
    return -EBADF;
  
  ret = listen_internal(fd, backlog);
  return ret;
}



l4_int32_t
l4vfs_net_io_recvfrom_component(CORBA_Object _dice_corba_obj,
                                object_handle_t fd,
                                l4_int8_t **buf,
                                l4_int32_t *len,
                                l4_int32_t flags,
                                l4_int8_t from[128],
                                l4_int32_t *fromlen,
                                l4_int32_t *actual_fromlen,
                                l4_int16_t *_dice_reply,
                                CORBA_Server_Environment *_dice_corba_env)
{
  LOG("l4vfs_net_io_recvfrom_component is not implemented!");
  return 0;
}



l4_int32_t
l4vfs_net_io_recv_component(CORBA_Object _dice_corba_obj,
                            object_handle_t fd,
                            l4_int8_t **buf,
                            l4_int32_t *len,
                            l4_int32_t flags,
                            l4_int16_t *_dice_reply,
                            CORBA_Server_Environment *_dice_corba_env)
{
  if ( !handle_is_valid(fd) ||
       !client_owns_handle(*_dice_corba_obj, fd))
    return -EBADF;

#ifdef NO_WORKER_THREAD
  return recv_internal(NULL, fd, *buf, len, flags);

#else
  {
    l4thread_t th;
    job_info_t *j;
    
    *_dice_reply     = DICE_NO_REPLY;
    
    j                = create_job_info(JOB_TYPE_RECV, *_dice_corba_obj);
    j->in.recv.fd    = fd;
    j->in.recv.msg   = *buf;
    j->in.recv.len   = *len;
    j->in.recv.flags = flags;
    
    free_old_dice_buffers(_dice_corba_env);
    th = start_worker(j);
    return (th > 0) ? 0 : -1;
  }
#endif
}



l4_int32_t
l4vfs_net_io_send_component(CORBA_Object _dice_corba_obj,
                            object_handle_t fd,
                            const l4_int8_t *msg,
                            l4_int32_t len,
                            l4_int32_t flags,
                            l4_int16_t *_dice_reply,
                            CORBA_Server_Environment *_dice_corba_env)
{
  if ( !handle_is_valid(fd) ||
       !client_owns_handle(*_dice_corba_obj, fd))
    return -EBADF;
  
#ifdef NO_WORKER_THREAD
  return send_internal(NULL, fd, msg, len, flags);

#else
  {
    l4thread_t th;
    job_info_t *j;
    
    *_dice_reply     = DICE_NO_REPLY;
    
    j                = create_job_info(JOB_TYPE_SEND, *_dice_corba_obj);
    j->in.send.fd    = fd;
    /* FIXME: I don't know how to make server_loop() always allocate a
     * new recv_buffer. For the moment we copy it. */
    j->in.send.msg   = (char *) CORBA_alloc(len);
    memcpy(j->in.send.msg, msg, len);
    j->in.send.len   = len;
    j->in.send.flags = flags;

    th = start_worker(j);
    return (th > 0) ? 0 : -1;
  }
#endif
}



l4_int32_t
l4vfs_net_io_sendto_component(CORBA_Object _dice_corba_obj,
                              object_handle_t fd,
                              const l4_int8_t *msg,
                              l4_int32_t len,
                              l4_int32_t flags,
                              const l4_int8_t *to,
                              l4_int32_t tolen,
                              l4_int16_t *_dice_reply,
                              CORBA_Server_Environment *_dice_corba_env)
{
  LOG("l4vfs_net_io_sendto_component is not implemented!");
  return 0;
}



l4_int32_t
l4vfs_net_io_sendmsg_component(CORBA_Object _dice_corba_obj,
                               object_handle_t fd,
                               const l4_int8_t msg_name[8192],
                               l4_int32_t msg_namelen,
                               const l4_int8_t *msg_iov,
                               l4_int32_t msg_iovlen,
                               l4_int32_t msg_iov_size,
                               const l4_int8_t *msg_control,
                               l4_int32_t msg_controllen,
                               l4_int32_t msg_flags,
                               l4_int32_t flags,
                               l4_int16_t *_dice_reply,
                               CORBA_Server_Environment *_dice_corba_env)
{
  LOG("l4vfs_net_io_sendmsg_component is not implemented!");
  return 0;
}



l4_int32_t
l4vfs_net_io_shutdown_component(CORBA_Object _dice_corba_obj,
                                object_handle_t fd,
                                l4_int32_t how,
                                CORBA_Server_Environment *_dice_corba_env)
{
  int ret;

  if ( !handle_is_valid(fd) ||
       !client_owns_handle(*_dice_corba_obj, fd))
    return -EBADF;
  
  if (how >= 0 || how <= 2)
    ret = shutdown_internal(fd, how);
  else
    ret = -EINVAL;

  return ret;
}



l4_int32_t
l4vfs_net_io_socket_component(CORBA_Object _dice_corba_obj,
                              l4_int32_t domain,
                              l4_int32_t type,
                              l4_int32_t protocol,
                              CORBA_Server_Environment *_dice_corba_env)
{
  return socket_internal(domain, type, protocol);
}



l4_int32_t
l4vfs_net_io_socketpair_component(CORBA_Object _dice_corba_obj,
                                  l4_int32_t domain,
                                  l4_int32_t type,
                                  l4_int32_t protocol,
                                  object_handle_t *fd0,
                                  object_handle_t *fd1,
                                  CORBA_Server_Environment *_dice_corba_env)
{
  return socketpair_internal(domain, type, protocol, fd0, fd1);
}



l4_int32_t
l4vfs_net_io_getsockname_component(CORBA_Object _dice_corba_obj,
                                   object_handle_t s,
                                   l4_int8_t name[4096],
                                   l4_int32_t *len,
                                   CORBA_Server_Environment *_dice_corba_env)
{
  return -EOPNOTSUPP;
}



l4_int32_t
l4vfs_net_io_setsockopt_component(CORBA_Object _dice_corba_obj,
                                  object_handle_t s,
                                  l4_int32_t level,
                                  l4_int32_t optname,
                                  const l4_int8_t *optval,
                                  l4_int32_t optlen,
                                  CORBA_Server_Environment *_dice_corba_env)
{
  LOG("l4vfs_net_io_setsockopt_component is not implemented!");
  return 0;
}

/* ******************************************************************* */

l4_threadid_t
l4vfs_connection_init_connection_component(CORBA_Object _dice_corba_obj,
                                           CORBA_Server_Environment *_dice_corba_env)
{
  l4thread_t th;

  LOGd(_DEBUG, "starting new server thread");
  th = l4thread_create(server_thread, (void *) _dice_corba_obj, L4THREAD_CREATE_SYNC);
  if (th <= 0) {
    LOG_Error("failed to create new thread: %d: %s", th, l4env_strerror(th));
  }
  
  return l4thread_l4_id(th);
}



void
l4vfs_connection_close_connection_component(CORBA_Object _dice_corba_obj,
                                            const l4_threadid_t *server,
                                            CORBA_Server_Environment *_dice_corba_env)
{
  idle_workers_t *my_workers = my_idle_workers();

  CORBA_free(my_workers);

  LOGd(_DEBUG, "connection closed");
  l4thread_exit();
}

/* ******************************************************************* */

void
l4vfs_select_notify_request_component(CORBA_Object _dice_corba_obj,
                                      object_handle_t fd,
                                      l4_int32_t mode,
                                      const l4_threadid_t *notif_tid,
                                      CORBA_Server_Environment *_dice_corba_env)
{
  if ( !handle_is_valid(fd) ||
       !client_owns_handle(*_dice_corba_obj, fd) ||
       mode & ~(SELECT_READ | SELECT_WRITE | SELECT_EXCEPTION))
    return;
  
  register_select_notify(fd, *notif_tid, mode);
}



void
l4vfs_select_notify_clear_component(CORBA_Object _dice_corba_obj,
                                    object_handle_t fd,
                                    l4_int32_t mode,
                                    const l4_threadid_t *notif_tid,
                                    CORBA_Server_Environment *_dice_corba_env)
{
  if ( !handle_is_valid(fd) ||
       !client_owns_handle(*_dice_corba_obj, fd) ||
       mode & ~(SELECT_READ | SELECT_WRITE | SELECT_EXCEPTION))
    return;
  
  deregister_select_notify(fd, *notif_tid, mode);
}

/* ******************************************************************* */

void
local_socks_worker_done_component(CORBA_Object _dice_corba_obj,
                                  l4_addr_t *job_info,
                                  l4_int16_t *_dice_reply,
                                  CORBA_Server_Environment *_dice_corba_env)
{
  job_info_t *j = (job_info_t *) *job_info;

  LOGd(_DEBUG_WORKER, "worker thread %d is ready and waiting",
       _dice_corba_obj->id.lthread);

  if ( !l4_task_equal(*_dice_corba_obj, l4_myself() )) {
    LOGd(_DEBUG, "Called by another task; this is not allowed!");
    return;
  }

  set_idle_worker_ready(j->worker);

  /* let worker sleep; will be waken when a new job arrives */
  *_dice_reply = DICE_NO_REPLY;

  reply_to_client(j);
}



void
local_socks_worker_done_and_exiting_component(CORBA_Object _dice_corba_obj,
                                              l4_addr_t job_info,
                                              CORBA_Server_Environment *_dice_corba_env)
{
  job_info_t *j = (job_info_t *) job_info;

  LOGd(_DEBUG_WORKER, "worker thread %d is ready and exited",
       _dice_corba_obj->id.lthread);

  if ( !l4_task_equal(*_dice_corba_obj, l4_myself() )) {
    LOGd(_DEBUG, "Called by another task; this is not allowed!");
    return;
  }

  set_idle_worker_ready(j->worker);
  reply_to_client(j);
}

/* ******************************************************************* */
/* ******************************************************************* */

static void
server_thread(void *param)
{
  l4thread_started(NULL);
  local_socks_server_loop(NULL);
}

/* ******************************************************************* */

static idle_workers_t *
my_idle_workers(void)
{
  idle_workers_t *my_workers;

  /* allocate data key once */
  if (idle_workers_key == -L4_ENOKEY)
    idle_workers_key = l4thread_data_allocate_key();

  /* get thread local data structure */
  my_workers   = (idle_workers_t *) l4thread_data_get_current(idle_workers_key);

  if (my_workers == NULL) {

    /* there is no idle_workers_t for this thread yet */
    my_workers = (idle_workers_t *) CORBA_alloc(sizeof(*my_workers));

    if (my_workers) {

      if (l4thread_data_set_current(idle_workers_key, my_workers) != 0)
        LOG_Error("Failed to set thread private data for worker thread pool.");
      else {
        my_workers->num_idle       = 0;
        my_workers->num_idle_ready = 0;
      }
    }
  }

  return my_workers;
}


static l4thread_t
start_worker(job_info_t *j)
{
  l4thread_t     th;
  idle_workers_t *my_workers = my_idle_workers();

  if (my_workers->num_idle_ready > 0) {

    CORBA_Server_Environment env = dice_default_server_environment;
    l4_threadid_t idle_worker;

    /* there is an idle worker thread waiting, which can accept a new
       job right now */

    my_workers->num_idle_ready--;
    idle_worker = my_workers->workers[my_workers->num_idle_ready];

    LOGd(_DEBUG_WORKER, "%d is restarting idle worker thread %d",
         l4thread_myself(), idle_worker.id.lthread);
    local_socks_worker_done_reply(&idle_worker, (l4_addr_t *) &j, &env);

    th = l4thread_id(idle_worker);

  } else {

    /* No luck. This will be expensive! */

    LOGd(_DEBUG_WORKER, "creating new worker thread");
    th = l4thread_create(worker_thread, j, L4THREAD_CREATE_SYNC);
  }

  return th;
}


static void
set_idle_worker_ready(l4_threadid_t worker)
{
  idle_workers_t *my_workers = my_idle_workers();

  my_workers->workers[my_workers->num_idle_ready] = worker;
  my_workers->num_idle_ready++;
}


static int
exit_worker(job_info_t *j)
{
  idle_workers_t *workers;

  workers = l4thread_data_get(l4thread_id(j->replier), idle_workers_key);

  if (workers->num_idle < MAX_IDLE_WORKERS) {
    
    l4_uint32_t old, new;
    do {
      old = workers->num_idle;
      new = l4util_cmpxchg32(&workers->num_idle, old, workers->num_idle + 1);
    } while (old != new && new < MAX_IDLE_WORKERS);

    return (old != new);
  }

  return 0;
}


static void
worker_thread(void *p)
{
  CORBA_Environment env = dice_default_environment;
  job_info_t        *j = (job_info_t *) p;
  l4_threadid_t     myself;
  int               terminate = 1;

  l4thread_started(NULL);

  myself    = l4_myself();
  j->worker = myself;

  do {

    switch (j->type) {

    case JOB_TYPE_CONNECT:
      j->retval = connect_internal(j, j->in.connect.fd, j->in.connect.addr,
                                   j->in.connect.addr_len);
      break;

    case JOB_TYPE_ACCEPT:
      j->retval = accept_internal(j, j->in.accept.fd, j->in.accept.addr,
                                  &j->in.accept.addr_len);
      j->out.accept.fd       = j->retval;
      j->out.accept.addr     = (char *) j->in.accept.addr; /* (char*) to shut up compiler */
      j->out.accept.addr_len = j->in.accept.addr_len;
      break;

#ifndef NO_WORKER_THREAD
    case JOB_TYPE_SEND:    
    case JOB_TYPE_WRITE:
      j->retval = send_internal(j, j->in.send.fd, j->in.send.msg,
                                j->in.send.len, j->in.send.flags);
      break;

    case JOB_TYPE_RECV:    
    case JOB_TYPE_READ: 
      j->retval = recv_internal(j, j->in.recv.fd, j->in.recv.msg,
                                &j->in.recv.len, j->in.recv.flags);
      j->out.recv.len = (j->retval > 0) ? j->retval : 0;
      j->out.recv.msg = j->in.recv.msg;
      break;
#endif /* ! NO_WORKER_THREAD */

    default:
      LOG_Error("invalid value for job type: %d", j->type);
      return;
    }

    terminate = exit_worker(j);

    if (terminate)
      local_socks_worker_done_and_exiting_send(&j->replier, (l4_addr_t) j, &env);
    else {
      local_socks_worker_done_call(&j->replier, (l4_addr_t *) &j, &env);
      j->worker = myself;
    }

  } while ( !terminate);

  LOGd(_DEBUG_WORKER, "worker thread %d exiting", myself.id.lthread);
}


static void
reply_to_client(job_info_t *j)
{ 
  CORBA_Server_Environment env = dice_default_server_environment;
  
  switch (j->type) {

  case JOB_TYPE_CONNECT:
    l4vfs_net_io_connect_reply(&j->client, j->retval, &env);
    break;

  case JOB_TYPE_ACCEPT:
    l4vfs_net_io_accept_reply(&j->client, j->retval,
                              j->out.accept.addr, &j->out.accept.addr_len,
                              &j->out.accept.addr_len, &env);
    break;

#ifndef NO_WORKER_THREAD
  case JOB_TYPE_SEND:
    l4vfs_net_io_send_reply(&j->client, j->retval, &env);
    CORBA_free(j->in.send.msg);
    break;

  case JOB_TYPE_WRITE:
    l4vfs_common_io_write_reply(&j->client, j->retval, &j->retval, &env);
    CORBA_free(j->in.send.msg);
    break;

  case JOB_TYPE_RECV:
    l4vfs_net_io_recv_reply(&j->client, j->retval, (l4_int8_t **) &j->out.recv.msg,
                            &j->out.recv.len, &env);
    /* j->out.recv.msg will be free()ed by dice's code or
       free_old_dice_buffers() */
    break;

  case JOB_TYPE_READ:
    l4vfs_common_io_read_reply(&j->client, j->retval, (l4_int8_t **) &j->out.recv.msg,
                               &j->out.recv.len, &env);
    /* j->out.recv.msg will be free()ed by dice's code or
       free_old_dice_buffers() */
    break;
#endif /* ! NO_WORKERTHREAD */

  default:
    LOG_Error("invalid value for job type");
  }

  CORBA_free(j);
}

/* ******************************************************************* */

void *
CORBA_alloc(unsigned long size)
{
  return malloc(size);
}



void
CORBA_free(void *p)
{
  free(p);
}



static void
free_old_dice_buffers(CORBA_Server_Environment *env)
{
  /* Sometimes dice doesn't free buffers it allocated for
     [allow_reply_only]-functions, if such a function sets
     dice_reply=0 */

  void *p;

  while ((p=dice_get_last_ptr(env)) != 0)
    CORBA_free(p);
}

