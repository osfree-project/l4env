/*
 * \brief   Linux Event provider for Overlay WM
 * \date    2003-08-04
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This L4Linux user-level program receives input events as L4 IPC
 * calls from Overlay WM, converts them to the Linux input event
 * protocol and exports the event messages via a named pipe such
 * that another L4Linux process (e.g. XFree) can read input events
 * from this device via a plain Linux input event driver.
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the Overlay WM package, which is distributed
 * under the  terms  of the GNU General Public Licence 2. Please see
 * the COPYING file for details.
 */

/*** GENERAL INCLUDES ***/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

/*** LOCAL INCLUDES ***/
#include "ovl_input.h"

#define EV_KEY 0x01
#define EV_REL 0x02
#define EV_ABS 0x03

struct lxevent {
	long long time;
	unsigned short type;
	unsigned short code;
	unsigned int value;
};

int l4libc_heapsize = 1024*1024;
int lxev_fh;

static int verbose;


/*** UTILITY: SUBMIT LINUX EVENT TO NAMED PIPE ***/
static void submit_event(struct lxevent *ev) {
	if (!ev) return;
	write(lxev_fh, ev, sizeof(*ev));
}


/*** CALLBACK: CALLED FOR EACH INCOMING PRESS/RELEASE EVENT ***/
static void button_event_callback(int type, int code) {
	struct lxevent ev;

	if (verbose)
		printf("lxevent(button_event_callback): type=%d, code=%d\n", type, code);

	ev.type  = EV_KEY;               /* key/button       */
	ev.code  = code;                 /* keycode          */
	ev.value = (type == 1) ? 1 : 0;  /* press or release */
	submit_event(&ev);
}


/*** CALLBACK: CALLED FOR EACH MOUSE MOTION EVENT ***/
static void motion_event_callback(int mx, int my) {
	struct lxevent ev;
	static int curr_mx, curr_my;

	if (verbose)
		printf("lxevent(motion_event_callback): mx=%d, my=%d\n", mx, my);

	/*
	 * For each incoming motion event, we check if the
	 * vertical or horizontal position changed and
	 * send two distinct events for the horizontal and
	 * vertical component of the motion.
	 */

	if (mx != curr_mx) {
		ev.type  = EV_ABS;  /* absolute motion */
		ev.code  = 0;       /* horizontal      */
		ev.value = mx;      /* new x value     */
		submit_event(&ev);
	}

	if (my != curr_my) {
		ev.type  = EV_ABS;  /* absolute motion */
		ev.code  = 1;       /* vertical        */
		ev.value = my;      /* new y value     */
		submit_event(&ev);
	}

	/* remember current position */
	curr_mx = mx;
	curr_my = my;
}


int main(int argc, char **argv) {
	char *ovl_ident  = NULL;
	char *lxev_fname = NULL;

	if (argc <= 1) {
		printf("Usage: lxevent <pseudo event device> [<overlay wm identifier>]\n");
		return -1;
	}
	if (argc >= 2) lxev_fname = argv[1];
	if (argc >= 3) ovl_ident  = argv[2];

	/* create named pipe */
	if (mkfifo(lxev_fname, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) {
		printf("lxevent(main): Error: failed to create named pipe %s\n", lxev_fname);
		return -1;
	}

	/* install signal handler for signal SIGPIPE */
	signal(SIGPIPE, SIG_IGN);

	/* open named pipe for writing */
	lxev_fh = open(lxev_fname, O_WRONLY);
	if (lxev_fh < 0) {
		printf("lxevent(main): Error: cannot open named pipe %s for writing\n", lxev_fname);
		return -1;
	}

	/* init overlay event library */
	if (ovl_input_init(ovl_ident)) {
		printf("lxevent(main): Error: failed to initialize ovl_input lib\n");
		return -1;
	}

	/* register input event callbacks */
	ovl_input_button(button_event_callback);
	ovl_input_motion(motion_event_callback);

	/* process input events */
	ovl_input_eventloop();
	return 0;
}
