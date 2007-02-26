#include <names.h>
#include <l4/sys/syscalls.h>
#include <l4/util/string.h>


int
names_unregister_task(l4_threadid_t tid)
{
  message_t message;
  char	    buffer[MAX_NAME_LEN+1];

  names_init_message(&message, buffer);
  
  message.cmd = NAMES_UNREGISTER_TASK;
  message.id  = tid;

  return names_send_message(&message);
};
