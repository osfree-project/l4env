/* $Id$ */
/*****************************************************************************/
/**
 * \file	dde_linux26/lib/src/pci.c
 *
 * \brief	PCI Support
 *
 * \author	Marek Menzer <mm19@os.inf.tu-dresden.de>
 *
 * Based on dde_linux version by Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
/** \ingroup mod_common
 * \defgroup mod_pci PCI Bus/Device Support
 *
 * This module emulates the PCI subsystem inside the Linux kernel.
 *
 * Most of the services of this module are wrappers to libio functions. The
 * remainder is simple glue code. The PCI module supports up to \c PCI_DEVICES
 * devices at one virtual PCI bus.
 *
 * Services are:
 *
 * -# exploration of bus/attached devices (find)
 * -# device setup (bus mastering, enable/disable)
 * -# Power Management related functions
 * -# PCI device related resources
 * -# Hotplugging (not supported yet)
 * -# PCI memory pools (consistent DMA mappings...)
 * -# configuration space access
 * -# functions for Linux backward compatibility
 *
 * Requirements: (additionally to \ref pg_req)
 * 
 * - initialized libio
 *
 * Configuration:
 *
 * - setup #PCI_DEVICES to configure number of supported PCI devices (default
 * 32)
 */
/*****************************************************************************/

/* L4 */
#include <l4/env/errno.h>
#include <l4/generic_io/libio.h>

#include <l4/dde_linux/dde.h>

/* Linux */
#include <linux/pci.h>
#include <linux/list.h>

/* local */
#include "internal.h"
#include "__config.h"
#include "__macros.h"

/*****************************************************************************/
/**
 * \name Module variables
 * @{ */
/*****************************************************************************/
/** PCI device structure array */
static struct pcidevs
{
  struct pci_dev linus;		/**< Linux device structure */
  l4io_pdev_t l4;		/**< l4io device handle */
} pcidevs[PCI_DEVICES];

/** list of all PCI devices (must be global) */
LIST_HEAD(pci_devices);

/** virtual PCI bus */
static struct pci_bus pcibus =
{
  name:		"LINUX DDE PCI BUS",
  number:	0,
};

/** initialization flag */
static int _initialized = 0;

/** @} */
/*****************************************************************************/
/** Get L4IO device handle for given device
 *
 * \param  linus	Linux device
 *
 * \return l4io handle for device or 0 if not found
 */
/*****************************************************************************/
static inline l4io_pdev_t __pci_get_handle(struct pci_dev *linus)
{
  return ((struct pcidevs*)linus)->l4;
}

/*****************************************************************************/
/** Convert IO's pci_dev to Linux' pci_dev struct
 *
 * \param  l4io		IO device
 * \param  linus	Linux device
 *
 * \krishna don't know about `struct resource' pointers?
 */
/*****************************************************************************/
static inline void __pci_io_to_linux(l4io_pci_dev_t *l4io,
				     struct pci_dev *linus)
{
  int i;

  memset(linus, 0, sizeof(struct pci_dev));

  linus->devfn = l4io->devfn;
  linus->vendor = l4io->vendor;
  linus->device = l4io->device;
  linus->subsystem_vendor = l4io->sub_vendor;
  linus->subsystem_device = l4io->sub_device;
  linus->class = l4io->class;

  linus->irq = l4io->irq;

  for (i = 0; i < DEVICE_COUNT_RESOURCE; i++)
    {
      linus->resource[i].name = linus->pretty_name;

      linus->resource[i].start = l4io->res[i].start;
      linus->resource[i].end = l4io->res[i].end;
      linus->resource[i].flags = l4io->res[i].flags;

      linus->resource[i].parent = NULL;
      linus->resource[i].sibling = NULL;
      linus->resource[i].child = NULL;
    }

  strcpy(&linus->pretty_name[0], &l4io->name[0]);
  strcpy(&linus->dev.bus_id[0], &l4io->slot_name[0]);
  linus->slot_name=&linus->dev.bus_id[0];

  linus->bus = &pcibus;

  list_add_tail(&linus->global_list, &pci_devices);
  list_add_tail(&linus->bus_list, &pcibus.devices);
}

/*****************************************************************************/
/** \name Exploration of bus/attached devices and drivers
 * @{ */
/*****************************************************************************/

/*****************************************************************************/
/** Check device against ID table
 *
 * \param ids		ID table
 * \param dev		target device
 *
 * \return matching device id
 *
 * Simple helper for device id matching check.
 */
