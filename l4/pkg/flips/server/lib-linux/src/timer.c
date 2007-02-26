#include <l4/util/rdtsc.h>
#include <l4/util/macros.h>

#include <linux/time.h>

#include "local.h"

/* XXX tv->tv_sec is long _not_ long long */
/* we also could use io_info->xtime for that */
void do_gettimeofday(struct timeval *tv)
{
	l4_uint64_t secs;
	l4_uint64_t usecs = l4_tsc_to_us(l4_rdtsc());

	secs = usecs / 1000000;

#if 0
	tv->tv_sec = secs & 0x7fffffff;
	tv->tv_usec = (usecs % 1000000) & 0x7fffffff;
	LOG("%d:%06d", tv->tv_sec, tv->tv_usec);
#else
	tv->tv_sec = secs;
	tv->tv_usec = usecs % 1000000;
#endif
}


/** INITIALISATION OF THE TIMER MODULE 
 *
 * This function must be called once before the ip-stack is started.
 */
int liblinux_timer_init(void);
int liblinux_timer_init(void)
{
	unsigned int scaler;

	if (!(scaler = l4_calibrate_tsc())) {
		Panic("l4_calibrate_tsc: fucked up");
		return -1;
	}

	return 0;
}

