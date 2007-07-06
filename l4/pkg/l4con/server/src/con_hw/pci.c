/*!
 * \file	pci.c
 * \brief	Handling of PCI devices
 *
 * \date	07/2002
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include <stdio.h>
#include <l4/env/errno.h>

#include "pci.h"
#include "init.h"

#define MAX_DRIVERS 16

struct pci_driver
{
  const struct pci_device_id *dev;
  int (*probe)(unsigned int bus, unsigned int devfn,
	       const struct pci_device_id *dev, con_accel_t *accel);
};

static struct pci_driver pci_drv[MAX_DRIVERS];

void
pci_register(const struct pci_device_id *tbl, 
	     int(*probe)(unsigned int bus, unsigned int devfn,
			 const struct pci_device_id *dev, con_accel_t *accel))
{
  struct pci_driver *drv;

  for (drv = pci_drv; drv->dev; drv++)
    {
      if (drv >= pci_drv + MAX_DRIVERS-1)
	{
	  printf("Too many drivers registered, increase MAX_DRIVERS!!\n");
	  return;
	}
    }

  drv->dev   = tbl;
  drv->probe = probe;
}

int
pci_probe(con_accel_t *accel)
{
  int ret;

  l4io_pdev_t start = 0;
  l4io_pci_dev_t new;

  for (;;)
  {
      struct pci_driver *drv;
      const struct pci_device_id *dev;

      /* only scan for graphics cards */
      ret = l4io_pci_find_class(PCI_CLASS_DISPLAY_VGA<<8, start, &new);
      if (ret)
          return ret;

      for (drv = pci_drv; drv->dev; drv++)
      {
          for (dev = drv->dev; dev->vendor; dev++)
          {
              if (dev->vendor != new.vendor)
                  continue;
              if (dev->device != 0)
                  if (dev->device != new.device)
                      continue;
              if (dev->svid != 0)
                  if ((dev->svid != new.sub_vendor) || 
                          (dev->sid != new.sub_device))
                      continue;

              /* found appropriate driver ... */
              if ((ret = drv->probe(new.handle>>8, 
                              new.handle&0xff, dev, accel)) != 0)
                  /* ... YES */
                  continue;

              return 0;
          }
      }

      /* go on from here */
      start = new.handle;
    }

  return -L4_ENOTFOUND;
}

void
pci_resource(unsigned int bus, unsigned int devfn, 
	     int num, l4_addr_t *addr, l4_size_t *size)
{
  unsigned l, sz, reg;

  switch (num)
    {
    case 0:  reg = PCI_BASE_ADDRESS_0; break;
    case 1:  reg = PCI_BASE_ADDRESS_1; break;
    case 2:  reg = PCI_BASE_ADDRESS_2; break;
    default: return;
    }

  PCIBIOS_READ_CONFIG_DWORD (bus, devfn, reg, &l);
  PCIBIOS_WRITE_CONFIG_DWORD(bus, devfn, reg, ~0);
  PCIBIOS_READ_CONFIG_DWORD (bus, devfn, reg, &sz);
  PCIBIOS_WRITE_CONFIG_DWORD(bus, devfn, reg, l);
  if ((l & PCI_BASE_ADDRESS_SPACE) == PCI_BASE_ADDRESS_SPACE_MEMORY)
    {
      *addr = l & PCI_BASE_ADDRESS_MEM_MASK;
      sz   &= PCI_BASE_ADDRESS_MEM_MASK;
      *size = sz & ~(sz - 1);
    }
  else
    {
      *addr = l & PCI_BASE_ADDRESS_IO_MASK;
      sz   &= PCI_BASE_ADDRESS_IO_MASK & 0xffff;
      *size = sz & ~(sz - 1);
    }
}
