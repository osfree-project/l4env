#include <names.h>
#include <l4/sys/syscalls.h>
#include <l4/util/string.h>

int
names_query_nr(const int nr, char* name, const int length, l4_threadid_t *id)
{
  message_t message;
  char	    buffer[MAX_NAME_LEN+1];
  int	    ret;

  names_init_message(&message, buffer);
  message.cmd = NAMES_QUERY_NR;
  message.id.lh.low = nr;

  ret = names_send_message(&message);
  if (ret)
    strncpy(name, buffer, MAX_NAME_LEN < length ? MAX_NAME_LEN : length);
    *id = message.id;
  return ret;
};
