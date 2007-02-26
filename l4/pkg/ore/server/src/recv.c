/****************************************************************
 * Generic receive functions for ORe.                           *
 *                                                              *
 * Generic part of ORe's rx functionality.                      *
 *                                                              *
 * Bjoern Doebel <doebel@os.inf.tu-dresden.de>                  *
 * 2005-08-10                                                   *
 ****************************************************************/

#include <linux/mm.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>

#include <l4/sys/types.h>
#include <l4/util/l4_macros.h>

#include "ore-local.h"

static int service_waiting_clients(void);

/* Service all clients that are waiting for a rx packet. We check if such a
 * packet has arrived during execution of the rx_component function.
 */
static int service_waiting_clients(void)
{
  int i;
  int cnt = 0;

  for (i=0; i < ORE_CONFIG_MAX_CONNECTIONS; i++)
    {
      l4lock_lock(&ore_connection_table[i].channel_lock);
      // check for waiting threads
      if (ore_connection_table[i].flags & ORE_FLAG_RX_WAITING)
        {
          if (!list_empty(&ore_connection_table[i].rx_list))
            {
              ore_connection_table[i].flags &= ~ORE_FLAG_RX_WAITING;
              ore_connection_table[i].rx_reply_func(i);
              cnt++;
            }
        }
      l4lock_unlock(&ore_connection_table[i].channel_lock);
    }

  return cnt;
}

/* Service receive requests.
 */
CORBA_int
ore_ore_recv_component(CORBA_Object _dice_corba_obj,
                       l4ore_handle_t channel,
                       CORBA_char **buf,
                       l4_umword_t *size,
                       l4_umword_t *real_size,
                       CORBA_int rx_blocking,
                       CORBA_short *_dice_reply,
                       CORBA_Server_Environment *_dice_corba_env)
{
//  DICE_DECLARE_SERVER_ENV(env);
//  env.timeout = L4_IPC_NEVER;
  int ret;

  l4lock_lock(&ore_connection_table[channel].channel_lock);

  if (!ore_connection_table[channel].in_use)
    {
      LOG_Error("Trying to send via unused connection.");
      l4lock_unlock(&ore_connection_table[channel].channel_lock);
      return -L4_EBADF;
    }

  // go to do the real work --> this depends on the type of channel used
  ret = ore_connection_table[channel].rx_component_func(_dice_corba_obj,
                                                        channel, buf, size,
                                                        real_size, rx_blocking,
                                                        _dice_reply,
                                                        _dice_corba_env);

  l4lock_unlock(&ore_connection_table[channel].channel_lock);

  /* In the meantime new packets might have arrived. The IRQ thread then
   * will have enqueued them into the rx_lists and has tried to signal us.
   * As we were busy, this signal has not been received by the main thread,
   * therefore we check for new packets now.
   */
  while (service_waiting_clients())
    ;

  return ret;
}

/* Internal notify component. This is called by netif_rx if a currently waiting
 * client receives a packet. This is necessary, because the IPC answer to this
 * client must come from the main thread, not from the IRQ handler. (Damn the
 * L4v2 spec for that!!!!)
 */
CORBA_void
ore_ore_notify_rx_notify_component(CORBA_Object _dice_corba_obj,
                                   l4ore_handle_t channel,
                                   CORBA_Server_Environment *_dice_corba_env)
{
  l4lock_lock(&ore_connection_table[channel].channel_lock);
  ore_connection_table[channel].rx_reply_func(channel);
  l4lock_unlock(&ore_connection_table[channel].channel_lock);
}
