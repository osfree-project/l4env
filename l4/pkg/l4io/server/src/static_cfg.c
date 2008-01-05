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
#include <stdio.h>

/* local */
#include "io.h"
#include "res.h"
#include "static_cfg.h"
#include "__config.h"
#include "__macros.h"


static struct {
  const char *name;
  l4io_desc_device_t **devs;
  int num;
} registered_devs[MAX_NUM_REGISTERED_DEVS];

static int num_devs;

void register_device_group_fn(const char *name, l4io_desc_device_t **devs, int num)
{
  if (num_devs == MAX_NUM_REGISTERED_DEVS)
    {
      printf("WARNING: Number for storable devices descriptors exceeded. Skipping %s\n", name);
      return;
    }

  registered_devs[num_devs].name = name;
  registered_devs[num_devs].devs = devs;
  registered_devs[num_devs].num = num;

  num_devs++;
}

int io_static_cfg_init(l4io_info_t *info, const char *requested_platform)
{
  unsigned char *buffer = (unsigned char *) info->devices;
  int i, j;
  int found = 0;

  for (i = 0; i < num_devs; ++i)
    {
      if (strcmp(requested_platform, registered_devs[i].name))
	continue;

      found = 1;
      printf("Using platform configuration '%s'\n", registered_devs[i].name);

      for (j = 0; j < registered_devs[i].num; ++j)
	{
           l4io_desc_device_t *dev = registered_devs[i].devs[j];
           LOGd(DEBUG_RES, "device \"%s\" found @ %p", dev->id, dev);

           /* poke device into info page */
           unsigned dev_size = sizeof(*dev) + dev->num_resources * sizeof(*(dev->resources));
           Assert((buffer + dev_size) < (unsigned char *)(info + 1));
           memcpy(buffer, dev, dev_size);
           buffer += dev_size;

           int k;
           l4io_desc_resource_t *r;
           for (k = dev->num_resources, r = dev->resources; k; k--, r++) {
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

      /* Go on checking, maybe we're finding the same name again... */
    }

  if (!found)
    printf("WARNING: No platform configuration for '%s' found.\n", requested_platform);

  /* terminate with invalid device */
  ((l4io_desc_device_t *)buffer)->num_resources = 0;

  return 0;
}
