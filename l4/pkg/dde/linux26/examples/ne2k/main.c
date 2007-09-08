#include <l4/dde/linux26/dde26.h> /* l4dde26_*() */
#include <l4/dde/linux26/dde26_net.h> /* l4dde26 networking */
#include <l4/log/l4log.h> /* LOG() */
#include <l4/ore/ore-types.h> /* for skb LOG macros */
#include <l4/sys/types.h> /* l4_threadid_t */
#include <l4/sys/ipc.h> /* l4_ipc_*() */

#include <linux/netdevice.h> /* struct sk_buff */
#include <linux/pci.h> /* pci_unregister_driver() */

#include "8390.h" /* that's what we are */

extern struct pci_driver ne2k_driver;
extern int arping(void);
l4_threadid_t main_thread = L4_INVALID_ID;

void open_nw_dev(void);
void open_nw_dev()
{
	struct net_device *dev;

	read_lock(&dev_base_lock);
	for (dev = dev_base; dev; dev = dev->next) {
		int err = 0;
		LOG("dev: '%s'", dev->name);

		err = dev_open(dev);
	}
	read_unlock(&dev_base_lock);
}

void close_nw_dev(void);
void close_nw_dev(void)
{
	struct net_device *dev;

	read_lock(&dev_base_lock);
	for (dev = dev_base; dev; dev = dev->next) {
		int err = 0;

		err = dev_close(dev);
		LOG("closed %s", dev->name);
	}
	read_unlock(&dev_base_lock);
}


static int net_rx_handle(struct sk_buff *skb)
{
    l4_msgdope_t res;
    int err = 0;
	l4_umword_t dw;

    skb_push(skb, skb->dev->hard_header_len);
    LOG("skb: %p", skb);
    LOG_MAC(1, skb->data);

    err = l4_ipc_call(main_thread, L4_IPC_SHORT_MSG,
	                  (l4_umword_t)skb, 0xCAFEBABE,
	                  L4_IPC_SHORT_MSG, &dw, &dw,
	                  L4_IPC_NEVER, &res);
    LOG("ipc_call: %d", err);
    
    kfree_skb(skb);

	LOG("freed skb, returning from netif_rx");
    return NET_RX_SUCCESS;
}


int main(int argc, char **argv)
{
	LOG("Initializing DDE base system.");
	l4dde26_init();
	l4dde26_process_init();

	LOG("Initializing skb subsystem");
	skb_init();

	LOG("Doing initcalls");
	l4dde26_do_initcalls();
	printk("initialized DDE.\n");

	l4dde26_register_rx_callback(net_rx_handle);

	open_nw_dev();
	LOG("dev is up and ready.");

	main_thread = l4_myself();
	arping();

	close_nw_dev();

	pci_unregister_driver(&ne2k_driver);
	LOG("shut down driver");

	return 0;
}
