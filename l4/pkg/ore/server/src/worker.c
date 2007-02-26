/*****************************************************************************
 * Worker threads for ORe.													 *
 * 						                                                     *
 * Bjoern Doebel <doebel@os.inf.tu-dresden.de>                               *
 * 2005-10-06                                                                *
 *																			 *
 * (c) 2005 - 2007 Technische Universitaet Dresden							 *
 * This file is part of DROPS, which is distributed under the				 *
 * terms of the GNU General Public License 2. Please see the				 *
 * COPYING file for details.												 *
 *****************************************************************************/

#include "ore-local.h"

int __l4ore_tls_id_key = -1;

static void worker_exit(l4thread_t thread, void *data);
L4THREAD_EXIT_FN(exit_fn, worker_exit);

// worker thread for string ipc connections
void worker_thread_string(void *arg)
{
// XXX: dde_add_worker !!!!!!
    int *my_channel = kmalloc(sizeof(int), GFP_KERNEL);

    DICE_DECLARE_SERVER_ENV(env);
    env.malloc = (dice_malloc_func)CORBA_alloc;
    env.free   = (dice_free_func)CORBA_free;
    
    *my_channel = *(int *)arg;

    LOGd(ORE_DEBUG_COMPONENTS, "(String IPC)Worker thread initializing: "l4util_idfmt, 
            l4util_idstr(l4_myself()));
    LOGd(ORE_DEBUG_COMPONENTS, "Worker for connection %d", *my_channel);

    l4thread_data_set_current(__l4ore_tls_id_key, my_channel);

    // register exit handler for cleanup
    l4thread_on_exit(&exit_fn, NULL);

    l4thread_started(NULL);

    LOGd(ORE_DEBUG_COMPONENTS, "entering server loop");

    ore_server_worker_server_loop(&env);
}

#ifdef ORE_DSI
// worker thread for sending packets from dsi connections
void worker_thread_dsi(void *arg)
{
    int *my_channel = kmalloc(sizeof(int), GFP_KERNEL);
    
    *my_channel = *(int *)arg;
    
    LOGd(ORE_DEBUG_COMPONENTS, "(DSI)Worker thread initializing: "l4util_idfmt, 
            l4util_idstr(l4_myself()));
    LOGd(ORE_DEBUG_COMPONENTS, "Worker for connection %d", *my_channel);

    l4thread_data_set_current(__l4ore_tls_id_key, my_channel);

    // register exit handler for cleanup
    l4thread_on_exit(&exit_fn, NULL);
    
    l4thread_started(NULL);
    // await startup
    l4lock_lock(&ore_connection_table[*my_channel].tx_startlock);
    LOGd(ORE_DEBUG_COMPONENTS, "entering dsi loop");
    
    while (1)
    {
        int ret = -1;
        int xmit = -1;
        l4_size_t size;
        dsi_packet_t *packet;
        dsi_socket_t *sock = ore_connection_table[*my_channel].tx_socket;
        void *addr;
        struct sk_buff *skb;
        struct net_device *dev = ore_connection_table[*my_channel].dev;
        struct rxtx_entry_t *ent = kmalloc(sizeof(struct rxtx_entry_t), GFP_KERNEL);

        ret = dsi_packet_get(sock, &packet);
        LOGd(ORE_DEBUG_DSI, "packet_get: %d", ret);
        if (ret)
        {
            LOG_Error("error on packet_get: %d", ret);
        }

        ret = dsi_packet_get_data(sock, packet, &addr, &size);
        LOGd(ORE_DEBUG_DSI, "get_data: %d (%p, size = %d)", ret, addr, size);
        if (ret)
        {
            LOG_Error("error on packet_get_data: %d", ret);
        }

        // create skb
        skb 		= alloc_skb(size, GFP_KERNEL);
        skb->len 	= size;
        skb->data 	= addr;
        
        // build rxtx_entry
        ent->skb 			= skb;
        ent->in_dataspace 	= 1;

		/* The DSI packet is not commited at the end of this function, because
		 * we need to keep this packet until the device driver has really sent
		 * the packet away. Therefore we remember the packet and handle commits
		 * in kfree_skb().
		 */     
        __l4ore_remember_packet(packet, sock, addr, size);

        // TODO: local delivery!
        
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

      	ore_connection_table[*my_channel].packets_sent++;
    }
}
#endif

static void worker_exit(l4thread_t thread, void *data)
{
    int *key = l4thread_data_get_current(__l4ore_tls_id_key);
    kfree(key);
    l4thread_data_set_current(__l4ore_tls_id_key, NULL);
}
