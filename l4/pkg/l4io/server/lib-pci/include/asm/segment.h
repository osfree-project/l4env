#ifndef _LIBPCI_ASM_SEGMENT_H_
#define _LIBPCI_ASM_SEGMENT_H_

#include_next <asm/segment.h>

static inline short bios_get_cs(void)
{
	unsigned cs;
	asm volatile ("mov %%cs, %0" : "=r"(cs));
	return cs;
}

#endif
