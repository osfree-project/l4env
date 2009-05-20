#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/kthread.h>

//BEER for L4
#include <l4/sys/kdebug.h>
#include <l4/util/util.h>
#include <l4/dde/linux26/dde26.h>

#if 0
#include <l4/ferret/client.h>
#include <l4/ferret/clock.h>
#include <l4/ferret/types.h>
#include <l4/ferret/util.h>
#include <l4/ferret/sensors/list_producer.h>
#include <l4/ferret/sensors/list_producer_wrap.h>
#endif

static int idle_thread(void *);
int __init init_module(void);
void __exit cleanup_module(void);
#if 0
ferret_list_local_t *sensor;
#endif

enum Events
{
	DDE_INIT           = 10,
	DDE_INITCALLS      = 11,
	INIT_MODULE        = 12,
	START_THREAD       = 13,
	THREAD_INIT        = 14,
	THREAD_SLEEP_START = 15,
	THREAD_SLEEP_END   = 16
};

atomic_t val = ATOMIC_INIT(0);

static int idle_thread(void *priv_data)
{
    int master_thread_counter = 0;
#if 0
	ferret_list_post_1t1wc(1, sensor, 42, 1, 0, me,THREAD_INIT);
#endif

    while (1)
    {
        master_thread_counter++;
        if ((master_thread_counter % 100) == 0)
			atomic_inc(&val);
#if 0
            printk(KERN_INFO "master_thread_counter: %d jiffies: %d\n",
                   master_thread_counter, jiffies);
#endif

        // do something interesting

        // wait for 10ms assuming HZ=100
        set_current_state(TASK_INTERRUPTIBLE);
//		ferret_list_post_1t1wc(1, sensor, 42, 1, 0, me,THREAD_SLEEP_START);
        schedule_timeout(1);
//		ferret_list_post_1t1wc(1, sensor, 42, 1, 0, me,THREAD_SLEEP_END);
        //msleep(10);
    }
}

int __init init_module(void)
{
    int i;
    printk(KERN_INFO "init module\n");

    for (i = 0; i < 10; i++)
    {
//		ferret_list_post_1t1wc(1, sensor, 42, 1, 0, l4_myself(),START_THREAD);
        kthread_run(idle_thread, NULL, "IDLE THREAD");
        udelay(1000);
    }
    // no error check
	return 0;
}

void __exit cleanup_module(void)
{
    printk(KERN_INFO "exit module\n");
    // kill thread
}

MODULE_AUTHOR("test");
MODULE_DESCRIPTION("test");
MODULE_LICENSE("GPL");

static void *kmalloc_wrap(size_t sz)
{ return kmalloc(sz,GFP_KERNEL); }

int main(void)
{
	int old, v = 12000;
	printk("Initializing Ferret\n");
#if 0
	int ret = ferret_create(42, 1, 0, FERRET_LIST, 0,
							"64:100000", sensor, &kmalloc_wrap);
	printk("Created sensor: %d\n", ret);

	ferret_list_post_1t1wc(1, sensor, 42, 1, 0, l4_myself(),DDE_INIT);
#endif
	printk("Initializing DDE base system.\n");

#if 0
	ferret_list_post_1t1wc(1, sensor, 42, 1, 0, l4_myself(),DDE_INITCALLS);
#endif

#if 0
	ferret_list_post_1t1wc(1, sensor, 42, 1, 0, l4_myself(),INIT_MODULE);
#endif
	init_module();

	while (1)
	{
		l4_sleep(3000);
		old = v;
		v = atomic_read(&val);
		printk("%d - %d = %d\n", v, old, v-old);
		if (v - old == 0)
		{
			printk("System halt!? %d\n", v);
			//enter_kdebug("..");
#if 0
			ferret_list_entry_common_t * elc;

			int idx = ferret_list_dequeue(sensor);
			elc = (ferret_list_entry_common_t *)ferret_list_e4i(sensor->glob, idx);
			elc->major = FERRET_MONCON_MAJOR;
			elc->minor = FERRET_MONCON_SERSEND;
			strncpy(elc->data8, "foobar", 7);
			ferret_list_commit(sensor, idx);
			printk("Dumped %d\n", idx);
			l4_sleep(10000);
#endif
			enter_kdebug("");
		}
	}

	l4_sleep_forever();
	return -1;
}
