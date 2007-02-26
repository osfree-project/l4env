#ifndef __INIT_H__
#define __INIT_H__

#include <l4/sys/kernel.h>

#include "exec.h"

void init_l4_gmd(l4_kernel_info_t * l4i, exec_info_t * exec_info);
void init_ibm_nucleus(l4_kernel_info_t * l4i);
void init_hazelnut(l4_kernel_info_t ** l4i);

#endif /* ! __INIT_H__ */
