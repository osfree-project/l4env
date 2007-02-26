#ifndef __STARTUP_H__
#define __STARTUP_H__

#include <l4/sys/l4int.h>
#include <l4/util/mb_info.h>

#include "types.h"

#ifndef MEMORY
#define MEMORY 64
#endif

typedef struct 
{
  unsigned long kernel_low, kernel_high, kernel_start;
  unsigned long sigma0_low, sigma0_high, sigma0_start, sigma0_stack;
  unsigned long roottask_low, roottask_high, roottask_start, roottask_stack;
  unsigned long mem_lower, mem_upper, mem_high;
  unsigned long mbi_low, mbi_high;

} boot_info_t;

int have_hercules(void);

extern l4_addr_t _mod_end;

#endif /* ! __STARTUP_H__ */
