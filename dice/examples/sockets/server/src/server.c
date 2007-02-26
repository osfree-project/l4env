#include "test-server.h"
#include <stdio.h>

CORBA_long sock_srv_func1_component(CORBA_Object _dice_corba_obj,
    CORBA_long a,
    CORBA_Environment *_dice_corba_env)
{
  CORBA_long _return = 12345;
  printf("func1 @ server received %d; return %d\n", (int)a, (int)_return);
  return _return;
}

const char* test_string = "Hello World!";

CORBA_long sock_srv_func2_component(CORBA_Object _dice_corba_obj,
    CORBA_char_ptr *str,
    CORBA_Environment *_dice_corba_env)
{
  CORBA_long _return = 67890;
  printf("func2 @ server returns %d and \"%s\"\n",(int)_return, test_string);
  *str = (CORBA_char_ptr)test_string;
  return _return;
}

CORBA_void sock_srv_func3_component(CORBA_Object _dice_corba_obj,
    const str_A *t,
    CORBA_Environment *_dice_corba_env)
{
  printf("t.a = %ld\n", t->a);
  printf("t.b = %ld\n", t->b);
  printf("t.c.d = %ld\n", t->c.d);
#ifdef SOCKETAPI
  printf("t.c.e = %hd\n", t->c.e);
#else
  printf("t.c.e = %d\n", t->c.e);
#endif
}

CORBA_void sock_srv_func4_component(CORBA_Object _dice_corba_obj,
    const_CORBA_char_ptr str,
    const_CORBA_char_ptr str2,
    CORBA_Environment *_dice_corba_env)
{
  printf("func4 @ server: String \"%s\" empfangen\n", str);
  printf("func4 @ server: String2 \"%s\" empfangen\n", str2);
}

CORBA_void sock_srv_func5_component(CORBA_Object _dice_corba_obj,
    const_CORBA_char_ptr str,
    CORBA_Environment *_dice_corba_env)
{
  printf("func5 @ server: String \"%s\" empfangen\n", str);
}

CORBA_long sock_srv_func6_component(CORBA_Object _dice_corba_obj,
    const_CORBA_char_ptr str,
    const_CORBA_char_ptr str2,
    CORBA_Environment *_dice_corba_env)
{
  printf("func6 @ server: String \"%s\" empfangen\n", str);
  printf("func6 @ server: String2 \"%s\" empfangen\n", str2);
  printf("func6 @ server: return %d\n", 32452654);
  return 32452654;
}