/*****************************************************************************/
const struct pci_device_id *pci_match_device(const struct pci_device_id *ids,
					     const struct pci_dev *dev)
{
  while (ids->vendor || ids->subvendor || ids->class_mask)
    {
      if ((ids->vendor == PCI_ANY_ID || ids->vendor == dev->vendor) &&
	  (ids->device == PCI_ANY_ID || ids->device == dev->device) &&
	  (ids->subvendor == PCI_ANY_ID || ids->subvendor == dev->subsystem_vendor) &&
	  (ids->subdevice == PCI_ANY_ID || ids->subdevice == dev->subsystem_device) &&
	  !((ids->class ^ dev->class) & ids->class_mask))
	return ids;
      ids++;
    }
  return NULL;
}

/*****************************************************************************/
/** Check device - driver compatibility
 *
 * \param drv		device driver structure
 * \param dev		PCI device structure
 *
 * \return 1 if driver claims device; 0 otherwise
 */
/*****************************************************************************/
static int pci_announce_device(struct pci_driver *drv, struct pci_dev *dev)
{
  const struct pci_device_id *id;
  int ret = 0;

  if (drv->id_table)
    {
      id = pci_match_device(drv->id_table, dev);
      if (!id)
	{
	  ret = 0;
	  goto out;
	}
    }
  else
    id = NULL;

//      dev_probe_lock();
  if (drv->probe(dev, id) >= 0)
    {
      dev->driver = drv;
      ret = 1;
    }
//      dev_probe_unlock();
out:
  return ret;
}

/*****************************************************************************/
/** Get PCI driver of given device
 * \ingroup mod_pci
 *
 * \param dev		device to query
 *
 * \return appropriate pci_driver structure or NULL
 */
/*****************************************************************************/
struct pci_driver *pci_dev_driver(const struct pci_dev *dev)
{
  if (dev->driver)
    return dev->driver;
  return NULL;
}

/*****************************************************************************/
/** Register PCI driver
 * \ingroup mod_pci
 *
 * \param drv		device driver structure
 *
 * \return number of pci devices which were claimed by the driver
 *
 * pci_module_init(struct pci_driver *drv) is used to initalize drivers. Doing
 * it this way keeps the drivers away from for_each_dev() or pci_find_device().
 *
 * pci_register/unregister_driver() are helpers for these and have to be
 * implemented.
 */
/*****************************************************************************/
int pci_register_driver(struct pci_driver *drv)
{
  struct pci_dev *dev=NULL;
  int count = 0;

  pci_for_each_dev(dev)
  {
    if (!pci_dev_driver(dev))
      count += pci_announce_device(drv, dev);
  }
  return count;
}

/*****************************************************************************/
/** Unregister PCI driver
 * \ingroup mod_pci
 *
 * \param drv		device driver structure
 *
 * \sa pci_register_driver()
 */
/*****************************************************************************/
void pci_unregister_driver(struct pci_driver *drv)
{
  struct pci_dev *dev=NULL;

  pci_for_each_dev(dev)
    {
      if (dev->driver == drv)
	{
	  if (drv->remove)
	    drv->remove(dev);
	  dev->driver = NULL;
	}
    }
}

/*****************************************************************************/
/** Find PCI Device on vendor and device IDs
 * \ingroup mod_pci
 *
 * \param vendor	vendor id of desired device
 * \param device	device id of desired device
 * \param from		PCI device in list to start at (incremental calls)
 *
 * \return PCI device found or NULL on error
 */
/*****************************************************************************/
struct pci_dev *pci_find_device(unsigned int vendor, unsigned int device,
				const struct pci_dev *from)
{
  struct list_head *n = from ? from->global_list.next : pci_devices.next;

  while (n != &pci_devices)
    {
      struct pci_dev *dev = pci_dev_g(n);
      if ((vendor == PCI_ANY_ID || dev->vendor == vendor) &&
	  (device == PCI_ANY_ID || dev->device == device))
	return dev;
      n = n->next;
    }

  return NULL;
}

/*****************************************************************************/
/** Find PCI Device on vendor and device IDs in reverse order
 * \ingroup mod_pci
 *
 * \param vendor	vendor id of desired device
 * \param device	device id of desired device
 * \param from		PCI device in list to start at (incremental calls)
 *
 * \return PCI device found or NULL on error
 */
/*****************************************************************************/
struct pci_dev *pci_find_device_reverse(unsigned int vendor, unsigned int device,
				        const struct pci_dev *from)
{
  struct list_head *n = from ? from->global_list.prev : pci_devices.prev;

  while (n && (n != &pci_devices))
    {
      struct pci_dev *dev = pci_dev_g(n);
      if ((vendor == PCI_ANY_ID || dev->vendor == vendor) &&
	  (device == PCI_ANY_ID || dev->device == device))
	return dev;
      n = n->prev;
    }

  return NULL;
}

