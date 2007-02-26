/* include original header */
#include_next <asm/page.h>

#undef __PAGE_OFFSET
#define __PAGE_OFFSET 0

extern void * bios_phys_to_virt(unsigned long);
#undef __va
#define __va(paddr) bios_phys_to_virt(paddr)
