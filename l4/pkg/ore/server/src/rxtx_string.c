/****************************************************************
 * The string IPC module for ORe.                               *
 *                                                              *
 * This module contains all functions that are used to          *
 * implement ORe's rx and tx functionality using string IPC.    *
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
#include <l4/sys/ipc.h>
#include <l4/util/l4_macros.h>

#include "ore-local.h"

#define RX_REPLY_SUCCESS    0
#define RX_REPLY_FAIL       1
#define RX_REPLY_NO_DATA    2

static int rx_reply_string(int h, rxtx_entry_t *entry);

/* Reply to a client's receive request. Dequeue packet from rx list, try to send
 * the reply and re-enqueue the packet on error. 
 */
void rx_to_client_string(int h)
{
  rxtx_entry_t *entry;
  int err;

  LOGd_Enter(ORE_DEBUG_PACKET);

  /* dequeue packet from receive list */
  l4lock_lock(&ore_connection_table[h].channel_lock);

  /* For some reason (if any), this function is sometimes called even though
   * the rx list is empty.
   *
   * XXX: RACE CONDITION!
   *
   * For now, we just ignore these situations.
   */
  if (list_empty(&ore_connection_table[h].rx_list))
  {
	  l4lock_unlock(&ore_connection_table[h].channel_lock);
	  return;
  }
  entry = list_entry(ore_connection_table[h].rx_list.next, rxtx_entry_t, list);
  /* dequeue entry */
  list_del(&entry->list);
  l4lock_unlock(&ore_connection_table[h].channel_lock);

  Assert(entry);
  Assert(entry->skb);

  /* send reply */
  err = rx_reply_string(h, entry);
  switch(-err)
  {
	  /* free entry if it was sent successfully */
      case RX_REPLY_SUCCESS:
          free_rxtx_entry(entry);
          break;
	  /* on error (probably the client's receive buffer was too small),
	   * we re-enqueue the packet so that the client may adapt its receive
	   * size. */
      case RX_REPLY_FAIL:       // re-enqueue packet
          LOGd(ORE_DEBUG_PACKET, "error on reply_to_client. re-adding packet");
          l4lock_lock(&ore_connection_table[h].channel_lock);
          list_add(&entry->list, &ore_connection_table[h].rx_list);
          l4lock_unlock(&ore_connection_table[h].channel_lock);
          break;
      default:
		  /* should never get here!!! */
		  Assert(0);
          break;
    }
}

/* Send the skb included in entry to the client listening on handle h.
 *
 * We need this function, because replies can be generated from several
 * points:
 *  - netif_rx() --> rx_notify() iff the client is already waiting, and
 *  - recv_component() if the client requests a packet from his rx_list.
 */
static int rx_reply_string(int h, rxtx_entry_t *entry)
{
  int packet_size       = -1;
  l4_threadid_t client	= ore_connection_table[h].waiting_client;
  int size              = ore_connection_table[h].waiting_size;
  DICE_DECLARE_SERVER_ENV(env);

  Assert(entry);
  Assert(entry->skb);

  env.timeout           = L4_IPC_SEND_TIMEOUT_0;
  env.malloc            = (dice_malloc_func)CORBA_alloc;
  env.free              = (dice_free_func)CORBA_free;
  LOGd_Enter(ORE_DEBUG_PACKET);

  if (ORE_DEBUG_PACKET)
  {
      LOG_SKB(entry->skb);
  }

  packet_size = entry->skb->len;
  if (packet_size > size)
  {
      LOGd(ORE_DEBUG_COMPONENTS, "receive buffer too small");
      size = packet_size;
      ore_rxtx_recv_reply(&client, -L4_ENOMEM, (char **)&entry->skb->data,
                         &packet_size, &env);
      return -RX_REPLY_FAIL;
  }

  // reply
  LOGd(ORE_DEBUG_PACKET, "size = %d, reply_buf = %p", size,
                             entry->skb->data);
  
  // reset the waiting flag
  ore_connection_table[h].flags &= ~ORE_FLAG_RX_WAITING;
  
  ore_rxtx_recv_reply(&client, 0, (char **)&entry->skb->data,
                     &packet_size, &env);
  
  // signal success
  if (DICE_EXCEPTION_MAJOR(&env) == CORBA_NO_EXCEPTION)
  {
      ore_connection_table[h].packets_queued--;
      return RX_REPLY_SUCCESS;
  }

  // no success - need to re-enqueue packet
  return -RX_REPLY_FAIL;
}

