/*
 * \brief   Simple overlay input test client
 * \date    2003-08-04
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*** GENERAL INCLUDES ***/
#include <stdlib.h>
#include <stdio.h>

/*** LOCAL INCLUDES ***/
#include "ovl_input.h"

int l4libc_heapsize = 1024*1024;

static void button_event_callback(int type, int code) {
	printf("ovlinputtest(button_event_callback): type=%d, code=%d\n", type, code);
}

static void motion_event_callback(int mx, int my) {
	printf("ovlinputtest(motion_event_callback): mx=%d, my=%d\n", mx, my);
}

int main(int argc, char **argv) {
	
	/* init overlay screen library */
	printf("ovlinputtest(main): init ovl_input lib\n");
	ovl_input_init(NULL);

	/* register input event callbacks */
	printf("ovlinputtest(main): register callbacks\n");
	ovl_input_button(button_event_callback);
	ovl_input_motion(motion_event_callback);
	
	printf("ovlinputtest(main): wasting some cpu time\n");
	while (1);
	return 0;
}
