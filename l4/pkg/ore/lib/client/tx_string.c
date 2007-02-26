/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

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
