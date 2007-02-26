#include <names.h>
#include <l4/sys/syscalls.h>
#include <l4/util/string.h>


int
names_query_name(const char* name, l4_threadid_t* id)
{
  message_t message;
  char	    buffer[MAX_NAME_LEN+1];
  int	    ret;

  names_init_message(&message, buffer);
  message.cmd = NAMES_QUERY_NAME;
  strncpy(buffer, name, MAX_NAME_LEN);
  
  ret = names_send_message(&message);
  if (ret)
    *id = message.id;
  return ret;
};
