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
#include <l4/generic_io/types.h>
#include <l4/log/l4log.h>

#include <string.h>

/* local */
#include "io.h"
#include "res.h"
#include "__config.h"
#include "__macros.h"


/************************
 ** Device descriptors **
 ************************/

#if defined(ARCH_arm)
static l4io_desc_device_t
	rv_eb_926_kbd = { "AMBA KMI kbd", 2,
		{
			{ L4IO_RESOURCE_MEM, 0x10006000, 0x10006fff },
			{ L4IO_RESOURCE_IRQ, 52, 52 }
		}
	},
	rv_eb_926_mou = { "AMBA KMI mou", 2,
		{
			{ L4IO_RESOURCE_MEM, 0x10007000, 0x10007fff },
			{ L4IO_RESOURCE_IRQ, 53, 53 }
		}
	},
	rv_eb_mc_kbd = { "AMBA KMI kbd", 2,
		{
			{ L4IO_RESOURCE_MEM, 0x10006000, 0x10006fff },
			{ L4IO_RESOURCE_IRQ, 39, 39 }
		}
	},
	rv_eb_mc_mou = { "AMBA KMI mou", 2,
		{
			{ L4IO_RESOURCE_MEM, 0x10007000, 0x10007fff },
			{ L4IO_RESOURCE_IRQ, 40, 40 }
		}
	},
	integrator_kbd = { "AMBA KMI kbd", 2,
		{
			{ L4IO_RESOURCE_MEM, 0x18000000, 0x18000fff },
			{ L4IO_RESOURCE_IRQ, 20, 20 }
		}
	},
	integrator_mou = { "AMBA KMI mou", 2,
		{
			{ L4IO_RESOURCE_MEM, 0x19000000, 0x19000fff },
			{ L4IO_RESOURCE_IRQ, 21, 21 }
		}
	};
#endif

#if defined(ARCH_x86) || defined(ARCH_amd64)
static l4io_desc_device_t
	tpm_tis = { "TPM TIS", 1, { { L4IO_RESOURCE_MEM, 0xfed40000, 0xfed44fff } } };
#endif


/****************************
 ** Static-device database **
 ****************************/
static struct static_device
{
	int cfg_token;
	l4io_desc_device_t *device;
} devices[] =
{
#if defined(ARCH_arm)
	{ CFG_RV_EB_926,  &rv_eb_926_kbd },
	{ CFG_RV_EB_926,  &rv_eb_926_mou },
	{ CFG_RV_EB_MC,   &rv_eb_mc_kbd },
	{ CFG_RV_EB_MC,   &rv_eb_mc_mou },
	{ CFG_INTEGRATOR, &integrator_kbd },
	{ CFG_INTEGRATOR, &integrator_mou },
#endif

#if defined(ARCH_x86) || defined(ARCH_amd64)
	{ CFG_STD, &tpm_tis },  /* TPM 1.2 TIS area (see TPM 1.2 spec) */
#endif

	{ 0, 0 }
};


int io_static_cfg_init(l4io_info_t *info, int cfg_token)
{
	unsigned char *buffer = (unsigned char *) info->devices;
	struct static_device *static_dev;

	/* parse all static devices */
	for (static_dev = devices; static_dev->device; static_dev++) {
		/* skip devices not matching cfg token */
		if (static_dev->cfg_token != cfg_token) continue;

		l4io_desc_device_t *dev = static_dev->device;
		LOGd(DEBUG_RES, "device \"%s\" found @ %p", dev->id, dev);

		/* poke device into info page */
		unsigned dev_size = sizeof(*dev) + dev->num_resources * sizeof(*(dev->resources));
		Assert((buffer + dev_size) < (unsigned char *)(info + 1));
		memcpy(buffer, dev, dev_size);
		buffer += dev_size;

		int i;
		l4io_desc_resource_t *r;
		for (i = dev->num_resources, r = dev->resources; i; i--, r++) {
			/* announce MMIO resources to res module */
			if (r->type == L4IO_RESOURCE_MEM)
				callback_announce_mem_region(r->start, r->end - r->start + 1);

			LOGd(DEBUG_RES, "    %s: %08lx-%08lx",
			     r->type == L4IO_RESOURCE_IRQ ? " IRQ" :
			     r->type == L4IO_RESOURCE_MEM ? " MEM" :
			     r->type == L4IO_RESOURCE_PORT ? "PORT" : "????",
			     r->start, r->end);
		}
	}

	/* delimit with invalid device */
	((l4io_desc_device_t *)buffer)->num_resources = 0;

	return 0;
}