/*****************************************************************************/
/** Find PCI Device on vendor, subvendor, device and subdevice IDs
 * \ingroup mod_pci
 *
 * \param vendor	vendor id of desired device
 * \param device	device id of desired device
 * \param ss_vendor	subsystem vendor id of desired device
 * \param ss_device	subsystem device id of desired device
 * \param from		PCI device in list to start at (incremental calls)
 *
 * \return PCI device found or NULL on error
 */
/*****************************************************************************/
struct pci_dev * pci_find_subsys(unsigned int vendor, unsigned int device,
				 unsigned int ss_vendor, unsigned int ss_device,
				 const struct pci_dev *from)
{
  struct list_head *n = from ? from->global_list.next : pci_devices.next;

  while (n != &pci_devices)
    {
      struct pci_dev *dev = pci_dev_g(n);
      if ((vendor == PCI_ANY_ID || dev->vendor == vendor) &&
	  (device == PCI_ANY_ID || dev->device == device) &&
	  (ss_vendor == PCI_ANY_ID || dev->subsystem_vendor == ss_vendor) &&
	  (ss_device == PCI_ANY_ID || dev->subsystem_device == ss_device))
	return dev;
      n = n->next;
    }

  return NULL;
}

/*****************************************************************************/
/** Find PCI Device on Slot
 * \ingroup mod_pci
 *
 * \param bus		target PCI bus
 * \param devfn		device and function number
 *
 * \return PCI device found or NULL on error
 */
/*****************************************************************************/
struct pci_dev *pci_find_slot(unsigned int bus, unsigned int devfn)
{
  struct pci_dev *dev=NULL;

  pci_for_each_dev(dev)
    {
      if (dev->bus->number == bus && dev->devfn == devfn)
	return dev;
    }

  return NULL;
}

/*****************************************************************************/
/** Find PCI Device on Class
 * \ingroup mod_pci
 *
 * \param class		class id of desired device
 * \param from		PCI device in list to start at (incremental calls)
 *
 * \return PCI device found or NULL on error
 */
/*****************************************************************************/
struct pci_dev *pci_find_class(unsigned int class, const struct pci_dev *from)
{
  struct list_head *n = from ? from->global_list.next : pci_devices.next;

  while (n != &pci_devices)
    {
      struct pci_dev *dev = pci_dev_g(n);
      if (dev->class == class)
	return dev;
      n = n->next;
    }

  return NULL;
}

/* emulate locally like in drivers/pci.c */
int pci_find_capability (struct pci_dev *dev, int cap)
{
  return 0;
}

/** @} */
/*****************************************************************************/
/** \name Device setup (bus mastering, enable/disable)
 * @{ */
/*****************************************************************************/

/*****************************************************************************/
/** Enable PCI Device
 * \ingroup mod_pci
 *
 * \param dev		target PCI device
 *
 * \return 0 on success; error code otherwise
 */
/*****************************************************************************/
int pci_enable_device(struct pci_dev *dev)
{
  int err;
  l4io_pdev_t pdev;

  if (!(pdev=__pci_get_handle(dev)))
    {
#if DEBUG_PCI
      LOG_Error("device %p not found", dev);
#endif
      return PCIBIOS_DEVICE_NOT_FOUND;
    }

  err = l4io_pci_enable(pdev);

#if DEBUG_PCI
  if (err)
    LOG_Error("enabling PCI device (%d)", err);
#endif

  return err;
}

/*****************************************************************************/
/** Disable PCI Device
 * \ingroup mod_pci
 *
 * \param dev		target PCI device
 */
/*****************************************************************************/
void pci_disable_device(struct pci_dev *dev)
{
  int err;
  l4io_pdev_t pdev;

  if (!(pdev=__pci_get_handle(dev)))
    {
#if DEBUG_PCI
      LOG_Error("device %p not found", dev);
#endif
      return;
    }

  err = l4io_pci_disable(pdev);

#if DEBUG_PCI
  if (err)
    LOG_Error("disabling PCI device (%d)", err);
#endif
}

/**
 *	pci_release_region - Release a PCI bar
 *	@pdev: PCI device whose resources were previously reserved by pci_request_region
 *	@bar: BAR to release
 *
 *	Releases the PCI I/O and memory resources previously reserved by a
 *	successful call to pci_request_region.  Call this function only
 *	after all use of the PCI regions has ceased.
 */
