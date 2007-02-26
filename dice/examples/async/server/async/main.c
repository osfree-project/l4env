/*
 * This is the async server.
 */
#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include "async-server.h"
#include "sync-client.h"

/* required global vars */
char LOG_tag[9] = "async";
l4_ssize_t l4libc_heapsize = 1*1024;

static l4_threadid_t caller[100];

void
async_foo_component(CORBA_Object _dice_corba_obj,
    long p1, long* p2, short *_dice_reply,
    CORBA_Server_Environment *_dice_corba_env)
{
  l4_threadid_t sync = L4_INVALID_ID;
  CORBA_Environment env = dice_default_environment;
  
  /* store caller's request */
  caller[p1] = *_dice_corba_obj;
  
  /* find sync server */
  if (!names_waitfor_name("sync", &sync, 1000))
    {
      LOG("failed to find sync");
      *p2 = -1;
      *_dice_reply = DICE_REPLY;
      return;
    }
  
  /* propagate to sync server (which will need some time) */
  LOG("propagate %ld to sync", p1);
  sync_bar_send(&sync, p1, &env);

  /* return to caller */
  *_dice_reply = DICE_NO_REPLY;
}

void
async_notify_component(CORBA_Object _dice_corba_obj,
    long p1,
    long result,
    CORBA_Server_Environment *_dice_corba_env)
{
  /* we should use our own environment here, because the 
   * _dice_corba_env "belongs" to the server sending the
   * reply.
   */
  CORBA_Server_Environment env = dice_default_server_environment;
  env.timeout = L4_IPC_TIMEOUT(0, 1, 0, 0, 15, 0);
  
  /* lookup caller's id using request id  == caller[p1] */
  if (l4_is_invalid_id(caller[p1]))
    {
      LOG("no client for response");
      return;
    }
  
  /* send reply to client */
  LOG("send reply for %ld: %ld", p1, result);
  async_foo_reply(&caller[p1], &result, &env);
}

int main(int argc, char* argv[])
{
  /* register at names */
  if (!names_register("async"))
    {
      LOG("failed to register at names");
      exit(1);
    }

  /* start server loop */
  async_server_loop(NULL);
}

