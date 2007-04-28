/* L4 */
#include <l4/util/macros.h>
#include <l4/log/l4log.h>
#include <l4/generic_io/libio.h>
#include <l4/dde_linux/dde.h>

#include <l4/socket_linux/socket_linux.h>

/* local */
#include "liblinux.h"
#include "local.h"

/* defined in linux sources (2.4.32) net/ipv4/ipconfig.c */
extern int ic_enable;

l4io_info_t *io_info_addr;

/** INITIALISATION OF DDE_LINUX */
static int dde_init(unsigned int vmem, unsigned int kmem)
{
	int err;

	l4io_init(&io_info_addr, L4IO_DRV_INVALID);
	Assert(io_info_addr);

	/* initialize all DDE modules required ... */
	/* XXX add cfg macros for memory */
	if ((err = l4dde_mm_init(vmem, kmem))) {
		LOG("initializing DDE_LINUX mm (%d)", err);
		return err;
	}
	/* tweak amount of memory reported to network code - we need not to reserve
	 * any memory for other tasks than networking */
	num_physpages *= 512;
#if 0 /* we're doing this via special implementation */
	if ((err = l4dde_softirq_init())) {
		Error("initializing softirqs (%d)", err);
		exit(-1);
	}
#endif
	if ((err = l4dde_process_init())) {
		LOG("initializing DDE_LINUX process-level (%d)", err);
		return err;
	}

	if ((err = l4dde_time_init())) {
		LOG("initializing DDE_LINUX time (%d)", err);
		return err;
	}
	return 0;
}

void * liblinux_get_l4io_info()
{
	return (void *)io_info_addr;
}

/** INITIALISATION OF LINUX NET
 *
 * Linux does it in net/socket.c.
 */
static int net_init(int dhcp)
{
	netlink_proto_init();
	rtnetlink_init();
	net_dev_init();             /* implicitly calls liblinux_lo_init.c */

	if (dhcp) {
		LOG("DHCP enabled");
		ic_enable = 1;
	}

	return 0;
}

/** INITIALISATION OF THE TCP/IP-STACK
 *
 * This function  must be called at first.
 * We initialize DDE before starting our submodules.
 */
int liblinux_init(unsigned int vmem, unsigned int kmem, int dhcp)
{
	int err;

	if ((err = dde_init(vmem, kmem))) {
		LOGd(DEBUG_LIBLX, "DDE initilization failed (%d)", err);
		return err;
	}

	/* our emulation framework */
	/* XXX errors? */
	liblinux_proc_init();
	liblinux_sysctl_init();
	liblinux_timer_init();
	liblinux_softirq_init();
	libsocket_linux_init();

	/* Linux */
	if ((err = net_init(dhcp))) {
		LOGd(DEBUG_LIBLX, "NET initialization failed (%d)", err);
		return err;
	}

	return 0;
}
