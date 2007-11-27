#include <stdio.h>

#include <l4/sys/types.h>
#include <l4/sys/ipc.h>

#include "global.h"
#include "helper.h"

#undef PREFIX
#define PREFIX(a) generic_ ## a
#include "helper_inc.h"
