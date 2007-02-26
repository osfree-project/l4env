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

/* Send component function.
 *
 * - perform sanity and security checks
 * - call the client's real send function
 */
CORBA_int ore_rxtx_send_component(CORBA_Object _dice_corba_obj,
                       const CORBA_char *buf,
                       l4_size_t size,
                       CORBA_Server_Environment *_dice_corba_env)
{
  int ret = 0;
  int channel = *(int *)l4thread_data_get_current(__l4ore_tls_id_key);
  
  LOGd(ORE_DEBUG_COMPONENTS, "send on channel %d", channel);
  LOG_MAC(ORE_DEBUG_COMPONENTS, buf);

  ret = sanity_check_rxtx(channel, *_dice_corba_obj);
  if (ret < 0)
      return ret;
  
  if (ore_connection_table[channel].config.rw_active == 0)
    {
      LOG_Error("Trying to send via inactive connection.");
      ret = -L4_EBADF;
    }

  /* blocked-waiting clients can have a timeout. we recognize this here,
   * because if a client calls send(), it has stopped waiting for RX
   */
  if (ore_connection_table[channel].flags & ORE_FLAG_RX_WAITING &&
	  l4_thread_equal(ore_connection_table[channel].waiting_client,
		  			  *_dice_corba_obj))
	  ore_connection_table[channel].flags &= ~ORE_FLAG_RX_WAITING;


  // still no error? then go on
  if (!ret)
    ret = ore_connection_table[channel].tx_component_func(
                _dice_corba_obj, buf, size, _dice_corba_env);

  return ret;
}

