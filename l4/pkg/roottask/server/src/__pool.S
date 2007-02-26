
#include "libc_support.h"

/* We have to declare __malloc_pool here to make sure that .bss_pool
 * is typed ``@nobits''. */

	.section .bss_pool, "aw", %nobits
	.size	__malloc_pool, (MALLOC_POOL_SIZE)
	.type	__malloc_pool, %object
	.globl	__malloc_pool
__malloc_pool:
	.space	MALLOC_POOL_SIZE

