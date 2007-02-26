/* 
 * Copyright (c) 1996 The University of Utah and
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
 * CPU identification code for x86 processors.
 */

#include <stdlib.h>
#include <flux/x86/eflags.h>
#include <flux/x86/proc_reg.h>
#include <flux/x86/cpuid.h>
#include <string.h>

static void get_cache_config(struct cpu_info *id)
{
	unsigned ci[4];
	unsigned cicount = 0;
	unsigned ccidx = 0;

	do
	{
		unsigned i;

		cicount++;
		asm volatile("cpuid"
			    : "=a" (ci[0]), 
			      "=b" (ci[1]),
			      "=c" (ci[2]),
			      "=d" (ci[3])
			    : "a" (2));

		for (i = 0; i < 4; i++)
		{
			unsigned reg = ci[i];
			if ((reg & (1 << 31)) == 0)
			{
				/* The low byte of EAX isn't a descriptor.  */
				if (i == 0)
					reg >>= 8;
				while (reg != 0)
				{
					if ((reg & 0xff) &&
					    (ccidx < sizeof(id->cache_config)))
					{
						id->cache_config[ccidx++] =
							reg & 0xff;
					}
					reg >>= 8;
				}
			}
		}
	}
	while (cicount < (ci[0] & 0xff));
}

void cpuid(struct cpu_info *out_id)
{
	int orig_eflags = get_eflags();

	memset(out_id, 0, sizeof(*out_id));

	/* Check for a dumb old 386 by trying to toggle the AC flag.  */
	set_eflags(orig_eflags ^ EFL_AC);
	if ((get_eflags() ^ orig_eflags) & EFL_AC)
	{
		/* It's a 486 or better.  Now try toggling the ID flag.  */
		set_eflags(orig_eflags ^ EFL_ID);
		if ((get_eflags() ^ orig_eflags) & EFL_ID)
		{
			int highest_val;

			/*
			 * CPUID is supported, so use it.
			 * First get the vendor ID string.
			 */
			asm volatile("cpuid" 
			            : "=a" (highest_val),
				      "=b" (*((int*)(out_id->vendor_id+0))),
				      "=d" (*((int*)(out_id->vendor_id+4))),
				      "=c" (*((int*)(out_id->vendor_id+8)))
				    : "a" (0));

			/* Now the feature information.  */
			if (highest_val >= 1)
			{
				asm volatile("cpuid"
				           : "=a" (*((int*)out_id)),
					     "=d" (out_id->feature_flags)
					     : "a" (1)
					     : "ebx", "ecx");
			}

			/* Cache and TLB information.  */
			if (highest_val >= 2)
				get_cache_config(out_id);
		}
		else
		{
			/* No CPUID support - it's an older 486.  */
			out_id->family = CPU_FAMILY_486;

			/* XXX detect FPU */
		}
	}
	else
	{
		out_id->family = CPU_FAMILY_386;

		/* XXX detect FPU */
	}

	set_eflags(orig_eflags);
}

