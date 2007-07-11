/**
 * \file   l4io/server/src/static_cfg.c
 * \brief  Simple static resource configuration
 *
 * \date   2007-07-06
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/sys/types.h>

#include <stdio.h>

/* local */
#include "io.h"
#include "res.h"


struct device_mem_resource
{
	int       cfg;    /* special token for runtime configuration (see ARM) */
	l4_addr_t start;  /* first byte of resource region */
	l4_addr_t end;    /* last byte of resource region */
};


static struct device_mem_resource mem_resources[] =
{
#ifdef ARCH_arm
	{ CFG_RV_EB_926,  0x10006000, 0x10006fff },  /* AMBA KMI keyboard */
	{ CFG_RV_EB_926,  0x10007000, 0x10007fff },  /* AMBA KMI mouse */

	{ CFG_RV_EB_MC,   0x10006000, 0x10006fff },  /* AMBA KMI keyboard */
	{ CFG_RV_EB_MC,   0x10007000, 0x10007fff },  /* AMBA KMI mouse */

	{ CFG_INTEGRATOR, 0x18000000, 0x18000fff },  /* AMBA KMI keyboard */
	{ CFG_INTEGRATOR, 0x19000000, 0x19000fff },  /* AMBA KMI mouse */
#endif

#ifdef ARCH_x86
	{ CFG_STD, 0xfed40000, 0xfed44fff },  /* TPM 1.2 TIS area (see TPM 1.2 spec) */
#endif

#ifdef ARCH_amd64
	{ CFG_STD, 0xfed40000, 0xfed44fff },  /* TPM 1.2 TIS area (see TPM 1.2 spec) */
#endif

	{ 0, 0 }
};

int io_static_cfg_init(int cfg_token)
{
	struct device_mem_resource *m;

	printf("begin of static cfg...............\n");

	for (m = mem_resources; (m->start != 0) && (m->end != 0); m++)
		if (m->cfg == cfg_token)
			callback_announce_mem_region(m->start, m->end - m->start + 1);

	printf("end of static cfg.................\n");

	return 0;
}
