#ifndef __STARTUP_H__
#define __STARTUP_H__

#include <l4/sys/l4int.h>
#include <l4/util/mb_info.h>
#include <l4/sys/compiler.h>

#include "types.h"

#ifndef MEMORY
#define MEMORY 64
#endif

typedef struct 
{
  unsigned long kernel_start;
  unsigned long sigma0_start, sigma0_stack;
  unsigned long roottask_start, roottask_stack;
  unsigned long mbi_low, mbi_high;

} boot_info_t;

EXTERN_C_BEGIN

int have_hercules(void);

EXTERN_C_END

extern l4_addr_t _mod_end;

#endif /* ! __STARTUP_H__ */
