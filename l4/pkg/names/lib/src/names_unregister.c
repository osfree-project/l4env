#include <names.h>
#include <l4/sys/syscalls.h>
#include <l4/util/string.h>


int
names_unregister(const char* name)
{
  message_t message;
  char	    buffer[MAX_NAME_LEN+1];

  names_init_message(&message, buffer);
  
  message.cmd = NAMES_UNREGISTER;
  message.id = l4_myself();
  strncpy(buffer, name, MAX_NAME_LEN);

  return names_send_message(&message);
};
