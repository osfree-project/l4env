#include "test-server.h"
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>

CORBA_void 
test_f1_component(CORBA_Object _dice_corba_obj,
    CORBA_char_ptr *t1,
    CORBA_Server_Environment *_dice_corba_env)
{
  LOG("rcvd: %s", *t1);
  *t1 = "Hey Jude, you're on my way! Dadadada, dadapdada-dadada.";
  LOG("reply: %s", *t1);
}

CORBA_void 
test_f2_component(CORBA_Object _dice_corba_obj,
    const_CORBA_char_ptr t2,
    CORBA_Server_Environment *_dice_corba_env)
{
  LOG("rcvd: %s", t2);
}

#define BUFFER_SIZE 256
static char buffer[BUFFER_SIZE];

void
my_init(int nb, 
    l4_umword_t* addr, 
    l4_umword_t* size,
    CORBA_Server_Environment *env)
{
  if (nb != 0)
    LOG("Hu?! init str %d???", nb);
  *addr = (l4_umword_t)buffer;
  *size = BUFFER_SIZE;
}

/* we have to provide a CORBA_free implementation,
 * which is never called in this example.
 */
void
CORBA_free(void* ptr)
{
  // do nothing
  if (ptr != buffer)
    LOG("try to free invalid ptr (%p)", ptr);
}

char LOG_tag[9] = "rcvstrS";

int 
main(int argc, char**argv)
{

  if (!names_register(LOG_tag))
    {
      LOG("could not register with names");
      return 1;
    }
  test_server_loop(NULL);

  LOG("Server loop exited!");
  return 0;
}
