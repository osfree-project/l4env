#include <stdlib.h>
#include <linux/ide.h>
#include <l4/dde_linux/dde.h>
#include <linux/pci.h>
#include <l4/util/macros.h>
#include <l4/generic_io/libio.h>
#include <l4/log/l4log.h>
#include <l4/env/errno.h>
#include <l4/names/libnames.h>

#include <l4/dm_mem/dm_mem.h>
#include <linux/mm.h>
#include <linux/bio.h>
#include <linux/module.h>

#include <driver/driver.h>
#include <driver/notification.h>
#include <driver/config.h>

char LOG_tag[9] = CONFIG_NAME;
const int l4thread_max_threads = CONFIG_L4IDE_MAX_THREADS;
l4_ssize_t l4libc_heapsize = CONFIG_L4IDE_COMMAND_STACK_SIZE;

static int ide_initio(void)
{
	l4io_info_t *info_addr = NULL;
	l4io_init(&info_addr,L4IO_DRV_INVALID);
	if (l4dde_pci_init()) Panic("Couldn't init PCI");
	l4dde_mm_init(4000000,4000000);
	l4dde_time_init();
	l4dde_softirq_init();
	l4dde_irq_init(1);
	l4dde_process_init();
	l4dde_workqueues_init();

	return 0;
}

int main(int argc, char *argv[])
{
	Assert(!ide_initio());
	l4dde_do_initcalls();
	l4ide_init_notifications();
	l4ide_init_driver_instances();

	if (!names_register(CONFIG_NAME)) {
	    LOG_Error("register at nameserver failed!");
	    exit(1);
	}

	LOG("IDE-Server is up");

	l4ide_driver_service_loop();

	return 0;
}
