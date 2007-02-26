#ifndef INIT_H
#define INIT_H

#include <l4/sys/compiler.h>

#define INIT_SECTION __attribute__((section (".init")))

void init(struct multiboot_info *mbi, unsigned int flag,
	  l4_kernel_info_t *info) L4_NORETURN;

#endif
