/**
 * \file   server/src/dmon.c
 * \brief  Demon - An evil supernatural being
 *
 * \date   10/01/2004
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 */
#include <l4/util/reboot.h>
#include <l4/util/rdtsc.h>
#include <l4/log/l4log.h>
#include <l4/thread/thread.h>
#include <l4/dope/dopelib.h>

/* standard */
#include <stdlib.h>

/* local */
#include "local.h"

l4_ssize_t l4libc_heapsize = 600 * 1024;

/*** FANCY STUFF (taken from dopelog by Thomas Friebel) ***/
static void reboot_callback(dope_event * ev, void *priv)
{
	LOG("Rebooting.\n");
	l4util_reboot();
}

static void time_loop(void *data)
{
	long app_id = (long)data;
	l4_uint32_t s, ns;

	while (1) {
		l4thread_sleep(200);
		l4_tsc_to_s_and_ns(l4_rdtsc(), &s, &ns);
		dope_cmdf(app_id, "l_time.set(-text \"uptime: %d:%02d:%02d\")",
		          s / 60 / 60, (s / 60) % 60, s % 60);
	}
}

/*** DUMPER WINDOW ACTIVATE BUTTON ***/
static void dumper_callback(dope_event *ev, void *priv)
{
	static int button_state = 0;

	long app_id = (long)priv;

	if (button_state) {
		/* was pressed before - close DUMPER */
		dope_cmd(app_id, "b_dumper.set(-state off)");
		button_state = 0;
		dope_cmd(app_id, "dumper.close()");
	} else {
		/* was not pressed - open DUMPER */
		dope_cmd(app_id, "b_dumper.set(-state on)");
		button_state = 1;
		dope_cmd(app_id, "dumper.open()");
	}
}

/*** GUI INITIALIZATION ***/
static int gui_init(long app_id)
{
	/* window description */
	#include "dmon.dpi"

	dope_bind(app_id, "b_dumper", "press", dumper_callback, (void *)app_id);
	dope_bind(app_id, "b_reboot", "commit", reboot_callback, NULL);

	/* fancy stuff */
	l4_calibrate_tsc();
	dope_cmdf(app_id, "l_mhz.set(-text \"%d MHz\")", l4_get_hz()/1000000);
	l4thread_create(time_loop, (void *)app_id, L4THREAD_CREATE_ASYNC);

	return 0;
}

/*** THE FAMOUS MAIN ***/
int main(void)
{
	int err;
	long app_id;

	/* start logserver as early as possible */
	err = dmon_logsrv_init();
	if (err) {
		LOG_Error("LOGGER I init failed (%d)", err);
		/* FIXME do cleanups */
		exit(1);
	}

	/* init DOpE */
	err = dope_init();
	if (err) {
		LOG_Error("dope_init() returned %d", err);
		exit(1);
	}

	app_id = dope_init_app(APP_NAME);

	/* init remainder */
	err = gui_init(app_id);
	if (err) {
		LOG_Error("GUI init failed (%d)", err);
		/* FIXME do cleanups */
		exit(1);
	}

	err = dmon_logger_init(app_id);
	if (err) {
		LOG_Error("LOGGER II init failed (%d)", err);
		/* FIXME do cleanups */
		exit(1);
	}

	dope_cmd(app_id, "w.open()");

	err = dmon_dumper_init(app_id);
	if (err) {
		LOG_Error("DUMPER init failed (%d)", err);
		/* FIXME do cleanups */
		exit(1);
	}

	/* enter infinite (?) loop */
	dope_eventloop(app_id);

	LOG_Error("XXX left dope_eventloop() XXX");
	exit(1);
}
