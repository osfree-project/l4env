/* $Id$ */
/*****************************************************************************/
/**
 * \file   local_socks/server/src/server.c
 * \brief  Socket server IDL glue layer and session management implementation.
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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/* *** L4-SPECIFIC INCLUDES *** */
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/env/errno.h>
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>
#include <l4/util/l4_macros.h>

/* *** LOCAL INCLUDES *** */
#include "local_socks-server.h"
#include "local_socks-client.h"
#include "socket_internal.h"
#include "server.h"
#include "events.h"

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
 *
 * FIXME: Using worker thread extensively causes Qt3 sessions to hang.
 * Need to debug this.
 */
#define NO_WORKER_THREAD

/* ******************************************************************* */

#define MAX_SESSIONS     20
#define MAX_IDLE_WORKERS MAX_SESSIONS
#define MAX_THREADS      80

/* ******************************************************************* */

const int  l4thread_max_threads = MAX_THREADS;
l4_ssize_t l4libc_heapsize      = 8 * 1048576;

/* ******************************************************************* */

typedef struct session_s {

  l4_threadid_t id;
  l4_threadid_t client;
} session_t;

static l4_threadid_t master_session_id;
static session_t     sessions[MAX_SESSIONS];
static int           num_sessions;

static l4_threadid_t idle_workers[MAX_IDLE_WORKERS];
static int           num_idle_workers;

static l4semaphore_t sessions_lock     = L4SEMAPHORE_UNLOCKED;
static l4semaphore_t idle_workers_lock = L4SEMAPHORE_UNLOCKED;

static int job_key;

/* ******************************************************************* */

static void       session_thread(void *arg);
static l4thread_t start_worker(job_info_t *j);
static void       perform_job(void *arg);
static void       restart_worker_and_perform_job(void);
static void       reply_to_client(job_info_t *j);

#ifndef NO_WORKER_THREAD
static void free_old_dice_buffers(CORBA_Server_Environment *env);
#endif

/* ******************************************************************* */
/* ******************************************************************* */

l4_int32_t
l4vfs_common_io_fcntl_component(CORBA_Object _dice_corba_obj,
                                object_handle_t fd,
                                int cmd,
                                long *arg,
                                CORBA_Server_Environment *_dice_corba_env)
{
  return fcntl_internal(_dice_corba_obj, fd, cmd, *arg);
}



l4vfs_ssize_t
l4vfs_common_io_read_component(CORBA_Object _dice_corba_obj,
                               object_handle_t fd,
                               char **buf,
                               l4vfs_size_t *count,
                               l4_int16_t *_dice_reply,
                               CORBA_Server_Environment *_dice_corba_env)
{
#ifdef NO_WORKER_THREAD
  job_info_t j;
  init_job_info(&j, _dice_corba_obj, JOB_TYPE_READ);
  return recv_internal(&j, fd, *buf, count, 0);

#else
  l4thread_t th;
  job_info_t *j;

  *_dice_reply     = DICE_NO_REPLY;
  j                = create_job_info(_dice_corba_obj, JOB_TYPE_READ);
  j->in.recv.fd    = fd;
  j->in.recv.msg   = *buf;
  j->in.recv.len   = *count;
  j->in.recv.flags = 0;

  free_old_dice_buffers(_dice_corba_env);
  th = start_worker(j);
  return (th > 0) ? 0 : -1;
#endif
}



l4vfs_ssize_t
l4vfs_common_io_write_component(CORBA_Object _dice_corba_obj,
                                object_handle_t fd,
                                const char *buf,
                                l4vfs_size_t *count,
                                l4_int16_t *_dice_reply,
                                CORBA_Server_Environment *_dice_corba_env)
{
#ifdef NO_WORKER_THREAD
  job_info_t j;
  init_job_info(&j, _dice_corba_obj, JOB_TYPE_WRITE);
  return send_internal(&j, fd, buf, *count, 0);

#else
  l4thread_t th;
  job_info_t *j;

  *_dice_reply     = DICE_NO_REPLY;
  j                = create_job_info(_dice_corba_obj, JOB_TYPE_WRITE);
  j->in.send.fd    = fd;
  /* FIXME: I don't know how to make server_loop() always allocate a
   * new recv_buffer. For the moment we copy it. */
  j->in.send.msg   = (char *) CORBA_alloc(*count);
  memcpy(j->in.send.msg, buf, *count);
  j->in.send.len   = *count;
  j->in.send.flags = 0;

  th = start_worker(j);
  return (th > 0) ? 0 : -1;
#endif
}



