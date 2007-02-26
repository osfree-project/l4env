/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#include <stdlib.h>
#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include "local.h"

// perform open actions
void ore_do_close(int handle)
{
  DICE_DECLARE_ENV(_dice_corba_env);
  _dice_corba_env.malloc = (dice_malloc_func)malloc;
  _dice_corba_env.free   = (dice_free_func)free;
  l4ore_handle_t channel = descriptor_table[handle].remote_worker_thread;

  ore_manager_close_call(&descriptor_table[handle].remote_manager_thread, 
		                 &channel, &_dice_corba_env);

  // TODO: dispose dataspaces ?
}