void pci_release_region(struct pci_dev *pdev, int bar)
{
	if (pci_resource_len(pdev, bar) == 0)
		return;
	if (pci_resource_flags(pdev, bar) & IORESOURCE_IO)
		release_region(pci_resource_start(pdev, bar),
				pci_resource_len(pdev, bar));
	else if (pci_resource_flags(pdev, bar) & IORESOURCE_MEM)
		release_mem_region(pci_resource_start(pdev, bar),
				pci_resource_len(pdev, bar));
}

/**
 *	pci_request_region - Reserved PCI I/O and memory resource
 *	@pdev: PCI device whose resources are to be reserved
 *	@bar: BAR to be reserved
 *	@res_name: Name to be associated with resource.
 *
 *	Mark the PCI region associated with PCI device @pdev BR @bar as
 *	being reserved by owner @res_name.  Do not access any
 *	address inside the PCI regions unless this call returns
 *	successfully.
 *
 *	Returns 0 on success, or %EBUSY on error.  A warning
 *	message is also printed on failure.
 */
int pci_request_region(struct pci_dev *pdev, int bar, char *res_name)
{
	if (pci_resource_len(pdev, bar) == 0)
		return 0;
		
	if (pci_resource_flags(pdev, bar) & IORESOURCE_IO) {
		if (!request_region(pci_resource_start(pdev, bar),
			    pci_resource_len(pdev, bar), res_name))
			goto err_out;
	}
	else if (pci_resource_flags(pdev, bar) & IORESOURCE_MEM) {
		if (!request_mem_region(pci_resource_start(pdev, bar),
				        pci_resource_len(pdev, bar), res_name))
			goto err_out;
	}
	
	return 0;

err_out:
	printk (KERN_WARNING "PCI: Unable to reserve %s region #%d:%lx@%lx for device %s\n",
		pci_resource_flags(pdev, bar) & IORESOURCE_IO ? "I/O" : "mem",
		bar + 1, /* PCI BAR # */
		pci_resource_len(pdev, bar), pci_resource_start(pdev, bar),
		pci_name(pdev));
	return -EBUSY;
}


/**
 *	pci_release_regions - Release reserved PCI I/O and memory resources
 *	@pdev: PCI device whose resources were previously reserved by pci_request_regions
 *
 *	Releases all PCI I/O and memory resources previously reserved by a
 *	successful call to pci_request_regions.  Call this function only
 *	after all use of the PCI regions has ceased.
 */

void pci_release_regions(struct pci_dev *pdev)
{
	int i;
	
	for (i = 0; i < 6; i++)
		pci_release_region(pdev, i);
}

/**
 *	pci_request_regions - Reserved PCI I/O and memory resources
 *	@pdev: PCI device whose resources are to be reserved
 *	@res_name: Name to be associated with resource.
 *
 *	Mark all PCI regions associated with PCI device @pdev as
 *	being reserved by owner @res_name.  Do not access any
 *	address inside the PCI regions unless this call returns
 *	successfully.
 *
 *	Returns 0 on success, or %EBUSY on error.  A warning
 *	message is also printed on failure.
 */
int pci_request_regions(struct pci_dev *pdev, char *res_name)
{
	int i;
	
	for (i = 0; i < 6; i++)
		if(pci_request_region(pdev, i, res_name))
			goto err_out;
	return 0;

err_out:
	printk (KERN_WARNING "PCI: Unable to reserve %s region #%d:%lx@%lx for device %s\n",
		pci_resource_flags(pdev, i) & IORESOURCE_IO ? "I/O" : "mem",
		i + 1, /* PCI BAR # */
		pci_resource_len(pdev, i), pci_resource_start(pdev, i),
		pci_name(pdev));
	while(--i >= 0)
		pci_release_region(pdev, i);
		
	return -EBUSY;
}

#ifndef HAVE_ARCH_PCI_MWI
/* This can be overridden by arch code. */
u8 pci_cache_line_size = L1_CACHE_BYTES >> 2;

/**
 * pci_generic_prep_mwi - helper function for pci_set_mwi
 * @dev: the PCI device for which MWI is enabled
 *
 * Helper function for generic implementation of pcibios_prep_mwi
 * function.  Originally copied from drivers/net/acenic.c.
 * Copyright 1998-2001 by Jes Sorensen, <jes@trained-monkey.org>.
 *
 * RETURNS: An appropriate -ERRNO error value on error, or zero for success.
 */
