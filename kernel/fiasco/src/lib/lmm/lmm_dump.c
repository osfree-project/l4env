/* 
 * Copyright (c) 1995 The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 */
/*
 * Debugging routine:
 * dump an LMM memory pool and do a thorough sanity check on it.
 */

#ifndef DEBUG
#define DEBUG
#endif

#include <stdio.h>

#include "lmm_i.h"

void lmm_dump(lmm_t *lmm)
{
	struct lmm_region *reg;

	printf("lmm_dump(lmm=%p)\n", lmm);

	for (reg = lmm->regions; reg; reg = reg->next)
	{
		struct lmm_node *node;
		vm_size_t free_check;

		printf(" region %p-%p size=%p\n flags=0x%08x pri=0x%x free=%p\n",
			(void*)(reg->min), (void*)(reg->max), (void*)(reg->max - reg->min),
		       reg->flags, reg->pri, (void*)(reg->free));

		assert((reg->nodes == 0)
		       || (vm_offset_t)reg->nodes >= reg->min);
		assert((reg->nodes ==0) || (vm_offset_t)reg->nodes < reg->max);
		assert(reg->free <= reg->max - reg->min);

		free_check = 0;
		for (node = reg->nodes; node; node = node->next)
		{
			printf("  node %p-%p size=%p\n", 
				node, (void*)((vm_offset_t)node + node->size), (void*)(node->size));

			assert(((vm_offset_t)node & ALIGN_MASK) == 0);
			assert((node->size & ALIGN_MASK) == 0);
			assert(node->size >= sizeof(*node));
			assert((node->next == 0) || (node->next > node));
			assert((vm_offset_t)node < reg->max);

			free_check += node->size;
		}

		printf(" free_check=%p\n", (void*)free_check);
		assert(reg->free == free_check);
	}

	printf("lmm_dump done\n");
}