l4_int32_t
l4vfs_common_io_close_component(CORBA_Object _dice_corba_obj,
                                object_handle_t object_handle,
                                CORBA_Server_Environment *_dice_corba_env)
{
  return close_internal(_dice_corba_obj, object_handle);
}



l4_int32_t
l4vfs_common_io_ioctl_component(CORBA_Object _dice_corba_obj,
                                object_handle_t fd,
                                int cmd,
                                char **arg,
                                l4vfs_size_t *count,
                                CORBA_Server_Environment *_dice_corba_env)
{
  return ioctl_internal(_dice_corba_obj, fd, cmd, (char **) arg, count);
}


int
l4vfs_net_io_accept_component(CORBA_Object _dice_corba_obj,
                              object_handle_t fd,
                              char addr[16],
                              l4vfs_socklen_t *addrlen,
                              l4vfs_socklen_t *actual_len,
                              l4_int16_t *_dice_reply,
                              CORBA_Server_Environment *_dice_corba_env)
{
  l4thread_t th;
  job_info_t *j;

  *_dice_reply          = DICE_NO_REPLY;
  j                     = create_job_info(_dice_corba_obj, JOB_TYPE_ACCEPT);
  j->in.accept.fd       = fd;
  j->in.accept.addr     = addr;
  j->in.accept.addr_len = *addrlen;

  th = start_worker(j);
  return (th > 0) ? 0 : -1;
}



int
l4vfs_net_io_bind_component(CORBA_Object _dice_corba_obj,
                            object_handle_t fd,
                            const char addr[16],
                            int addrlen,
                            CORBA_Server_Environment *_dice_corba_env)
{
  return bind_internal(_dice_corba_obj, fd, addr, addrlen);
}



l4_int32_t
l4vfs_net_io_connect_component(CORBA_Object _dice_corba_obj,
                               object_handle_t fd,
                               const char addr[16],
                               int addrlen,
                               l4_int16_t *_dice_reply,
                               CORBA_Server_Environment *_dice_corba_env)
{
  l4thread_t th;
  job_info_t *j;
  char *addr_buf;

  /* FIXME: I don't know how to make server_loop() always allocate a
   * new recv_buffer. For the moment we copy it. */
  *_dice_reply     = DICE_NO_REPLY;
  j                      = create_job_info(_dice_corba_obj, JOB_TYPE_CONNECT);
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
  return listen_internal(_dice_corba_obj, fd, backlog);
}



int
l4vfs_net_io_recvfrom_component(CORBA_Object _dice_corba_obj,
                                object_handle_t fd,
                                char **buf,
                                l4vfs_socklen_t *len,
                                int flags,
                                char from[128],
                                l4vfs_socklen_t *fromlen,
                                l4vfs_socklen_t *actual_fromlen,
                                l4_int16_t *_dice_reply,
                                CORBA_Server_Environment *_dice_corba_env)
{
  LOG("l4vfs_net_io_recvfrom_component is not implemented!");
  return 0;
}



int
l4vfs_net_io_recv_component(CORBA_Object _dice_corba_obj,
                            object_handle_t fd,
                            char **buf,
                            l4vfs_socklen_t *len,
                            int flags,
                            l4_int16_t *_dice_reply,
                            CORBA_Server_Environment *_dice_corba_env)
{
#ifdef NO_WORKER_THREAD
  job_info_t j;
  init_job_info(&j, _dice_corba_obj, JOB_TYPE_RECV);
  return recv_internal(&j, fd, *buf, len, flags);

#else
  l4thread_t th;
  job_info_t *j;

  *_dice_reply     = DICE_NO_REPLY;
  j                = create_job_info(_dice_corba_obj, JOB_TYPE_RECV);
  j->in.recv.fd    = fd;
  j->in.recv.msg   = *buf;
  j->in.recv.len   = *len;
  j->in.recv.flags = flags;

  free_old_dice_buffers(_dice_corba_env);
  th = start_worker(j);
  return (th > 0) ? 0 : -1;
#endif
}



