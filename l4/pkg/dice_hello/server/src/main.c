#include <stdio.h>

#include <l4/sys/syscalls.h>
#include <l4/names/libnames.h>

#include "hello-server.h"

int main(void)
{
  names_register("dice_hello_server");
  hello_test_server_loop(NULL);
  return 0;
}

void 
hello_test_f1_component(CORBA_Object _dice_corba_obj,
    l4_uint32_t t1,
    l4_uint32_t *t2,
    CORBA_Environment *_dice_corba_env)
{
  printf("hello: f1: %x\n", t1);
  *t2 = t1 * 10;
}

l4_uint16_t 
hello_test_f2_component(CORBA_Object _dice_corba_obj,
    l4_int32_t t1,
    CORBA_Environment *_dice_corba_env)
{
  printf("hello: f2: %x\n", t1);
  return (l4_int16_t)t1;
}

l4_uint32_t 
hello_test_f3_component(CORBA_Object _dice_corba_obj,
    const char* s,
    CORBA_Environment *_dice_corba_env)
{
  printf("f3: %s\n", s);
  return 923756345;
}
