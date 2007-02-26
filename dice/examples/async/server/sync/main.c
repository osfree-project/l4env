/*
 * Small server to delegate synchronous calls to
 */
#include <l4/util/util.h>
#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/thread/thread.h>
#include "sync-server.h"
#include "async-client.h"

/* required global vars */
char LOG_tag[9] = "sync";
l4_ssize_t l4libc_heapsize = 1*1024;

long
sync_foo_component(CORBA_Object _dice_corba_obj,
    long p1,
    CORBA_Server_Environment *_dice_corba_env)
{
  return p1*p1;
}

typedef struct
{
  l4_threadid_t caller;
  long p1;
} work_t;

static
void worker_func(void* parg)
{
  CORBA_Environment env = dice_default_environment;
  work_t arg = *(work_t*)parg;
  long sum = arg.p1 + arg.p1;
  
  /* wait to simulaty asynchronous processing */
  l4_sleep(100);
  
  /* the receiver of the notification, is the same
   * as the caller of this function.
   * !This might be different for you!
   *
   * We need to send p1 back, so async server can associate
   * response with request.
   */
  async_notify_call(&(arg.caller), arg.p1, sum, &env);
}
  
/* This function is called asynchronously, meaning
 * that it has been called and the caller proceeds
 * in its execution. This function has then (after
 * completion) to notifify the caller via an extra
 * interface (function).
 */
void
sync_bar_component(CORBA_Object _dice_corba_obj,
    long p1,
    CORBA_Server_Environment *_dice_corba_env)
{
  work_t arg;
  /* setup an extra worker thread, so server can
   * proceed.
   */
  arg.caller = *_dice_corba_obj;
  arg.p1 = p1;
  l4thread_create(worker_func, &arg, L4THREAD_CREATE_ASYNC);
}

int main(int argc, char* argv[])
{
  /* register at names */
  if (!names_register("sync"))
    {
      LOG("failed to register at names");
      exit(1);
    }

  sync_server_loop(NULL);

  for (;;) ;
}
