/****************************************************************
 * The string IPC module for ORe.                               *
 *                                                              *
 * This module contains all functions that are used to          *
 * implement ORe's rx and tx functionality using string IPC.    *
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

static int rx_reply_string(l4ore_handle_t h, rxtx_entry_t *entry);

/* Reply to a client waiting for an rx packet. Dequeue the next packet from
 * the client's rx_list and send it to him.
 *
 * !!! This function is called with the channel_lock helt for this
 * connection. !!!
 */
void rx_to_client_string(l4ore_handle_t h)
{
  rxtx_entry_t *entry;

  entry = list_entry(ore_connection_table[h].rx_list.next, rxtx_entry_t, list);
  list_del(&entry->list);

  // if reply was successfull, free the entry
  if (rx_reply_string(h, entry) == 0)
    free_rxtx_entry(entry);
  else // an error occured -> re-enqueue the packet
    {
      list_add(&entry->list, &ore_connection_table[h].rx_list);
    }
}

/* Send the skb included in entry to the client listening on handle h.
 *
 * !!! This function is called with the channel_lock helt for this
 * connection. !!!
 *
 * We need this function, because replies can be generated from several
 * points:
 *  - netif_rx() --> rx_notify() iff the client is already waiting, and
 *  - recv_component() if the client requests a packet from his rx_list.
 */
static int rx_reply_string(l4ore_handle_t h, rxtx_entry_t *entry)
{
  int packet_size       = -1;
  l4_threadid_t client	= ore_connection_table[h].waiting_client;
  int size              = ore_connection_table[h].waiting_size;
  DICE_DECLARE_SERVER_ENV(env);

  env.timeout           = L4_IPC_NEVER;

  if (!entry->buf)
    {
      LOG_Error("No skb found.");

      // silently fail...
      return 0;
    }

  LOGd(ORE_DEBUG_COMPONENTS, "entry = %p", entry);
  if (ORE_DEBUG_COMPONENTS)
  {
      LOG_SKB(entry->buf);
  }

  packet_size = entry->buf->len;
  if (packet_size > size)
  {
      LOGd(ORE_DEBUG_COMPONENTS, "receive buffer too small");
      size = packet_size;
      ore_ore_recv_reply(&client, -L4_ENOMEM, (char **)&entry->buf->data,
                         &size, &packet_size, &env);
      return -1;
  }

  // reply
  LOGd(ORE_DEBUG_COMPONENTS, "size = %d, reply_buf = %p", size,
                             entry->buf->data);
  ore_ore_recv_reply(&client, 0, (char **)&entry->buf->data, &size,
                     &packet_size, &env);

  // cleanup
  LOGd(ORE_DEBUG_COMPONENTS, "cleaning up");

  return 0;
}

/* real component function for rx via string ipc.
 *
 * !!! this func is called with the channel lock helt for this channel. !!!
 */
int rx_component_string(CORBA_Object _dice_corba_obj,
                       l4ore_handle_t channel, char **buf, l4_umword_t *size,
                       l4_umword_t *real_size, int rx_blocking,
                       short *_dice_reply,
                       CORBA_Server_Environment *_dice_corba_env)
{
  // we always use ore_recv_reply, therefore store the data within the
  // client state
  ore_connection_table[channel].waiting_client = *_dice_corba_obj;
  ore_connection_table[channel].waiting_size   = *size;
  *_dice_reply = DICE_NO_REPLY;

  // no data?
  if (list_empty(&ore_connection_table[channel].rx_list))
    {
      //LOGd(ORE_DEBUG_COMPONENTS, "list is empty");
      if (rx_blocking == ORE_NONBLOCKING_CALL)
        {
          /* Dice-generated code calls CORBA_alloc() to allocate the buf
           * variable. As our CORBA_alloc does not return any memory in this
           * case, we must not rely on the buffer and use our own faked one
           * for this reply.
           */
          static char fake_buf[ORE_CONFIG_MAX_BUF_SIZE];
          static char *p = fake_buf;
          // non-blocking rx will return immediately
          ore_ore_recv_reply(_dice_corba_obj, -L4_ENODATA, &p,
                             size, real_size, _dice_corba_env);
//          l4lock_unlock(&ore_connection_table[channel].channel_lock);
          return 0;
        }
      else // ORE_BLOCKING_CALL
        {
          // block until data arrives
          // mark waiting and store client
          ore_connection_table[channel].flags |= ORE_FLAG_RX_WAITING;
//          l4lock_unlock(&ore_connection_table[channel].channel_lock);
          return 0;
        }
    }

  rx_to_client_string(channel);

  return 0;
}

/* The real send function for the string IPC case.
 *
 * !!! Function is called with the channel lock helt for this channel. !!!
 */

