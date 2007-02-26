#ifndef MEMMAP_H
#define MEMMAP_H

#include <l4/sys/compiler.h>
#include <l4/sys/types.h>
#include <l4/sys/kernel.h>

#include "mem_man.h"
#include "globals.h"


extern Mem_man iomem;

extern l4_kernel_info_t	*l4_info;
extern l4_addr_t	tbuf_status;

#ifdef __cplusplus
extern "C" {
#endif
void pager(void) L4_NORETURN;

#ifdef __cplusplus
}
#endif


#endif
