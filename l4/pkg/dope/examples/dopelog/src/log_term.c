#include "log_term.h"
#include <l4/dope/dopelib.h>
#include <l4/semaphore/semaphore.h>
#include <l4/thread/thread.h>
#include <stdio.h>
#include <malloc.h>
#include "stringlist.h"

extern long app_id;
extern l4semaphore_t sem_log_sl;
extern stringlist_t log_stringlist;

void log_term_loop(void *data) {
	char* msg;

	l4thread_started(NULL);
	while (1) {
		l4semaphore_down(&sem_log_sl);
		msg = fifo_out(&log_stringlist);
		if (msg) {
			// todo: \" in msg must be escaped
			dope_cmdf(app_id, "term.print(\"%s\")", msg);
			free(msg);
		}
	}
}