/* Component function for rx via string ipc.
 *
 * Remember client and the size of the buffer he provides us, because replies
 * to rx requests are delayed in most cases (unless the call is non-blocking).
 *
 * If the rx list is empty, put the client asleep or return, depending on the
 * rx_blocking setting.
 *
 * If there are packets available for the client, go on to rx_to_client_string().
 */
int rx_component_string(CORBA_Object _dice_corba_obj,
                       char **buf, l4_size_t size,
                       l4_size_t *real_size, int rx_blocking,
                       short *_dice_reply,
                       CORBA_Server_Environment *_dice_corba_env)
{
  int channel = *(int *)l4thread_data_get_current(__l4ore_tls_id_key);  
  
  // we always use ore_recv_reply, therefore store the data within the
  // client state
  ore_connection_table[channel].waiting_client = *_dice_corba_obj;
  ore_connection_table[channel].waiting_size   = size;
  *_dice_reply = DICE_NO_REPLY;

  // no data?
  l4lock_lock(&ore_connection_table[channel].channel_lock);
  if (list_empty(&ore_connection_table[channel].rx_list))
    {
      if (rx_blocking == ORE_NONBLOCKING_CALL)
        {
          /* Dice-generated code calls CORBA_alloc() to allocate the buf
           * variable. As our CORBA_alloc does not return any memory in this
           * case, we must not rely on the buffer and use our own faked one
            for this reply.
           */
          static char fake_buf[ORE_CONFIG_MAX_BUF_SIZE];
          static char *p = fake_buf;
          l4lock_unlock(&ore_connection_table[channel].channel_lock);
          // non-blocking rx will return immediately
          ore_rxtx_recv_reply(_dice_corba_obj, -L4_ENODATA, &p,
                             real_size, _dice_corba_env);
          return 0;
        }
      else // ORE_BLOCKING_CALL
        {
          // client will block until data arrives - mark waiting 
          ore_connection_table[channel].flags |= ORE_FLAG_RX_WAITING;
          l4lock_unlock(&ore_connection_table[channel].channel_lock);
          return 0;
        }
    }

  l4lock_unlock(&ore_connection_table[channel].channel_lock);

  rx_to_client_string(channel);

  return 0;
}

/* The send function for the string IPC case.
 *
 * Add packet to the tx list and then hand over all tx packets currently
 * in the list to the device driver.
 */

