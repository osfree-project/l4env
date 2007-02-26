/****************************************************************
 * Generic send functions for ORe.                              *
 *                                                              *
 * Generic part of ORe's tx functionality.                      *
 *                                                              *
 * Bjoern Doebel <doebel@os.inf.tu-dresden.de>                  *
 * 2005-08-10                                                   *
 ****************************************************************/

#include <dice/dice.h>

#include "ore-local.h"

CORBA_int ore_ore_send_component(CORBA_Object _dice_corba_obj,
                       l4ore_handle_t channel,
                       const CORBA_char *buf,
                       l4_umword_t size,
                       int tx_blocking,
                       short *_dice_reply,
                       CORBA_Server_Environment *_dice_corba_env)
{
  int ret = 0;

  LOGd_Enter(ORE_DEBUG_COMPONENTS);

  l4lock_lock(&ore_connection_table[channel].channel_lock);

  if (ore_connection_table[channel].in_use == 0)
    {
      LOG_Error("Trying to send via unused connection.");
      ret = -L4_EBADF;
    }

  if (ore_connection_table[channel].config.rw_active == 0)
    {
      LOG_Error("Trying to send via inactive connection.");
      ret = -L4_EBADF;
    }

  if (ret == 0)
    ret = ore_connection_table[channel].tx_component_func(_dice_corba_obj,
                                                        channel, buf, size,
                                                        tx_blocking,
                                                        _dice_reply,
                                                        _dice_corba_env);

  l4lock_unlock(&ore_connection_table[channel].channel_lock);

  return ret;
}
