#ifndef __STARTUP_H__
#define __STARTUP_H__

#include <l4/sys/types.h>
#include <l4/util/mb_info.h>

#include "types.h"

/* Data */
extern l4_addr_t kernel_low;
extern l4_addr_t kernel_high;

extern l4util_mb_mod_t mb_mod[MODS_MAX];

/* Functions */
int have_hercules(void);

#endif /* ! __STARTUP_H__ */
