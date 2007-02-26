/* local */
#include "local.h"

/* loglib tag */
char LOG_tag[9] = "flips-0";

l4_ssize_t l4libc_heapsize = 1024 * 1024;

/** MAIN
 *
 * krishna: If you have to initialize anything before liblinux is running,
 * implement an L4CTOR function prio L4CTOR_BEFORE_FLIPS_LIBLINUX.
 */
int main(int argc, char **argv)
{
	LOG_printf("Let's go...\n");

	if (liblinux_init(256*1024, 1024*1024))
		exit(1);
#if USE_LXDRV
	if (liblxdrv_init())
		exit(1);
#endif
	l4dde_do_initcalls();

	ifconfig("lo", "127.0.0.1", "255.0.0.0");
#if USE_LXDRV
	ifconfig("eth0", "192.168.128.13", "255.255.255.0");
#endif

	switch (2) {
	case 1:
		single_thread();
		break;
	case 2:
		multi_thread();
		break;
	default:
		LOG_printf("Nothing to do -> stop.\n");
	}

	return 0;
}

