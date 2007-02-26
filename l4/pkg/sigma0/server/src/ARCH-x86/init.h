#ifndef INIT_H
#define INIT_H

#include <l4/sys/compiler.h>
#include <l4/util/mb_info.h>

#define INIT_SECTION __attribute__((section (".init")))

void init(l4util_mb_info_t *mbi, unsigned int flag,
	  l4_kernel_info_t *info) L4_NORETURN;

#endif