static int
pci_generic_prep_mwi(struct pci_dev *dev)
{
	u8 cacheline_size;

	if (!pci_cache_line_size)
		return -EINVAL;		/* The system doesn't support MWI. */

	/* Validate current setting: the PCI_CACHE_LINE_SIZE must be
	   equal to or multiple of the right value. */
	pci_read_config_byte(dev, PCI_CACHE_LINE_SIZE, &cacheline_size);
	if (cacheline_size >= pci_cache_line_size &&
	    (cacheline_size % pci_cache_line_size) == 0)
		return 0;

	/* Write the correct value. */
	pci_write_config_byte(dev, PCI_CACHE_LINE_SIZE, pci_cache_line_size);
	/* Read it back. */
	pci_read_config_byte(dev, PCI_CACHE_LINE_SIZE, &cacheline_size);
	if (cacheline_size == pci_cache_line_size)
		return 0;

	printk(KERN_WARNING "PCI: cache line size of %d is not supported "
	       "by device %s\n", pci_cache_line_size << 2, pci_name(dev));

	return -EINVAL;
}
#endif /* !HAVE_ARCH_PCI_MWI */

/**
 * pci_set_mwi - enables memory-write-invalidate PCI transaction
 * @dev: the PCI device for which MWI is enabled
 *
 * Enables the Memory-Write-Invalidate transaction in %PCI_COMMAND,
 * and then calls @pcibios_set_mwi to do the needed arch specific
 * operations or a generic mwi-prep function.
 *
 * RETURNS: An appropriate -ERRNO error value on error, or zero for success.
 */
int
pci_set_mwi(struct pci_dev *dev)
{
	int rc;
	u16 cmd;

#ifdef HAVE_ARCH_PCI_MWI
	rc = pcibios_prep_mwi(dev);
#else
	rc = pci_generic_prep_mwi(dev);
#endif

	if (rc)
		return rc;

	pci_read_config_word(dev, PCI_COMMAND, &cmd);
	if (! (cmd & PCI_COMMAND_INVALIDATE)) {
		LOGd(DEBUG_MSG, "PCI: Enabling Mem-Wr-Inval for device %s", pci_name(dev));
		cmd |= PCI_COMMAND_INVALIDATE;
		pci_write_config_word(dev, PCI_COMMAND, cmd);
	}
	
	return 0;
}

/**
 * pci_clear_mwi - disables Memory-Write-Invalidate for device dev
 * @dev: the PCI device to disable
 *
 * Disables PCI Memory-Write-Invalidate transaction on the device
 */
void
pci_clear_mwi(struct pci_dev *dev)
{
	u16 cmd;

	pci_read_config_word(dev, PCI_COMMAND, &cmd);
	if (cmd & PCI_COMMAND_INVALIDATE) {
		cmd &= ~PCI_COMMAND_INVALIDATE;
		pci_write_config_word(dev, PCI_COMMAND, cmd);
	}
}

/*****************************************************************************/
/** Set Busmastering for PCI Device
 * \ingroup mod_pci
 *
 * \param dev		target PCI device
 *
 * \todo Who panics if it fails?
 */
/*****************************************************************************/
void pci_set_master(struct pci_dev *dev)
{
  l4io_pdev_t pdev;

  if (!(pdev=__pci_get_handle(dev)))
    {
#if DEBUG_PCI
      PANIC("device %p not found", dev);
#endif
      return;
    }

  l4io_pci_set_master(pdev);
}

/** @} */
/*****************************************************************************/
/** \name Power Management related functions
 * @{ */
/*****************************************************************************/

/*****************************************************************************/
/** Set PM State for PCI Device
 * \ingroup mod_pci
 *
 * \param dev		target PCI device
 * \param state		PM state
 *
 * \return old PM state
 */
/*****************************************************************************/
int pci_set_power_state(struct pci_dev *dev, int state)
{
  l4io_pdev_t pdev;

  if (!(pdev=__pci_get_handle(dev)))
    {
#if DEBUG_PCI
      LOG_Error("device %p not found", dev);
#endif
      return PCIBIOS_DEVICE_NOT_FOUND;
    }

  return l4io_pci_set_pm(pdev, state);
}

/** @} */
/*****************************************************************************/
/** \name PCI device related resources
 *
 * \todo implementation
 * @{ */
/*****************************************************************************/

/** @} */
/*****************************************************************************/
/** \name Hotplugging (not supported yet)
 * @{ */
/*****************************************************************************/

/** @} */
/*****************************************************************************/
/** \name PCI DMA allocation functions
 * @{ */
/*****************************************************************************/

/*****************************************************************************/
/** Allocation of PCI consistent DMA Memory
 * \ingroup mod_pci
 *
 * \todo Is this really a PCI issue?!
 */
/*****************************************************************************/
void *pci_alloc_consistent(struct pci_dev *hwdev,
			   size_t size, dma_addr_t * dma_handle)
{
  void *ret;
  int gfp = GFP_ATOMIC;

/* NO ISA now
   if (hwdev == NULL || hwdev->dma_mask != 0xffffffff)
   gfp |= GFP_DMA;
*/
  ret = (void *) __get_free_pages(gfp, get_order(size));

  if (ret != NULL)
    {
      memset(ret, 0, size);
      *dma_handle = virt_to_bus(ret);
    }
  LOGd(DEBUG_PALLOC, "PCI requested pages");
  return ret;
}

