/****************************************************************
 * ORe functions for opening/closing connections.               *
 *                                                              *
 * Bjoern Doebel <doebel@os.inf.tu-dresden.de>                  *
 * 2005-08-10                                                   *
 ****************************************************************/

#include <l4/l4rm/l4rm.h>
#include <dice/dice.h>

#include "ore-local.h"

/* FIXME no global vars, please */
extern unsigned char global_mac_address_head[4];

l4ore_handle_t ore_ore_open_component(CORBA_Object _dice_corba_obj,
                                      const_CORBA_char_ptr device_name,
                                      CORBA_unsigned_char mac[6],
                                      const l4dm_dataspace_t *send_ds,
                                      const l4dm_dataspace_t *recv_ds,
                                      l4ore_config *conf,
                                      CORBA_Server_Environment *_dice_corba_env)
{
  l4ore_handle_t handle;
  int ret;

  LOGd_Enter(ORE_DEBUG_COMPONENTS);
  LOGd(ORE_DEBUG_COMPONENTS, "opening device = '%s'", device_name);

  handle = getUnusedConnection();
  // if we receive no error
  if (handle >= 0)
    {
      // init connection
      ret = setup_connection((char *)device_name, mac, send_ds, recv_ds,
                             global_mac_address_head, conf, handle);
      // return the connection handle on success
      if (ret == 0)
        return handle;
      else
        return ret;
    }

  // return the error from getUnusedConnection here
  return handle;
}

CORBA_void ore_ore_close_component(CORBA_Object _dice_corba_obj,
                                   l4ore_handle_t handle,
                                   CORBA_Server_Environment *_dice_corba_env)
{
    free_connection(handle);
}
