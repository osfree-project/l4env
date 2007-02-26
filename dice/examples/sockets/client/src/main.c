#include "test-client.h"

#ifndef SOCKETAPI
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>

static char gstr[2048];
#endif

void* CORBA_alloc(unsigned long size)
{
#ifdef SOCKETAPI
  return malloc(size);
#else
  return (void*)gstr;
#endif
}

int main(int argc, char**argv)
{
  CORBA_char_ptr str;
  CORBA_long ret;
  CORBA_Object srv;
  str_A t;
  CORBA_Environment env = dice_default_environment;
#ifdef SOCKETAPI
  srv.sin_family = AF_INET;
  srv.sin_port = DICE_DEFAULT_PORT;
  inet_aton("127.0.0.1", &srv.sin_addr);
#else
  LOG_init("client");
  names_waitfor_name("server", &srv, 120);
#endif

  printf("send request to server(23412)\n");

  ret = sock_srv_func1_call(&srv, 23412, &env);
  printf("func1 @ client returned %d\n", (int)ret);

  ret = sock_srv_func2_call(&srv, &str, &env);
  printf("func2 @ client returned %d, \"%s\"\n", (int)ret, str);

  t.a = 123;
  t.b = 456;
  t.c.d = 1234567890;
#ifdef SOCKETAPI
  printf("send t = { %ld, %ld, { %ld | %hd } }\n", t.a, t.b, t.c.d, t.c.e);
#else
  printf("send t = { %ld, %ld, { %ld | %d } }\n", t.a, t.b, t.c.d, t.c.e);
#endif
  sock_srv_func3_call(&srv, &t, &env);

  sock_srv_func4_call(&srv, "Hello World! with func4", 
      "Huhu again, wann werden wir uns wieda sehn?", &env);
  sock_srv_func5_call(&srv, "Hey! Don't you listen?", &env);

  ret = sock_srv_func6_call(&srv, "Test of return value with...", "... two in strings.", &env);
  printf("func6 @ client: returned %d\n", ret);

  return 0;
}