int tx_component_string(CORBA_Object _dice_corba_obj,
                        l4ore_handle_t channel,
                        const CORBA_char *buf,
                        l4_umword_t size,
                        int tx_blocking,
                        short *_dice_reply,
                        CORBA_Server_Environment *_dice_corba_env)
{
  struct net_device *dev;
  struct sk_buff *skb = NULL;
  int ret = 0;
  rxtx_entry_t *ent = kmalloc(sizeof(rxtx_entry_t), GFP_KERNEL);

  LOGd(ORE_DEBUG_COMPONENTS, "allocating skb");
  skb = alloc_skb(size, GFP_KERNEL);
  if (skb == NULL)
    {
      LOG_Error("failed to allocate send skb");
      return -L4_ENOMEM;
    }

  // no skb_get() here, because we do not want to be the owner of this skb
  // after committing it to the NIC
  ent->buf = skb;

  // enqueue into tx_list
//  l4lock_lock(&ore_connection_table[channel].channel_lock);
  list_add_tail(&ent->list, &(ore_connection_table[channel].tx_list));
//  l4lock_unlock(&ore_connection_table[channel].channel_lock);

  // non-blocking send will return now
  if (tx_blocking == ORE_NONBLOCKING_CALL)
    {
      *_dice_reply = DICE_NO_REPLY;
      ore_ore_send_reply(_dice_corba_obj, 0, _dice_corba_env);
      // no return, because real work still has to be performed
    }

  LOGd(ORE_DEBUG_COMPONENTS, "skb = %p", skb);

  // fill the skb with all data necessary for sending,
  memcpy(skb->data, buf, size);
  skb->len  = size;

  // go sending...
  while (!list_empty(&ore_connection_table[channel].tx_list))
    {
      int i;

      ent = list_entry(ore_connection_table[channel].tx_list.next,
                       rxtx_entry_t, list);
      list_del(&ent->list);

      i = local_deliver(ent);

      // deliver to external client or broadcast
      if (i < 0 || mac_is_broadcast(ent->buf->data))
        {
          dev = ore_connection_table[channel].dev;
          ret = dev->hard_start_xmit(ent->buf, dev);
        }

      // we kfree() the entry directly, because we do not need it anymore,
      // while the skb inside is still in possession of the NIC and will
      // be freed by the driver later on
      kfree(ent);
    }

  return ret;
}

int netif_rx_string(l4ore_handle_t h, struct sk_buff *skb)
{
  // create new rxtx entry
  rxtx_entry_t *ent = kmalloc(sizeof(rxtx_entry_t), GFP_KERNEL);

  if (ent == NULL)
    {
      LOG_Error("failed to kmalloc rxtx entry");
      return -L4_ENOMEM;
    }

  // make sure everything is clean
  memset(ent, 0, sizeof(rxtx_entry_t));

  // skb_get will make sure that the usage count is incremented
  // for each client in need of this skb
  ent->buf = skb_get(skb);

  LOG_MAC(ORE_DEBUG_IRQ, ent->buf->mac.raw);
/*  LOGd(ORE_DEBUG_IRQ, "MAC = %02X:%02X:%02X:%02X:%02X:%02X",
                       ent->buf->mac.raw[0], ent->buf->mac.raw[1],
                       ent->buf->mac.raw[2], ent->buf->mac.raw[3],
                       ent->buf->mac.raw[4], ent->buf->mac.raw[5]);
*/

  LOGd(ORE_DEBUG_IRQ, "this is for channel: %d", h);
  LOGd(ORE_DEBUG_IRQ, "entry = %p", ent);
  LOGd(ORE_DEBUG_IRQ, "buf = %p", skb);

  // in every case we add this packet to the rx_list.
  list_add_tail(&ent->list, &(ore_connection_table[h].rx_list));

  // if the client is already waiting for a packet, signal arrival of
  // the packet to the main thread
  if (ore_connection_table[h].flags & ORE_FLAG_RX_WAITING)
    {
      DICE_DECLARE_ENV(_dice_corba_env);
      // we call the main thread with timeout 0. if the server is waiting,
      // he will receive this and handle the reply. if the server is busy,
      // he will check for new packets in the end. this is necessary,
      // because we do not want to block this IRQ thread.
      _dice_corba_env.timeout = L4_IPC_SEND_TIMEOUT_0;
      // remove waiting flag
      ore_ore_notify_rx_notify_send(&ore_main_server, h, &_dice_corba_env);
      // reset flag only if IPC was successful
      if (_dice_corba_env.major == CORBA_NO_EXCEPTION)
        ore_connection_table[h].flags &= ~ORE_FLAG_RX_WAITING;
    }

  return NET_RX_SUCCESS;
}
