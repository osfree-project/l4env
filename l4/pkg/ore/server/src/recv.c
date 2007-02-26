/****************************************************************
 * Generic receive functions for ORe.                           *
 *                                                              *
 * Generic part of ORe's rx functionality.                      *
 *                                                              *
 * Bjoern Doebel <doebel@os.inf.tu-dresden.de>                  *
 * 2005-08-10                                                   *
 *                                                              *
 * (c) 2005 - 2007 Technische Universitaet Dresden				*
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#include <linux/mm.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>

#include <l4/sys/types.h>
#include <l4/util/l4_macros.h>

#include "ore-local.h"

/* service receive requests */
CORBA_int
ore_rxtx_recv_component(CORBA_Object _dice_corba_obj,
                        CORBA_char **buf,
                        l4_size_t size,
                        l4_size_t *real_size,
                        CORBA_int rx_blocking,
                        CORBA_short *_dice_reply,
                        CORBA_Server_Environment *_dice_corba_env)
{
  int ret = 0;
  int channel = *(int *)l4thread_data_get_current(__l4ore_tls_id_key);

  LOGd(ORE_DEBUG_COMPONENTS, "recv on channel %d, blocking = %x", channel, rx_blocking);

  ret = sanity_check_rxtx(channel, *_dice_corba_obj);
  if (ret < 0)
      return ret;
  
  // go to do the real work --> this depends on the type of channel used
  ret = ore_connection_table[channel].rx_component_func(_dice_corba_obj,
                                                  buf, size, real_size, rx_blocking,
                                                  _dice_reply,
                                                  _dice_corba_env);

  return ret;
}

/* Internal notify component. This is called by netif_rx if a currently waiting
 * client receives a packet. This is necessary, because the IPC answer to this
 * client must come from the worker thread, not from the IRQ handler. (Damn the
 * L4v2 spec for that... ;)
 */
CORBA_void
ore_notify_rx_notify_component(CORBA_Object _dice_corba_obj,
                               CORBA_Server_Environment *_dice_corba_env)
{
  int channel = *(int *)l4thread_data_get_current(__l4ore_tls_id_key);
  ore_connection_table[channel].rx_reply_func(channel);
}

