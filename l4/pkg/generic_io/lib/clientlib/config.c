/*****************************************************************************/
/**
 * \file   generic_io/lib/clientlib/config.c
 * \brief  Functions for static configuration handling.
 *
 * \date   July 2007
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <string.h>

#include <l4/generic_io/libio.h>
#include "internal.h"

/*****************************************************************************/
/* Lookup device in configuration page.                                      */
/*****************************************************************************/
l4io_desc_device_t *l4io_desc_lookup_device(const char *name,
                                            l4io_info_t *iopage)
{
  l4io_desc_device_t *dev = l4io_desc_first_device(iopage);

  while (dev)
    {
      if (!strncmp(name, (const char *)dev->id, L4IO_DEVICE_NAME_LEN))
        return dev;

      dev = l4io_desc_next_device(dev);
    }

  return NULL;
}

/*****************************************************************************/
/* Lookup resource in device structure.                                      */
/*****************************************************************************/
int l4io_desc_lookup_resource(l4io_desc_device_t *d, unsigned long type, int i)
{
  for (; i < d->num_resources; i++)
    {
      if (d->resources[i].type == type)
	return i;
    }
  return -1;
}

/*****************************************************************************/
/**
 * \brief  Get memory region by name.
 *
 * \param  name   Name of the region
 * \index  index  Index in the resource
 *
 * \return virtual address; 0 on error.
 */
/*****************************************************************************/
l4_addr_t l4io_request_mem_region_name(const char *name, const int index)
{
  l4io_desc_device_t *l4dev;
  int i;

  if (!l4io_info_page())
    return 0;

  if ((l4dev = l4io_desc_lookup_device(name, l4io_info_page())) == NULL)
    return 0;

  if ((i = l4io_desc_lookup_resource(l4dev, L4IO_RESOURCE_MEM, 0)) == -1)
    return 0;

  return l4io_request_mem_region(l4dev->resources[i].start,
                                 l4dev->resources[i].end
                                   - l4dev->resources[i].start + 1,
                                 L4IO_MEM_NONCACHED);
}

/*****************************************************************************/
/**
 * \brief  Get IRQ number by name
 *
 * \param  name   Name of the IRQ
 * \index  index  Index in the resource
 *
 * \return IRQ number; -1 on error
 */
/*****************************************************************************/
int l4io_get_irq_name(const char *name, const int index)
{
  l4io_desc_device_t *l4dev;
  int i;

  if (!l4io_info_page())
    return -1;

  if ((l4dev = l4io_desc_lookup_device(name, l4io_info_page())) == NULL)
    return -1;

  if ((i = l4io_desc_lookup_resource(l4dev, L4IO_RESOURCE_IRQ, 0)) == -1)
    return -1;

  return l4dev->resources[i].start;
}
