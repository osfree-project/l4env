#include <stdio.h>

#include <l4/sys/types.h>
#include <l4/names/libnames.h>

#include "hello-client.h"

char LOG_tag[9] = "dice_clt";

int main(void)
{
  l4_threadid_t hello_id;
  CORBA_Environment env = dice_default_environment;
  l4_uint32_t tmp;

  names_waitfor_name("dice_hello_server", &hello_id, 10000);

  printf("hello is %x.%x\n", hello_id.id.task, hello_id.id.lthread);
  
  hello_test_f1_call(&hello_id, 5, &tmp, &env);
  if (env.major != CORBA_NO_EXCEPTION)
    {
      printf("Fehler aufgetreten: %d.%d", 
	  env.major, env.repos_id);
      if (env.major == CORBA_SYSTEM_EXCEPTION)
	{
	  switch (env.repos_id)
	    {
	    case CORBA_DICE_EXCEPTION_WRONG_OPCODE:
	      printf("Server did not recognize the opcode\n");
	      break;
	    case CORBA_DICE_EXCEPTION_IPC_ERROR:
	      printf("IPC error occured (0x%x)\n", env._p.ipc_error);
	      break;
	    default:
	      printf("unrecognized error code (%d)\n", env.repos_id);
	      break;
	    }
	}
    }
  else
    printf("f1 returned: %d\n", tmp);

  tmp = hello_test_f2_call(&hello_id, 27, &env);
  printf("f2 returned %d\n", tmp);

  tmp = hello_test_f3_call(&hello_id, "blurf sabbel blabber!!! - und blub", &env);
  printf("f3 returned %d\n", tmp);

  return 0;
}
