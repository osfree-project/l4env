#include <stdio.h>
#include <l4/env/errno.h>

#include "pci.h"
#include "init.h"

#define MAX_DRIVERS 16

struct pci_driver
{
  struct pci_device_id *dev;
  int (*probe)(unsigned int bus, unsigned int devfn,
	       struct pci_device_id *dev, con_accel_t *accel);
};

static struct pci_driver pci_drv[MAX_DRIVERS];

void
pci_register(struct pci_device_id *tbl, 
	     int(*probe)(unsigned int bus, unsigned int devfn,
			 struct pci_device_id *dev, con_accel_t *accel))
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
  unsigned int devfn, l, bus, buses;
  unsigned char hdr_type = 0;
  unsigned short vendor, device, svid, sid;
  unsigned char class, subclass;
  int ret;

  l4io_pdev_t start = 0;
  l4io_pci_dev_t new;

  if (!use_l4io)
    {
      buses = 1;
      for (bus = 0; bus < buses; ++bus)
	{
	  for (devfn = 0; devfn < 0xff; ++devfn)
	    {
	      struct pci_driver *drv;
	      struct pci_device_id *dev;

	      if (PCI_FUNC (devfn) == 0)
		pcibios_read_config_byte (bus, devfn, PCI_HEADER_TYPE, &hdr_type);
	      else if (!(hdr_type & 0x80))	/* not a multi-function device */
		continue;

	      pcibios_read_config_dword (bus, devfn, PCI_VENDOR_ID, &l);
	      /* some broken boards return 0 if a slot is empty: */
	      if (l == 0xffffffff || l == 0x00000000)
		{
		  hdr_type = 0;
		  continue;
		}

	      vendor = l & 0xffff;
	      device = (l >> 16) & 0xffff;

	      /* check for pci-pci bridge devices!! - more buses when found */
	      pcibios_read_config_byte (bus, devfn, PCI_CLASS_CODE, &class);
	      pcibios_read_config_byte (bus, devfn, PCI_SUBCLASS_CODE, &subclass);
	      if (class == 0x06 && subclass == 0x04)
		buses++;

	      /* only scan for graphics cards */
	      if (class != PCI_BASE_CLASS_DISPLAY)
		continue;

	      pcibios_read_config_word (bus, devfn, PCI_SUBSYSTEM_VENDOR_ID, &svid);
	      pcibios_read_config_word (bus, devfn, PCI_SUBSYSTEM_ID, &sid);

	      for (drv = pci_drv; drv->dev; drv++)
		{
		  for (dev = drv->dev; dev->vendor; dev++)
		    {
		      if (dev->vendor != vendor)
			continue;
		      if (dev->device != 0)
			if (dev->device != device)
			  continue;
		      if (dev->svid != 0)
			if ((dev->svid != svid) || (dev->sid != sid))
			  continue;

		      /* found appropriate driver ... */
		      if ((ret = drv->probe(bus, devfn, dev, accel)) != 0)
			/* ... not really */
			continue;

		      return 0;
		    }
		}
	    }
	}
    }
  else /* use l4io */
    {
      for (;;)
	{
	  struct pci_driver *drv;
	  struct pci_device_id *dev;

	  /* only scan for graphics cards */
	  ret = l4io_pci_find_class(PCI_CLASS_DISPLAY_VGA<<8, start, &new);
	  if (ret) return ret;

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
		    if ((dev->svid != new.sub_vendor) || (dev->sid != new.sub_device))
		      continue;

		  /* found appropriate driver ... */
		  if ((ret = drv->probe(new.handle>>8, new.handle&0xff, dev, accel)) != 0)
		    /* ... YES */
		    continue;

		  return 0;
		}
	    }

	  /* go on from here */
	  start = new.handle;
	}
    }

  return -L4_ENOTFOUND;
}
