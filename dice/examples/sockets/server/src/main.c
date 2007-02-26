#include "test-server.h"

#ifndef L4API_linux
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#endif

int main(int argc, char** argv)
{
#ifndef L4API_linux
  LOG_init("server");
  names_register("server");
#endif
  // start server loop
  sock_srv_server_loop(NULL);
  // never reah this
#ifndef L4API_linux
  names_unregister("server");
#endif
  return 0;
}