int
l4vfs_net_io_send_component(CORBA_Object _dice_corba_obj,
                            object_handle_t fd,
                            const char *msg,
                            int len,
                            int flags,
                            l4_int16_t *_dice_reply,
                            CORBA_Server_Environment *_dice_corba_env)
{
#ifdef NO_WORKER_THREAD
  job_info_t j;
  init_job_info(&j, _dice_corba_obj, JOB_TYPE_SEND);
  return send_internal(&j, fd, msg, len, flags);

#else
  l4thread_t th;
  job_info_t *j;

  *_dice_reply     = DICE_NO_REPLY;
  j                = create_job_info(_dice_corba_obj, JOB_TYPE_SEND);
  j->in.send.fd    = fd;
  /* FIXME: I don't know how to make server_loop() always allocate a
   * new recv_buffer. For the moment we copy it. */
  j->in.send.msg   = (char *) CORBA_alloc(len);
  memcpy(j->in.send.msg, msg, len);
  j->in.send.len   = len;
  j->in.send.flags = flags;

  th = start_worker(j);
  return (th > 0) ? 0 : -1;
#endif
}



int
l4vfs_net_io_sendto_component(CORBA_Object _dice_corba_obj,
                              object_handle_t fd,
                              const char *msg,
                              int len,
                              int flags,
                              const char *to,
                              int tolen,
                              l4_int16_t *_dice_reply,
                              CORBA_Server_Environment *_dice_corba_env)
{
  LOG("l4vfs_net_io_sendto_component is not implemented!");
  return 0;
}



int
l4vfs_net_io_sendmsg_component(CORBA_Object _dice_corba_obj,
                               object_handle_t fd,
                               const char msg_name[8192],
                               int msg_namelen,
                               const char *msg_iov,
                               int msg_iovlen,
                               int msg_iov_size,
                               const char *msg_control,
                               int msg_controllen,
                               int msg_flags,
                               int flags,
                               l4_int16_t *_dice_reply,
                               CORBA_Server_Environment *_dice_corba_env)
{
  LOG("l4vfs_net_io_sendmsg_component is not implemented!");
  return 0;
}



int
l4vfs_net_io_shutdown_component(CORBA_Object _dice_corba_obj,
                                object_handle_t fd,
                                int how,
                                CORBA_Server_Environment *_dice_corba_env)
{
  int ret;

  if (how >= 0 || how <= 2)
    ret = shutdown_internal(_dice_corba_obj, fd, how);
  else
    ret = -EINVAL;

  return ret;
}



int
l4vfs_net_io_socket_component(CORBA_Object _dice_corba_obj,
                              int domain,
                              int type,
                              int protocol,
                              CORBA_Server_Environment *_dice_corba_env)
{
  return socket_internal(_dice_corba_obj, domain, type, protocol);
}



int
l4vfs_net_io_socketpair_component(CORBA_Object _dice_corba_obj,
                                  int domain,
                                  int type,
                                  int protocol,
                                  object_handle_t *fd0,
                                  object_handle_t *fd1,
                                  CORBA_Server_Environment *_dice_corba_env)
{
  return socketpair_internal(_dice_corba_obj, domain, type, protocol, fd0, fd1);
}



int
l4vfs_net_io_getsockname_component(CORBA_Object _dice_corba_obj,
                                   object_handle_t s,
                                   char name[4096],
                                   l4vfs_socklen_t *len,
                                   CORBA_Server_Environment *_dice_corba_env)
{
  return -EOPNOTSUPP;
}



int
l4vfs_net_io_setsockopt_component(CORBA_Object _dice_corba_obj,
                                  object_handle_t s,
                                  int level,
                                  int optname,
                                  const char *optval,
                                  int optlen,
                                  CORBA_Server_Environment *_dice_corba_env)
{
  LOG("l4vfs_net_io_setsockopt_component is not implemented!");
  return 0;
}

int
l4vfs_net_io_getsockopt_component(CORBA_Object _dice_corba_obj,
                                  object_handle_t s,
                                  int level,
                                  int optname,
                                  char *optval,
                                  l4vfs_socklen_t *optlen,
                                  l4vfs_socklen_t *actual_optlen,
                                  CORBA_Server_Environment *_dice_corba_env)
{
  LOG("l4vfs_net_io_getsockopt_component is not implemented!");
  return -1;
}

int
l4vfs_net_io_getpeername_component (CORBA_Object _dice_corba_obj,
                                    object_handle_t handle,
                                    char addr[120],
                                    l4vfs_socklen_t *addrlen,
                                    l4vfs_socklen_t *actual_len,
                                    CORBA_Server_Environment *_dice_corba_env)
{
  LOG("l4vfs_net_io_getpeername_component is not implemented!");
  return -1;
}
/* ******************************************************************* */

