/****************************************************************
 * ORe functions for opening/closing connections.               *
 *                                                              *
 * Bjoern Doebel <doebel@os.inf.tu-dresden.de>                  *
 * 2005-08-10                                                   *
 *                                                              *
 * (c) 2005 - 2007 Technische Universitaet Dresden				*
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#include <l4/l4rm/l4rm.h>
#include <dice/dice.h>

#include "ore-local.h"

/* FIXME no global vars, please */
extern unsigned char global_mac_address_head[4];

/********************************************************************
 * Open() component function. Sets up a connection if possible and  *
 * returns an ORe handle.                                           *
 ********************************************************************/
l4ore_handle_t ore_manager_open_component(CORBA_Object _dice_corba_obj,
                                      const_CORBA_char_ptr device_name,
                                      CORBA_unsigned_char mac[6],
                                      l4ore_config *conf,
                                      CORBA_Server_Environment *_dice_corba_env)
{
    int handle, ret;

    LOGd_Enter(ORE_DEBUG_COMPONENTS);
    LOGd(ORE_DEBUG_COMPONENTS, "opening device = '%s'", device_name);

    handle = getUnusedConnection();

    if (handle >= 0)
    {
        // init connection
        ret = setup_connection((char *)device_name, mac, 
              global_mac_address_head, conf, handle, _dice_corba_obj);
		LOG_MAC_s(ORE_DEBUG_COMPONENTS, "initialized: ", ore_connection_table[handle].mac);
        // return the worker thread on success
        if (ret == 0)
            return ore_connection_table[handle].worker;
        else
            return L4_INVALID_ID;
    }

    // return the error from getUnusedConnection here
    return L4_INVALID_ID;
}

/* Close a connection */
CORBA_void ore_manager_close_component(CORBA_Object _dice_corba_obj,
                                   const l4ore_handle_t *handle,
                                   CORBA_Server_Environment *_dice_corba_env)
{
  int channel = find_channel_for_worker(*handle);
  l4_threadid_t worker = ore_connection_table[channel].worker;
    
  LOGd_Enter(ORE_DEBUG_COMPONENTS);
  LOGd(ORE_DEBUG_COMPONENTS, "closing connection for client "l4util_idfmt, l4util_idstr(*_dice_corba_obj));

  // kill worker
  l4thread_shutdown(l4thread_id(worker));
  
  // free connection
  free_connection(channel);
}
