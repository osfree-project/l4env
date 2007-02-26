#include "test-server.h"

#ifndef SOCKETAPI
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#endif

int main(int argc, char** argv)
{
#ifndef SOCKETAPI
  LOG_init("server");
  names_register("server");
#endif
  // start server loop
  sock_srv_server_loop(NULL);
  // never reah this
#ifndef SOCKETAPI
  names_unregister("server");
#endif
  return 0;
}
