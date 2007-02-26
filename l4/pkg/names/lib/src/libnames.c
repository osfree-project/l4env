#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>

#include <l4/rmgr/librmgr.h>
#include <l4/util/util.h>

#include <names.h>

static l4_threadid_t	ns;
static int initialized = 0;


static int
names_init(void)
{
  int error;

  if (!rmgr_init())
    return 0;
  
  error = rmgr_get_task_id("names", &ns);

  if (error)
    return 0;
  
  if (l4_is_invalid_id(ns))
    return 0;
  
  initialized = 1;
  return 1;
};


void
names_init_message(message_t* message, void* buffer)
{
  message->fpage.fpage = 0;
  message->size_dope = L4_IPC_DOPE(3,1);
  message->send_dope = L4_IPC_DOPE(3,1);
  message->string.snd_str = (l4_umword_t) buffer;
  message->string.rcv_str = (l4_umword_t) buffer;
  message->string.snd_size = MAX_NAME_LEN;
  message->string.rcv_size = MAX_NAME_LEN;
};


int
names_send_message(message_t* message)
{
  int error;
  l4_msgdope_t	result;

  if (!initialized)
    if (!names_init())
      return 0;

  error = l4_i386_ipc_call(ns, message,
			   message->id.lh.low, message->id.lh.high,
			   message,
			   &message->id.lh.low, &message->id.lh.high,
			   L4_IPC_NEVER, &result);

  if (error)
    return 0;

  return message->cmd;
};

