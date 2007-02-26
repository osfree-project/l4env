#include <stdio.h>

#include <l4/sys/types.h>
#include <l4/names/libnames.h>
#include <l4/util/l4_macros.h>

#include "hello-client.h"

char LOG_tag[9] = "dice_clt";

int main(void)
{
  l4_threadid_t hello_id;
  CORBA_Environment env = dice_default_environment;
  unsigned long tmp;

  names_waitfor_name("dice_hello_server", &hello_id, 10000);

  printf("hello is "l4util_idfmt"\n", l4util_idstr(hello_id));
  
  hello_test_f1_call(&hello_id, 5, &tmp, &env);
  if (DICE_HAS_EXCEPTION(&env))
    {
      printf("Fehler aufgetreten: %d.%d", 
	  DICE_EXCEPTION_MAJOR(&env),
	  DICE_EXCEPTION_MINOR(&env));
      if (DICE_IS_EXCEPTION(&env, CORBA_SYSTEM_EXCEPTION))
	{
	  switch (DICE_EXCEPTION_MINOR(&env))
	    {
	    case CORBA_DICE_EXCEPTION_WRONG_OPCODE:
	      printf("Server did not recognize the opcode\n");
	      break;
	    case CORBA_DICE_EXCEPTION_IPC_ERROR:
	      printf("IPC error occured (0x%x)\n", DICE_IPC_ERROR(&env));
	      break;
	    default:
	      printf("unrecognized error code (%d)\n", 
		  DICE_EXCEPTION_MINOR(&env));
	      break;
	    }
	}
    }
  else
    printf("f1 returned: %ld\n", tmp);

  tmp = hello_test_f2_call(&hello_id, 27, &env);
  printf("f2 returned %ld\n", tmp);

  tmp = hello_test_f3_call(&hello_id, "blurf sabbel blabber!!! - und blub",
      &env);
  printf("f3 returned %ld\n", tmp);

  return 0;
}
