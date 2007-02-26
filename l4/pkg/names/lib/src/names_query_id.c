#include <names.h>
#include <l4/sys/syscalls.h>
#include <l4/util/string.h>


int
names_query_id(const l4_threadid_t id, char* name, const int length)
{
  message_t message;
  char	    buffer[MAX_NAME_LEN+1];
  int	    ret;

  names_init_message(&message, buffer);
  message.cmd = NAMES_QUERY_ID;
  message.id = id;

  ret = names_send_message(&message);
  if (ret)
    strncpy(name, buffer, MAX_NAME_LEN < length ? MAX_NAME_LEN : length);
  return ret;
};
