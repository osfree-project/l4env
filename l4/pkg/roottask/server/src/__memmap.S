
#include "memmap.h"

/* We have to declare __memmap here to make sure that .bss_memmap
 * is typed ``@nobits''. */

	.section .bss_memmap, "aw", %nobits
	.size	__memmap, PAGE_MAX
	.type	__memmap, %object
	.globl	__memmap
__memmap:
	.space	PAGE_MAX

