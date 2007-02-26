#include "local.h"
#include <stdlib.h>

int ore_send_string(l4ore_handle_t channel, char *data, unsigned int size)
{
  CORBA_Environment _dice_corba_env = dice_default_environment;
  _dice_corba_env.malloc            = (dice_malloc_func)malloc;
  _dice_corba_env.free              = (dice_free_func)free;

  return ore_ore_send_call(&ore_server, channel, data, size,
                           ORE_BLOCKING_CALL, &_dice_corba_env);
}
