
#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/env/errno.h>
#include <l4/sys/syscalls.h>
#include <l4/names/libnames.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include <l4/presenter/presenter-client.h>
#include <l4/presenter/presenter_lib.h>

int main(int argc, char **argv)
{
  l4_threadid_t presenter_id;
  int key;
  CORBA_Environment env = dice_default_environment;

  if (argc == 1)
    {
      printf("Need to give a path\n");
      return 1;
    }

  while (!names_waitfor_name(PRESENTER_NAME, &presenter_id, 2000))
    printf("Still looking around for %s\n", PRESENTER_NAME);

  key = presenter_load_call(&presenter_id, argv[1], &env);

  printf("Submitted %s to the presentation system\n", argv[1]);

  return 0;
}