/*****************************************************************************/
/** Deallocation of PCI consistent DMA Memory
 * \ingroup mod_pci
 *
 * \todo Is this really a PCI issue?!
 */
/*****************************************************************************/
void pci_free_consistent(struct pci_dev *hwdev, size_t size,
			 void *vaddr, dma_addr_t dma_handle)
{
  free_pages((unsigned long) vaddr, get_order(size));
  LOGd(DEBUG_PALLOC, "PCI released pages");
}

/* XXX think about this */
int pci_set_dma_mask(struct pci_dev *dev, u64 mask)
{
  dev->dma_mask = mask;

  return 0;
}

/** @} */
/*****************************************************************************/
/** \name Configuration space access
 * @{ */
/*****************************************************************************/

/*****************************************************************************/
/** PCI Configuration Space access - read byte
 * \ingroup mod_pci
 *
 * \param bus	PCI bus
 * \param devfn device and function index
 * \param where	configuration register
 *
 * \retval val	register contents
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int pci_bus_read_config_byte(struct pci_bus *bus, unsigned int devfn, int where, l4_uint8_t * val)
{
  struct pci_dev *dev;
  
  dev = pci_find_slot(0,devfn); // We only support one bus!
  if (dev) return pci_read_config_byte(dev, where, val);
   else {
#if DEBUG_PCI
      LOG_Error("device %p not found", dev);
#endif
      return PCIBIOS_DEVICE_NOT_FOUND;
   }
}

/*****************************************************************************/
/** PCI Configuration Space access - read word
 * \ingroup mod_pci
 *
 * \param bus	PCI bus
 * \param devfn device and function index
 * \param where	configuration register
 *
 * \retval val	register contents
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int pci_bus_read_config_word(struct pci_bus *bus, unsigned int devfn, int where, l4_uint16_t * val)
{
  struct pci_dev *dev;
  
  dev = pci_find_slot(0,devfn); // We only support one bus!
  if (dev) return pci_read_config_word(dev, where, val);
   else {
#if DEBUG_PCI
      LOG_Error("device %p not found", dev);
#endif
      return PCIBIOS_DEVICE_NOT_FOUND;
   }
}

/*****************************************************************************/
/** PCI Configuration Space access - read dword
 * \ingroup mod_pci
 *
 * \param bus	PCI bus
 * \param devfn device and function index
 * \param where	configuration register
 *
 * \retval val	register contents
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int pci_bus_read_config_dword(struct pci_bus *bus, unsigned int devfn, int where, l4_uint32_t * val)
{
  struct pci_dev *dev;
  
  dev = pci_find_slot(0,devfn); // We only support one bus!
  if (dev) return pci_read_config_dword(dev, where, val);
   else {
#if DEBUG_PCI
      LOG_Error("device %p not found", dev);
#endif
      return PCIBIOS_DEVICE_NOT_FOUND;
   }
}

/*****************************************************************************/
/** PCI Configuration Space access - write byte
 * \ingroup mod_pci
 *
 * \param bus	PCI bus
 * \param devfn device and function index
 * \param where	configuration register
 * \param val   new value
 *
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int pci_bus_write_config_byte(struct pci_bus *bus, unsigned int devfn, int where, l4_uint8_t val)
{
  struct pci_dev *dev;
  
  dev = pci_find_slot(0,devfn); // We only support one bus!
  if (dev) return pci_write_config_byte(dev, where, val);
   else {
#if DEBUG_PCI
      LOG_Error("device %p not found", dev);
#endif
      return PCIBIOS_DEVICE_NOT_FOUND;
   }
}

/*****************************************************************************/
/** PCI Configuration Space access - write word
 * \ingroup mod_pci
 *
 * \param bus	PCI bus
 * \param devfn device and function index
 * \param where	configuration register
 * \param val   new value
 *
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int pci_bus_write_config_word(struct pci_bus *bus, unsigned int devfn, int where, l4_uint16_t val)
{
  struct pci_dev *dev;
  
  dev = pci_find_slot(0,devfn); // We only support one bus!
  if (dev) return pci_write_config_word(dev, where, val);
   else {
#if DEBUG_PCI
      LOG_Error("device %p not found", dev);
#endif
      return PCIBIOS_DEVICE_NOT_FOUND;
   }
}

/*****************************************************************************/
/** PCI Configuration Space access - write dword
 * \ingroup mod_pci
 *
 * \param bus	PCI bus
 * \param devfn device and function index
 * \param where	configuration register
 * \param val   new value
 *
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int pci_bus_write_config_dword(struct pci_bus *bus, unsigned int devfn, int where, l4_uint32_t val)
{
  struct pci_dev *dev;
  
  dev = pci_find_slot(0,devfn); // We only support one bus!
  if (dev) return pci_write_config_dword(dev, where, val);
   else {
#if DEBUG_PCI
      LOG_Error("device %p not found", dev);
#endif
      return PCIBIOS_DEVICE_NOT_FOUND;
   }
}

/*****************************************************************************/
/** PCI Configuration Space access - read byte
 * \ingroup mod_pci
 *
 * \param dev	PCI device
 * \param pos	configuration register
 *
 * \retval val	register contents
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int pci_read_config_byte(struct pci_dev *dev, int pos, l4_uint8_t * val)
{
  int err;
  l4io_pdev_t pdev;

  if (!(pdev=__pci_get_handle(dev)))
    {
#if DEBUG_PCI
      LOG_Error("device %p not found", dev);
#endif
      return PCIBIOS_DEVICE_NOT_FOUND;
    }

  err = l4io_pci_readb_cfg(pdev, pos, val);

#if DEBUG_PCI
  if (err)
    LOG_Error("reading PCI config register (%d)", err);
#endif

  return err ? -EIO : 0;
}

/*****************************************************************************/
/** PCI Configuration Space access - read word
 * \ingroup mod_pci
 *
 * \param dev	PCI device
 * \param pos	configuration register
 *
 * \retval val	register contents
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int pci_read_config_word(struct pci_dev *dev, int pos, l4_uint16_t * val)
{
  int err;
  l4io_pdev_t pdev;

  if (!(pdev=__pci_get_handle(dev)))
    {
#if DEBUG_PCI
      LOG_Error("device %p not found", dev);
#endif
      return PCIBIOS_DEVICE_NOT_FOUND;
    }

  err = l4io_pci_readw_cfg(pdev, pos, val);

#if DEBUG_PCI
  if (err)
    LOG_Error("reading PCI config register (%d)", err);
#endif

  return err ? -EIO : 0;
}

/*****************************************************************************/
/** PCI Configuration Space access - read double word
 * \ingroup mod_pci
 *
 * \param dev	PCI device
 * \param pos	configuration register
 *
 * \retval val	register contents
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int pci_read_config_dword(struct pci_dev *dev, int pos, l4_uint32_t * val)
{
  int err;
  l4io_pdev_t pdev;

  if (!(pdev=__pci_get_handle(dev)))
    {
#if DEBUG_PCI
      LOG_Error("device %p not found", dev);
#endif
      return PCIBIOS_DEVICE_NOT_FOUND;
    }

  err = l4io_pci_readl_cfg(pdev, pos, val);

#if DEBUG_PCI
  if (err)
    LOG_Error("reading PCI config register (%d)", err);
#endif

  return err ? -EIO : 0;
}

/*****************************************************************************/
/** PCI Configuration Space access - write byte
 * \ingroup mod_pci
 *
 * \param dev	PCI device
 * \param pos	configuration register
 * \param val	new value
 *
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int pci_write_config_byte(struct pci_dev *dev, int pos, l4_uint8_t val)
{
  int err;
  l4io_pdev_t pdev;

  if (!(pdev=__pci_get_handle(dev)))
    {
#if DEBUG_PCI
      LOG_Error("device %p not found", dev);
#endif
      return PCIBIOS_DEVICE_NOT_FOUND;
    }

  err = l4io_pci_writeb_cfg(pdev, pos, val);

#if DEBUG_PCI
  if (err)
    LOG_Error("writing PCI config register (%d)", err);
#endif

  return err ? -EIO : 0;
}

/*****************************************************************************/
/** PCI Configuration Space access - write word
 * \ingroup mod_pci
 *
 * \param dev	PCI device
 * \param pos	configuration register
 * \param val	new value
 *
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int pci_write_config_word(struct pci_dev *dev, int pos, l4_uint16_t val)
{
  int err;
  l4io_pdev_t pdev;

  if (!(pdev=__pci_get_handle(dev)))
    {
#if DEBUG_PCI
      LOG_Error("device %p not found", dev);
#endif
      return PCIBIOS_DEVICE_NOT_FOUND;
    }

  err = l4io_pci_writew_cfg(pdev, pos, val);

#if DEBUG_PCI
  if (err)
    LOG_Error("writing PCI config register (%d)", err);
#endif

  return err ? -EIO : 0;
}

/******************************************************************************/
/** PCI Configuration Space access - write double word
 * \ingroup mod_pci
 *
 * \param dev	PCI device
 * \param pos	configuration register
 * \param val	new value
 *
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int pci_write_config_dword(struct pci_dev *dev, int pos, l4_uint32_t val)
{
  int err;
  l4io_pdev_t pdev;

  if (!(pdev=__pci_get_handle(dev)))
    {
#if DEBUG_PCI
      LOG_Error("device %p not found", dev);
#endif
      return PCIBIOS_DEVICE_NOT_FOUND;
    }

  err = l4io_pci_writel_cfg(pdev, pos, val);

#if DEBUG_PCI
  if (err)
    LOG_Error("writing PCI config register (%d)", err);
#endif

  return err ? -EIO : 0;
}

/** @} */
/*****************************************************************************/
/** \name Functions for Linux backward compatibility
 * This is from drivers/pci/compat.c
 * @{ */
