/*
 * This is the client (or rather the clients) which will initiate
 * requests to the asynchronous server, so one can see the interleaved 
 * processing of the requests.
 */
#include <l4/thread/thread.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/util/util.h>
#include <l4/sys/syscalls.h>
#include <l4/util/l4_macros.h>
#include "async-client.h"

l4_threadid_t async_id = L4_INVALID_ID;

char LOG_tag[9] = "client";
l4_ssize_t l4libc_heapsize = 1*1024;

static
void request(void* parg)
{
  CORBA_Environment env = dice_default_environment;
  long result;
  long arg = *(long*)parg;
  l4_threadid_t myself = l4_myself();

  l4thread_started(NULL);

  LOG("Call async(%ld) (" l4util_idfmt ")", arg,
      l4util_idstr(myself));
  async_foo_call(&async_id, arg, &result, &env);
  LOG("async returned %ld (" l4util_idfmt ")", result,
      l4util_idstr(myself));

  l4thread_exit();
}

int main(int argc, char* argv[])
{
  int i = 0;
  /* find async server */
  if (!names_waitfor_name("async", &async_id, 10000))
    {
      LOG("Failed to find 'async'\n");
      exit(1);
    }
  
  /* create multiple threads initiating the requests to the
   * server
   */
  for (; i<100; i++)
    l4thread_create(request, (void*)&i, L4THREAD_CREATE_SYNC);

  l4_sleep(-1);

  for (;;) ;
}
