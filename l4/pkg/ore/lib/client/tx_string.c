#include "local.h"
#include <stdlib.h>

int ore_send_string(l4ore_handle_t channel, int handle,
                    char *data, unsigned int size)
{
  DICE_DECLARE_ENV(_dice_corba_env);
  _dice_corba_env.malloc = (dice_malloc_func)malloc;
  _dice_corba_env.free   = (dice_free_func)free;

  return ore_rxtx_send_call(&channel, data, size, &_dice_corba_env);
}