/*****************************************************************************/

#if 0
/** Find ... (old interface)
 * \ingroup mod_pci */
int pcibios_find_class(unsigned int class, unsigned short index,
		       unsigned char *bus, unsigned char *devfn)
{
  const struct pci_dev *dev = NULL;
  int cnt = 0;

  while ((dev = pci_find_class(class, dev)))
    if (index == cnt++)
      {
	*bus = dev->bus->number;
	*devfn = dev->devfn;
	return PCIBIOS_SUCCESSFUL;
      }

  return PCIBIOS_DEVICE_NOT_FOUND;
}
#endif

/** Find ... (old interface)
 * \ingroup mod_pci */
int pcibios_find_device(unsigned short vendor, unsigned short device,
			unsigned short index, unsigned char *bus,
			unsigned char *devfn)
{
  const struct pci_dev *dev = NULL;
  int cnt = 0;

  while ((dev = pci_find_device(vendor, device, dev)))
    if (index == cnt++)
      {
	*bus = dev->bus->number;
	*devfn = dev->devfn;
	return PCIBIOS_SUCCESSFUL;
      }

  return PCIBIOS_DEVICE_NOT_FOUND;
}

/** Configuration space access function creation (old interface)
 * \ingroup mod_pci */
#define PCI_OP(rw,size,type)							\
int pcibios_##rw##_config_##size (unsigned char bus, unsigned char dev_fn,	\
				  unsigned char where, unsigned type val)	\
{										\
	struct pci_dev *dev = pci_find_slot(bus, dev_fn);			\
	if (!dev) return PCIBIOS_DEVICE_NOT_FOUND;				\
	return pci_##rw##_config_##size(dev, where, val);			\
}

