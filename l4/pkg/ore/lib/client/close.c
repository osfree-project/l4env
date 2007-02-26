#include <stdlib.h>
#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include "local.h"

// perform open actions
void ore_do_close(l4ore_handle_t handle)
{
  DICE_DECLARE_ENV(_dice_corba_env);
  _dice_corba_env.malloc            = (dice_malloc_func)malloc;
  _dice_corba_env.free              = (dice_free_func)free;

  ore_ore_close_call(&ore_server, handle, &_dice_corba_env);
}
