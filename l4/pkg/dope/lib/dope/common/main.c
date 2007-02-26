#include "dopelib-config.h"
#include <stdio.h>
#include <stdlib.h>
#include <dopelib.h>
#include "listener.h"
#include "sync.h"
#include "events.h"

extern struct dopelib_mutex *dopelib_cmd_mutex;


/*** BIND AN EVENT TO A DOpE WIDGET ***/
void dope_bind(long id,char *var,char *event_type,
               void (*callback)(dope_event *,void *),void *arg) {
	static char cmdbuf[256];
	sprintf(cmdbuf,"%s.bind(\"%s\",\"#! %lx %lx\")",
	        var,event_type,(uint32)callback,(uint32)arg);
	dope_cmd(id,cmdbuf);
}


/*** HANDLE EVENTLOOP OF A DOpE CLIENT ***/
void dope_eventloop(long id) {
	dope_event *e;
	char  *bindarg;
	while (1) {
		dopelib_wait_event(id, &e, &bindarg);

		/* test if cookie is valid */
		if ((bindarg[0]=='#') && (bindarg[1]=='!')) {
		
			/* determine callback adress and callback argument */
			uint32 num1 = 0;
			uint32 num2 = 0;
			char  *s = bindarg + 3;
			
			while (*s == ' ') s++;
			while ((*s != 0) && (*s != ' ')) {
				num1 = (num1<<4) + (*s & 15);
				if (*(s++)>'9') num1+=9;
			}
			if (*(s++) == 0) return;
			while ((*s != 0) && (*s != ' ')) {
				num2 = (num2<<4) + (*s&15);
				if (*(s++)>'9') num2+=9;
			}
			
			/* call callback routine */
			if (num1) ((void (*)(dope_event *,void *))num1)(e,(void *)num2);
		}
	}
}


/*** UNREGISTER DOpE APPLICATION */
long dope_deinit_app(long id) {
	return 0;
}


/*** WAIT FOR THE FINISHING OF THE CURRENT COMMAND AND BLOCK NET COMMANDS ***/
void dope_deinit(void) {
	dopelib_lock_mutex(dopelib_cmd_mutex);
}

void *CORBA_alloc(unsigned long size);

void *CORBA_alloc(unsigned long size) {
	return malloc(size);
}
