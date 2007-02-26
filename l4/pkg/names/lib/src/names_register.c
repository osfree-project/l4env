#include <names.h>
#include <l4/sys/syscalls.h>
#include <l4/util/string.h>


/*!\brief Register the current thread with the given name
 *
 * The name is not necessarily 0-terminated when being sent to the server.
 */
int
names_register(const char* name)
{
  message_t message;
  char	    buffer[MAX_NAME_LEN];

  names_init_message(&message, buffer);

  message.cmd = NAMES_REGISTER;
  message.id = l4_myself();
  strncpy(buffer, name, MAX_NAME_LEN);

  return names_send_message(&message);
};
