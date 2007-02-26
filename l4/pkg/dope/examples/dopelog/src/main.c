#include <l4/oskit10_l4env/support.h>
#include <l4/log/l4log.h>
#include <l4/thread/thread.h>
#include <stdio.h>
#include "logserv.h"
#include "window.h"
#include "log_term.h"

extern int console_puts(const char *s);
static void my_LOG_outstring(const char *s) {
	console_puts(s);
}

int main(int argc, char* argv[]) {
	void (*old_LOG_outstring) (const char*) = LOG_outstring;
	LOG_outstring = my_LOG_outstring;
	strcpy(LOG_tag, "dope_log");

	OSKit_libc_support_init(16 * 1024);

	// start log server first,other processes depend on it
	LOG("starting log server");
	l4thread_create(logserver_loop, NULL, L4THREAD_CREATE_SYNC);
	LOG_outstring = old_LOG_outstring;
	LOG_init("dope_log");
	// start dope loop SYNC, to ensure that window is created when starting term_logger
	LOG("starting dope loop");
	l4thread_create(dope_loop, NULL, L4THREAD_CREATE_SYNC);
	LOG("starting dope logger");
	l4thread_create(log_term_loop, NULL, L4THREAD_CREATE_SYNC);

	LOG("all threads startet, exiting main(...)");
	return 0;
}
