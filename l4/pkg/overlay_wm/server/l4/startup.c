/*
 * \brief   OverlayWM - native startup
 * \date    2003-08-18
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*** L4 INCLUDES ***/
#include <l4/log/l4log.h>

/*** LOCAL INCLUDES ***/
#include "startup.h"

l4_ssize_t l4libc_heapsize = 6*1024*1024;

void native_startup(int argc, char **argv) {
	LOG_init("ovlwm");
}
