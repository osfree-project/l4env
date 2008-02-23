#include <l4/util/macros.h>
#include <l4/log/l4log.h>

#include <l4/dde/linux26/dde26.h>

#include <l4/socket_linux/socket_linux.h>

/* local */
#include "liblinux.h"
#include "local.h"

/* defined in linux sources (2.6.20.19 net/ipv4/ipconfig.c))
 * and made accessible (not static) for FLIPS */
extern int ic_enable;

/** Initialization of dde26*/
static int dde_init(unsigned int vmem, unsigned int kmem)
{
        LOG("Initializing DDE base system.");
	l4dde26_init();
	l4dde26_kmalloc_init();
	l4dde26_process_init();
	l4dde26_init_timers();
	l4dde26_softirq_init();

        LOG("Initializing skb subsystem");
	skb_init();

        return 0;
}

/*
 * Initialize the IP Stack
 */
int liblinux_init(unsigned int vmem, unsigned int kmem, int dhcp)
{
  int err;

  if ((err = dde_init(vmem, kmem))) {
    LOGd(DEBUG_LIBLX, "DDE initilization failed (%d)", err);
    return err;
  }

  liblinux_proc_init();
//	liblinux_sysctl_init();
  if ((err = liblinux_timer_init())) {
    LOGd(DEBUG_LIBLX, "Timer initialization failed (%d)", err);
    return err;
  }
         
//	liblinux_softirq_init();
  libsocket_linux_init();

  if (dhcp) {
    ic_enable = 1;
  }

  l4dde26_do_initcalls();

  return 0;
}