int tx_component_string(CORBA_Object _dice_corba_obj,
                        const CORBA_char *buf,
                        l4_size_t size,
                        CORBA_Server_Environment *_dice_corba_env)
{
  struct net_device *dev;
  struct sk_buff *skb = NULL;
  int i = -1;
  int xmit = -1;
  int channel = *(int *)l4thread_data_get_current(__l4ore_tls_id_key);  
  rxtx_entry_t *ent = kmalloc(sizeof(rxtx_entry_t), GFP_KERNEL);

  LOGd(ORE_DEBUG_PACKET, "allocating skb");
  skb = alloc_skb(size, GFP_KERNEL);
  if (skb == NULL)
    {
      LOG_Error("failed to allocate send skb");
      return -L4_ENOMEM;
    }

  // no skb_get() here, because we do not want to be the owner of this skb
  // after committing it to the NIC
  ent->skb = skb;
  ent->in_dataspace = 0;

  LOGd(ORE_DEBUG_PACKET, "skb = %p", skb);
  LOG_MAC_s(ORE_DEBUG_PACKET_SEND, "packet for:", skb->data);

  // fill the skb with all data necessary for sending,
  memcpy(skb->data, buf, size);
  skb->len  = size;

  i = local_deliver(ent);

  // deliver to external client or broadcast
  if (i < 0 || mac_is_broadcast(ent->skb->data))
    {
      dev = ore_connection_table[channel].dev;
      do
      {
        while (netif_queue_stopped(dev))
          {
            /* Wait some time (1ms) until the drivers calls netif_wake_
             * queue(dev). We don't do l4_yield() here because that
             * does not work if the priority of the IRQ threads(!) is
             * less than the current priority. */
             l4_ipc_sleep(L4_IPC_TIMEOUT(0,0,250,14,0,0));
          }
        xmit_lock(dev->name);
        xmit = dev->hard_start_xmit(ent->skb, dev);
        xmit_unlock(dev->name);
        if (xmit)
            LOG_Error("Error sending packet!");
      } while (xmit != 0);

      ore_connection_table[channel].packets_sent++;
    }
    else  // we only did local_deliver() -> no NIC will ever free this skb, so
        // we do it ourselves
    {
        kfree_skb(skb);
    }

    // we kfree() the entry directly, because we do not need it anymore,
    // while the skb inside is still in possession of the NIC and will
    // be freed by the driver later on
    kfree(ent);

    /* There might occur situations, where the client called a blocking
     * receive and did not get an answer yet. If the client then sends
     * a packet, it is possible for him to have received an answer up
     * to here. Therefore we check for incoming packets now. */
    if (ore_connection_table[channel].flags & ORE_FLAG_RX_WAITING)
    {
        if (!list_empty(&ore_connection_table[channel].rx_list))
            rx_to_client_string(channel);
    }

  return 0;
}

/* netif_rx() for the string IPC module.
 *
 * Enqueues incoming packets to a client's rx list and notifies the main
 * thread about this event, if the client is already waiting for the next
 * packet. 
 */
int netif_rx_string(int h, struct sk_buff *skb)
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
  ent->skb = skb_get(skb);

  if (ORE_DEBUG_IRQ)
  {
      if (ent->skb->mac.raw)
        LOG_MAC(ORE_DEBUG_IRQ, ent->skb->mac.raw);
      else
        LOG_MAC(ORE_DEBUG_IRQ, ent->skb->data);
  }

  Assert(ent);
  Assert(ent->skb);

  // in every case we add this packet to the rx_list.
  l4lock_lock(&ore_connection_table[h].channel_lock);

  if (ore_connection_table[h].packets_queued < CONFIG_ORE_RX_QUOTA)
  {
	  list_add_tail(&ent->list, &(ore_connection_table[h].rx_list));
	  LOGd(ORE_DEBUG_PACKET, "added to list");
	  ore_connection_table[h].packets_received++;
	  ore_connection_table[h].packets_queued++;
  }
  else
  {
      LOGd(ORE_DEBUG_PACKET, "quota limit reached - removing first");
	  free_rxtx_entry(ent);
  }
  
  l4lock_unlock(&ore_connection_table[h].channel_lock);

  // notify the worker thread of a new packet (except for local deliver,
  // if I am the worker thread myself)
  if (!l4_thread_equal(ore_connection_table[h].worker, l4_myself()) &&
      (ore_connection_table[h].flags & ORE_FLAG_RX_WAITING))
    {
      DICE_DECLARE_ENV(_dice_corba_env);
      _dice_corba_env.malloc = (dice_malloc_func)CORBA_alloc;
      _dice_corba_env.free   = (dice_free_func)CORBA_free;
      l4_threadid_t worker = ore_connection_table[h].worker;

      // we call the worker thread with timeout 0. if the worker is waiting,
      // he will receive this and handle the reply. if the worker is busy,
      // we were fooled by the state of the waiting flag (the client really
      // is waiting, but there is already a packet being delivered and the
      // flag is not reset yet.)
      _dice_corba_env.timeout = L4_IPC_SEND_TIMEOUT_0;
      ore_notify_rx_notify_send(&worker, &_dice_corba_env);
    }

  return NET_RX_SUCCESS;
}