l4_threadid_t
l4vfs_connection_init_connection_component(CORBA_Object _dice_corba_obj,
                                           CORBA_Server_Environment *_dice_corba_env)
{
  int i;

  l4semaphore_down(&sessions_lock);  

  for (i = 0; i < MAX_SESSIONS; i++) {
    if (l4_thread_equal(sessions[i].client, L4_INVALID_ID))
      break;
  }    

  if (i == MAX_SESSIONS) {
    /* maximum number of session threads already reached; let the new session be
     * served by the master-session thread (we loose parallelity) */
    l4semaphore_up(&sessions_lock);
    LOG("Warning: Too many client sessions; returning master-session thread ID'");
    return master_session_id;
  }
  
  if (l4_thread_equal(sessions[i].id, L4_INVALID_ID)) {  
    
    l4thread_t th = l4thread_create_named(session_thread, ".session", NULL,
                                          L4THREAD_CREATE_SYNC);
    if (th > 0) {
      num_sessions++;
      sessions[i].id = l4thread_l4_id(th);
      LOGd(_DEBUG, "new session thread "l4util_idfmt" started",
           l4util_idstr(l4thread_l4_id(th)));
    } else {
      l4semaphore_up(&sessions_lock);
      LOG_Error("failed to create new thread: %d: %s", th, l4env_strerror(th));
      return master_session_id;
    }
  }

  sessions[i].client = *_dice_corba_obj;
  
  l4semaphore_up(&sessions_lock);
  
  return sessions[i].id;
}



void
l4vfs_connection_close_connection_component(CORBA_Object _dice_corba_obj,
                                            const l4_threadid_t *server,
                                            CORBA_Server_Environment *_dice_corba_env)
{
  shutdown_session_of_client(*_dice_corba_obj);
}

/* ******************************************************************* */

void
l4vfs_select_notify_request_component(CORBA_Object _dice_corba_obj,
                                      object_handle_t fd,
                                      l4_int32_t mode,
                                      const l4_threadid_t *notif_tid,
                                      CORBA_Server_Environment *_dice_corba_env)
{
  if (mode & ~(SELECT_READ | SELECT_WRITE | SELECT_EXCEPTION))
    return;
  
  register_select_notify(_dice_corba_obj, fd, notif_tid, mode);
}



void
l4vfs_select_notify_clear_component(CORBA_Object _dice_corba_obj,
                                    object_handle_t fd,
                                    l4_int32_t mode,
                                    const l4_threadid_t *notif_tid,
                                    CORBA_Server_Environment *_dice_corba_env)
{
  if (mode & ~(SELECT_READ | SELECT_WRITE | SELECT_EXCEPTION))
    return;
  
  deregister_select_notify(_dice_corba_obj, fd, notif_tid, mode);
}

/* ******************************************************************* */

void
local_socks_worker_done_component(CORBA_Object _dice_corba_obj,
                                  l4_addr_t *job_info,
                                  l4_int16_t *_dice_reply,
                                  CORBA_Server_Environment *_dice_corba_env)
{
  job_info_t *j = (job_info_t *) *job_info;

  LOGd(_DEBUG_WORKER, "worker thread "l4util_idfmt" is ready and waiting",
       l4util_idstr(*_dice_corba_obj));

  if ( !l4_task_equal(*_dice_corba_obj, l4_myself() )) {
    LOGd(_DEBUG_WORKER, "called by another task; this is not allowed!");
    return;
  }

  l4semaphore_down(&idle_workers_lock);

  if (num_idle_workers < num_sessions) {
    
    idle_workers[num_idle_workers] = *_dice_corba_obj;
    num_idle_workers++;
    *_dice_reply = DICE_NO_REPLY;
    LOGd(_DEBUG_WORKER, "letting worker "l4util_idfmt" sleep", l4util_idstr(*_dice_corba_obj));

  } else {
    /* maximum number of idle workers already reached, instruct the
     * worker to terminate itself (this will be done from within the
     * server loop, to save thread switches before delivering the
     * result to the client) */
    *_dice_reply = DICE_REPLY;
  }

  l4semaphore_up(&idle_workers_lock);

  reply_to_client(j);
}

/* ******************************************************************* */
/* ******************************************************************* */

