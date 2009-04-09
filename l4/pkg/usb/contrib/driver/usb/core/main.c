#include <l4/dde/linux26/dde26.h> /* l4dde26_*() */
// #include <l4/dde/linux26/dde26_net.h> /* l4dde26 networking */
#include <l4/log/l4log.h> /* LOG() */
// #include <l4/ore/ore-types.h> /* for skb LOG macros */
#include <l4/sys/types.h> /* l4_threadid_t */
#include <l4/sys/ipc.h> /* l4_ipc_*() */
#include <l4/util/util.h>


#include <linux/kernel.h>

#undef CONFIG_LOCK_KERNEL 



l4_threadid_t main_thread = L4_INVALID_ID;



int main(int argc, char **argv)
{
	LOG("Initializing DDE base system.");
	l4dde26_init();
	LOG("Initializing DDE kmalloc.");
	l4dde26_kmalloc_init();
	LOG("Initializing DDE process.");
	l4dde26_process_init();
	LOG("Initializing DDE timers.");
	l4dde26_init_timers();
	LOG("Initializing DDE softirq.");
	l4dde26_softirq_init();
	LOG("Doing initcalls...");
	l4dde26_do_initcalls();


	printk("initialized DDE.\n");

	main_thread = l4_myself();

  	l4_sleep_forever();

	return 0;
}
