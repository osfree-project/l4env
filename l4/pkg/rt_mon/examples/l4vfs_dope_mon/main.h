#ifndef __RT_MON_EXAMPLES_L4VFS_DOPE_MON_MAIN_H_
#define __RT_MON_EXAMPLES_L4VFS_DOPE_MON_MAIN_H_

#include <l4/dope/dopelib.h>

extern long app_id;
extern int counter;

void press_callback(dope_event *e, void *arg);

#endif

