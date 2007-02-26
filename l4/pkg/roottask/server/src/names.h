#ifndef NAMES_H
#define NAMES_H

#include <l4/sys/types.h>

#include "types.h"

void          names_set(l4_threadid_t task, const char *name);
l4_threadid_t names_get_id(const char *name);

#endif


