/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#include <stdlib.h>
#include <l4/log/l4log.h>
#include "local.h"

// perform open
l4ore_handle_t ore_do_open(int handle,
		                   const char *dev, 
						   unsigned char mac[6],
                           l4ore_config *conf)
{
	DICE_DECLARE_ENV(_dice_corba_env);
	l4ore_handle_t ret;
	l4ore_config _conf;

	_dice_corba_env.malloc = (dice_malloc_func)malloc;
	_dice_corba_env.free   = (dice_free_func)free;
	LOG_Enter();

	// evil user provides us with no space for the MAC...
	if (mac == NULL)
	{
		LOG("No memory allocated for MAC address return value.");
		return L4_INVALID_ID;
	}

	// gracefully handle NULL pointer for config
	if (conf == NULL)
	{
		LOG("No memory allocated for connection configuration.");
		_conf = L4ORE_DEFAULT_CONFIG;
	}
	else
		_conf = *conf;

	// open()
	ret = ore_manager_open_call(&descriptor_table[handle].remote_manager_thread,
							  dev, mac, &_conf, &_dice_corba_env);

	LOG("opened. worker = "l4util_idfmt, l4util_idstr(ret));

	if (conf)
	  *conf = _conf;

	return ret;
}
