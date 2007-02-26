/*
 * \brief	DOpE timer module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 */

#include "dope-config.h"

#include <l4/util/rdtsc.h>
#include <l4/util/macros.h>
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>

#include "timer.h"

int init_timer(struct dope_services *d);

/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/


/*** RETURN CURRENT SYSTEM TIME COUNTER IN MICROSECONDS ***/
static u32 get_time(void) {
#define TICKS_PER_SEC 1000000
  long long usecs;
  long long ns = l4_tsc_to_ns(l4_rdtsc());

//	u32 usecs;
//	u32 ns = (u32)(l4_tsc_to_ns(l4_rdtsc()) & 0xffffffff);
		
  usecs = ns / 1000;

#if 0
  __asm__ __volatile__
    ("                                      \n\t"
     "divl  %%esi                           \n\t"
     :"=a" (usecs)
     :"A" (ns),
     "S" (1000000000 / TICKS_PER_SEC)
     );
#endif
        
  return usecs & 0xffffffff;
//	return usecs;
}


/*** RETURN DIFFERENCE BETWEEN TWO TIMES ***/
static u32 get_diff(u32 time1,u32 time2) {

	/* overflow check */
	if (time1>time2) {
		time1 -= time2;
		return (u32)0xffffffff - time1;
	}
	return time2-time1;
}


/*** WAIT THE SPECIFIED NUMBER OF MICROSECONDS ***/
static void usleep(u32 num_usec) {
	l4thread_usleep(num_usec);
}

/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct timer_services services = {
	get_time,
	get_diff,
	usleep,
};


/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_timer(struct dope_services *d) {

	u32 scaler;

	if (!(scaler=l4_calibrate_tsc())) Panic("l4_calibrate_tsc: fucked up");
	
	d->register_module("Timer 1.0",&services);
	return 1;
}
