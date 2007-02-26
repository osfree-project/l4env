#include "log_term.h"
#include <l4/dope/dopelib.h>
#include <l4/semaphore/semaphore.h>
#include <l4/thread/thread.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "stringlist.h"

extern long app_id;
extern l4semaphore_t sem_log_sl;
extern stringlist_t log_stringlist;

static char* GetEscaped(char *source) {
	char *dest;
	int len;
	int s, d=0;

	len = strlen(source);

	// escaped one can be at most twice as long
	dest = (char*) malloc(len*2+1);

	if (dest) {
		for (s=0; s<=len; s++) {
			switch (source[s]) {
				case '"':
					dest[d++] = '\\';
					break;
			}
			dest[d++] = source[s];
		}
	}

	return dest;
}

void log_term_loop(void *data) {
	char* msg;

	l4thread_started(NULL);
	while (1) {
		l4semaphore_down(&sem_log_sl);
		msg = fifo_out(&log_stringlist);
		if (msg) {
			char* msg_esc = GetEscaped(msg);
			if (msg_esc) {
				dope_cmdf(app_id, "term.print(\"%s\")", msg_esc);
				free(msg_esc);
			}
			free(msg);
		}
	}
}