PCI_OP(read, byte, char *)
PCI_OP(read, word, short *)
PCI_OP(read, dword, int *)
PCI_OP(write, byte, char)
PCI_OP(write, word, short)
PCI_OP(write, dword, int)

/** @} */
/*****************************************************************************/
/** Initalize PCI module
 * \ingroup mod_pci
 *
 * \return 0 on success; negative error code otherwise
 *
 * Scan all PCI devices and establish virtual PCI bus.
 *
 * \todo consider pcibus no as parameter
 */
/*****************************************************************************/
int l4dde_pci_init()
{
  struct pci_dev *dev = NULL;
  int err, i;
  l4io_pdev_t start = 0;
  l4io_pci_dev_t new;

  if (_initialized)
    return -L4_ESKIPPED;

  /* setup virtual bus */
  INIT_LIST_HEAD(&pcibus.devices);

  /* setup devices */
  for (;;)
    {
      if (dev && !(start=__pci_get_handle((struct pci_dev*)dev)))
	{
#if DEBUG_PCI
	  PANIC("device %p not found", dev);
#endif
	  return -L4_EUNKNOWN;
	}

      err = l4io_pci_find_device(PCI_ANY_ID, PCI_ANY_ID, start, &new);

      if (err)
	{
	  if (err == -L4_ENOTFOUND) break;
#if DEBUG_PCI
	  LOG_Error("locate PCI device (%d)", err);
#endif
	  return err;
	}

      /* look for free slot */
      for (i=0; i < PCI_DEVICES; i++)
	if (!pcidevs[i].l4)
	  break;
      if (i == PCI_DEVICES)
	{
#if DEBUG_PCI
	  PANIC("all PCI device slots occupied");
#endif
	  return -L4_EUNKNOWN;
	}

      /* save information */
      __pci_io_to_linux(&new, &pcidevs[i].linus);
      pcidevs[i].l4 = new.handle;

      dev = &pcidevs[i].linus;
    }

  ++_initialized;
  return 0;
}