static l4thread_t
start_worker(job_info_t *j)
{
  l4thread_t th;

  l4semaphore_down(&idle_workers_lock);
  if (num_idle_workers > 0) {

    /* there is an idle worker thread waiting, which can accept a new
       job right now */

    l4_threadid_t invalid_id = L4_INVALID_ID;
    l4_threadid_t idle_worker;
    l4_umword_t   dummy;
    l4_addr_t     stack_low, stack_high;

    num_idle_workers--;
    idle_worker = idle_workers[num_idle_workers];

    l4semaphore_up(&idle_workers_lock);

    LOGd(_DEBUG_WORKER, "restarting idle worker "l4util_idfmt" for session "l4util_idfmt,
         l4util_idstr(idle_worker), l4util_idstr(l4_myself()));

    /* restart worker thread */
    l4thread_data_set(l4thread_id(idle_worker), job_key, j);
    l4thread_get_stack(l4thread_id(idle_worker), &stack_low, &stack_high);
    l4_thread_ex_regs(idle_worker, (l4_umword_t) restart_worker_and_perform_job,
                      (l4_umword_t) stack_high - (sizeof(l4_umword_t) - 1),
                      &invalid_id, &invalid_id, &dummy, &dummy, &dummy);
    
    th = l4thread_id(idle_worker);

  } else {
    /* No luck. This will be expensive! */

    l4semaphore_up(&idle_workers_lock);

    th = l4thread_create_named(perform_job, ".worker", j, L4THREAD_CREATE_ASYNC);
    LOGd(_DEBUG_WORKER, "created new worker "l4util_idfmt" for session "l4util_idfmt,
         l4util_idstr(l4thread_l4_id(th)), l4util_idstr(l4_myself()));
  }

  return th;
}


void
shutdown_session_of_client(l4_threadid_t client)
{
  int i;

  if (l4_thread_equal(client, L4_INVALID_ID))
    return;

  /* mark session of 'client' as available */
  l4semaphore_down(&sessions_lock);
  for (i = 0; i < MAX_SESSIONS; i++) {
    if (l4_task_equal(sessions[i].client, client)) {
      sessions[i].client = L4_INVALID_ID;
      break;
    }
  }
  l4semaphore_up(&sessions_lock);

  /* close all sockets owned by 'client' */
  if (i < MAX_SESSIONS) {
    close_all_sockets_of_client(&client);  
    LOGd(_DEBUG, "connection to "l4util_idfmt" closed; session "l4util_idfmt" is idle",
         l4util_idstr(client), l4util_idstr(sessions[i].id));
  }
}

/* ******************************************************************* */

static void
session_thread(void *arg)
{
  l4thread_started(NULL);  
  local_socks_server_loop(NULL);
}


static void
perform_job(void *arg)
{
  CORBA_Environment env = dice_default_environment;
  job_info_t        *j = (job_info_t *) arg;

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

  local_socks_worker_done_call(&j->replier, (l4_addr_t *) &j, &env);
}


static void
restart_worker_and_perform_job(void)
{
  perform_job(l4thread_data_get_current(job_key));
  l4thread_exit();
}

/* ******************************************************************* */

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
    l4vfs_net_io_recv_reply(&j->client, j->retval, &j->out.recv.msg,
                            &j->out.recv.len, &env);
    /* j->out.recv.msg will be free()ed by dice's code or
       free_old_dice_buffers() */
    break;

  case JOB_TYPE_READ:
    l4vfs_common_io_read_reply(&j->client, j->retval, &j->out.recv.msg,
                               &j->out.recv.len, &env);
    /* j->out.recv.msg will be free()ed by dice's code or
       free_old_dice_buffers() */
    break;
#endif /* ! NO_WORKERTHREAD */

  default:
    LOG_Error("invalid value for job type %d", j->type);
  }

  CORBA_free(j);
}

/* ******************************************************************* */

int
start_server(int events_support)
{
  int i;

  master_session_id = l4_myself();
  num_sessions      = 0;
  num_idle_workers  = 0;
  
  for (i = 0; i < MAX_SESSIONS; i++) {
    sessions[i].id     = L4_INVALID_ID;
    sessions[i].client = L4_INVALID_ID;
  }
  
  job_key = l4thread_data_allocate_key();

  if ( !names_register("PF_LOCAL")) {
    LOG_Error("Failed to register PF_LOCAL at names!");
    return -1;
  }

  if (events_support && init_events() < 0) {
    names_unregister("PF_LOCAL");
    return -1;
  }
    
  local_socks_init();

  local_socks_server_loop(NULL);

  return -1; /* we should never get here */
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



#ifndef NO_WORKER_THREAD
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
#endif

