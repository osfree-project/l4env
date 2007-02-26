#include <stdio.h>

#include <l4/sys/types.h>
#include <l4/names/libnames.h>

#include "hello-client.h"

int main(void)
{
  CORBA_Object hello_id;
  CORBA_Environment env = dice_default_environment;
  l4_uint32_t tmp;

  names_waitfor_name("flick_hello_server", &hello_id, 10000);

  printf("hello is %x.%x\n", hello_id.id.task, hello_id.id.lthread);
  
  hello_test_f1_call(&hello_id, 5, &tmp, &env);
  printf("f1 returned: %d\n", tmp);

  tmp = hello_test_f2_call(&hello_id, 27, &env);
  printf("f2 returned %d\n", tmp);

  tmp = hello_test_f3_call(&hello_id, "blurf sabbel blabber!!! - und blub", &env);
  printf("f3 returned %d\n", tmp);

  return 0;
}
