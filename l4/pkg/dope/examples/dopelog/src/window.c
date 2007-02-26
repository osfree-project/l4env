#include <l4/oskit10_l4env/support.h>
#include <l4/sys/kdebug.h>
#include <l4/dope/dopelib.h>
#include <l4/lock/lock.h>
#include <l4/util/rdtsc.h>
#include <l4/sys/l4int.h>
#include "window.h"

extern void my_pc_reset(void);

long app_id;

static void bt_clear_press_callback(dope_event * e, void *arg) {
// todo: there is no appropriate method in terminal
	dope_cmd(app_id, "term.print(\"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\")");
}

static void bt_reboot_press_callback(dope_event * e, void *arg) {
	my_pc_reset();
}

static void time_loop(void *data) {
	l4_uint32_t s, ns;
	while (1) {
		l4thread_sleep(200);
		l4_tsc_to_s_and_ns(l4_rdtsc(), &s, &ns);
		dope_cmdf(app_id, "bt_time.set(-text \"uptime: %d:%.2d\")", s/60, s%60);
	}
}

void dope_loop(void *data) {
	l4_calibrate_tsc();
	dope_init();
	app_id = dope_init_app("DOpE_log");

	#include "window.i"
	dope_bind(app_id, "bt_reboot", "press", bt_reboot_press_callback, NULL);
	dope_bind(app_id, "bt_clear", "press", bt_clear_press_callback, NULL);
	dope_cmd(app_id, "win_main.open()");

	dope_cmdf(app_id, "bt_mhz.set(-text \"%d MHz\")", l4_get_hz()/1000000);

	l4thread_create(time_loop, NULL, L4THREAD_CREATE_ASYNC);
	l4thread_started(NULL);
	dope_eventloop(app_id);
}
