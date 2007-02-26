#include "test-client.h"
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/util/util.h>

#define BUFFER_SIZE 100
static char buffer[BUFFER_SIZE];

void
my_init(int nb,
    l4_umword_t *addr,
    l4_umword_t *size,
    CORBA_Environment *env)
{
  *addr = (l4_umword_t)buffer;
  *size = BUFFER_SIZE;
}

int
main(int argc, char** argv)
{
  l4_threadid_t srv;
  CORBA_Environment env = dice_default_environment;

  char *str;
  LOG_init("rcvstrC");

  names_waitfor_name("rcvstrS", &srv, 1000);

  LOG("*** test f1 ***");
  str = "Hallo Server, bist Du da? Wenn ja dann antworte mir!";
  test_f1_call(&srv, &str, &env);
  if (env.major != CORBA_NO_EXCEPTION)
    {
      LOG("Error: %d", env.major);
      if (env.major == CORBA_SYSTEM_EXCEPTION)
	{
	  LOG("IPC Error: 0x%x", env.ipc_error);
	}
    }
  LOG("rcvd: %s", str);

  str = "Attention server, are you there? If you are, please answer!";
  test_f2_call(&srv, str, &env);

  l4_sleep(2000);
  enter_kdebug("*#^init-rcvstring stopped");

  return 0;
}
