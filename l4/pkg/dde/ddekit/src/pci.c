#include <l4/dde/ddekit/pci.h>
#include <l4/dde/ddekit/printf.h>
#include <l4/dde/ddekit/panic.h>
#include <l4/dde/ddekit/memory.h>
#include <l4/dde/ddekit/assert.h>

#include <l4/generic_io/types.h>
#include <l4/generic_io/libio.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>

#include "config.h"

#define dbg_this 0

#define MAX_PCI_DEVS 32

/** PCI descriptor */
typedef struct ddekit_pci_dev {
	int bus;                      /**< bus ID */
	int slot;                     /**< slot ID */
	int func;                     /**< function */
	l4io_pci_dev_t l4dev;         /**< L4IO handle */
	struct ddekit_pci_dev *next;  /**< chaining info */
} ddekit_pci_dev_t;

static ddekit_pci_dev_t ddekit_pci_bus[MAX_PCI_DEVS];

static inline int invalid_device(ddekit_pci_dev_t *d)
{
	return d->slot == -1;
}


/** Initialize DDEKit PCI module.
 *
 * \ingroup DDEKit_pci
 *
 * This function builds a list of devices by querying L4IO.
 */
void ddekit_pci_init(void)
{
	l4io_pdev_t start = 0;

	int slots_found = 0;
	int i;

	/* Init device list */
	for (i = 0; i < MAX_PCI_DEVS; i++)
		ddekit_pci_bus[i].slot = -1;

	while (1) {
		l4io_pci_dev_t l4dev;
		int err;

		/* search next device */
		err = l4io_pci_find_device(~0, ~0, start, &l4dev);
		if (err) {
			if (err == -L4_ENOTFOUND) {
				LOGd(dbg_this, "no more pci devices");
			} else {
				LOGd(dbg_this, "error: scanning pci devices: %s (%d)", l4env_errstr(err), err);
			}
			break;
		}

		/* next search start from here */
		start = l4dev.handle;

		/* print info */

		/* Pretend all our devices are chained to exactly one bus. */
		ddekit_pci_bus[slots_found].bus   = 0; /*l4dev.bus;*/
		ddekit_pci_bus[slots_found].slot  = slots_found;
		ddekit_pci_bus[slots_found].func  = 0;
		ddekit_pci_bus[slots_found].l4dev = l4dev;

		LOGd(dbg_this, "pcib_identify: found device (%x, %x, %x), mapped to (%x, %x, %x)",
		     l4dev.bus, l4dev.devfn >> 3, l4dev.devfn & 0x07,
		     ddekit_pci_bus[slots_found].bus,
		     ddekit_pci_bus[slots_found].slot,
		     ddekit_pci_bus[slots_found].func);

		++slots_found;
	}
}


/**
 * Get PCI device (bus, slot, func) for handle.
 *
 * \ingroup DDEKit_pci
 */
int ddekit_pci_get_device(int nr, int *bus, int *slot, int *func)
{
	ddekit_pci_dev_t *dev;

	LOGd(dbg_this, "searching for dev #%d", nr);

	if (nr >= 0 && nr < MAX_PCI_DEVS && !invalid_device(&ddekit_pci_bus[nr])) {
		dev = &ddekit_pci_bus[nr];
		*bus = dev->bus;
		*slot = dev->slot;
		*func = dev->func;
		return 0;
	}

	return -1;
}


ddekit_pci_dev_t *ddekit_pci_find_device(int *bus, int *slot, int *func,
                                         ddekit_pci_dev_t *start)
{
	Assert(bus);
	Assert(slot);
	Assert(func);

	LOGd(dbg_this, "start %p (slot %d)", start, start ? start->slot : -1);
	int idx = start ? start->slot + 1 : 0;
	
	for ( ; idx < MAX_PCI_DEVS; ++idx) {
	    ddekit_pci_dev_t *dev = &ddekit_pci_bus[idx];
		if (!invalid_device(dev) &&
			(*bus == DDEKIT_PCI_ANY_ID || dev->bus == *bus) &&
		    (*slot == DDEKIT_PCI_ANY_ID || dev->slot == *slot) &&
		    (*func == DDEKIT_PCI_ANY_ID || dev->func == *func)) {
			*bus = dev->bus;
			*slot = dev->slot;
			*func = dev->func;
			return dev;
		}
	}

	return NULL;
}

int ddekit_pci_read(int bus, int slot, int func, int pos, int len, ddekit_uint32_t *val)
{
	switch(len)
	{
		case 1:
			return ddekit_pci_readb(bus, slot, func, pos, (ddekit_uint8_t *)val);
		case 2:
			return ddekit_pci_readw(bus, slot, func, pos, (ddekit_uint16_t *)val);
		case 4:
			return ddekit_pci_readl(bus, slot, func, pos, val);
	}

	return -1;
}

int ddekit_pci_write(int bus, int slot, int func, int pos, int len, ddekit_uint32_t val)
{
	switch(len)
	{
		case 1:
			return ddekit_pci_writeb(bus, slot, func, pos, val);
		case 2:
			return ddekit_pci_writew(bus, slot, func, pos, val);
		case 4:
			return ddekit_pci_writel(bus, slot, func, pos, val);
	}

	return -1;
}


/** Find PCI device for a given (bus, slot, func) tuple.
*
* \ingroup DDEKit_pci
 *
 * This function is only used internally by the pci_{read,write}{b,l,w} functions.
 * It was once called pci_find_device, but this caused confusion with the "real"
 * find_device() functionality provided by L4IO and the Linux PCI subsystem.
 */
static inline ddekit_pci_dev_t *ddekit_pci_find_device_fixed(int bus, int slot, int func)
{
	return &ddekit_pci_bus[slot];
}


int ddekit_pci_readb (int bus, int slot, int func, int pos, ddekit_uint8_t  *val)
{
	ddekit_pci_dev_t *dev = ddekit_pci_find_device_fixed(bus, slot, func);
	if (dev)
		return l4io_pci_readb_cfg(dev->l4dev.handle, pos, val);
	else
		return -1;
}


int ddekit_pci_readw (int bus, int slot, int func, int pos, ddekit_uint16_t *val)
{
	ddekit_pci_dev_t *dev = ddekit_pci_find_device_fixed(bus, slot, func);
	if (dev)
		return l4io_pci_readw_cfg(dev->l4dev.handle, pos, val);
	else
		return -1;
}


int ddekit_pci_readl (int bus, int slot, int func, int pos, ddekit_uint32_t *val)
{
	ddekit_pci_dev_t *dev = ddekit_pci_find_device_fixed(bus, slot, func);
	if (dev)
		return l4io_pci_readl_cfg(dev->l4dev.handle, pos, val);
	else
		return -1;
}


int ddekit_pci_writeb(int bus, int slot, int func, int pos, ddekit_uint8_t   val)
{
	ddekit_pci_dev_t *dev = ddekit_pci_find_device_fixed(bus, slot, func);
	if (dev)
		return l4io_pci_writeb_cfg(dev->l4dev.handle, pos, val);
	else
		return -1;
}


int ddekit_pci_writew(int bus, int slot, int func, int pos, ddekit_uint16_t  val)
{
	ddekit_pci_dev_t *dev = ddekit_pci_find_device_fixed(bus, slot, func);
	if (dev)
		return l4io_pci_writew_cfg(dev->l4dev.handle, pos, val);
	else
		return -1;
}


int ddekit_pci_writel(int bus, int slot, int func, int pos, ddekit_uint32_t  val)
{
	ddekit_pci_dev_t *dev = ddekit_pci_find_device_fixed(bus, slot, func);
	if (dev)
		return l4io_pci_writel_cfg(dev->l4dev.handle, pos, val);
	else
		return -1;
}

int ddekit_pci_enable_device(struct ddekit_pci_dev *dev)
{
	return l4io_pci_enable(dev->l4dev.handle);
}

int ddekit_pci_disable_device(struct ddekit_pci_dev *dev)
{
	return l4io_pci_disable(dev->l4dev.handle);
}

/********************************************************************************
 ** Accessor functions for the various kinds of information that are available **
 ** from struct l4io_pci_dev (which is not handed through to the DDEs, because **
 ** it still contains L4 types).                                               **
 ********************************************************************************/

/**
 * Get PCI device vendor ID.
 *
 * \param dev     device
 *
 * \ingroup DDEKit_pci
 */
unsigned short ddekit_pci_get_vendor(struct ddekit_pci_dev *dev)
{
	return dev->l4dev.vendor;
}


/**
 * Get PCI device ID.
 *
 * \param dev     device
 *
 * \ingroup DDEKit_pci
 */
unsigned short ddekit_pci_get_device_id(struct ddekit_pci_dev *dev)
{
	return dev->l4dev.device;
}


/**
 * Get PCI device subvendor ID.
 *
 * \param dev     device
 *
 * \ingroup DDEKit_pci
 */
unsigned short ddekit_pci_get_sub_vendor(struct ddekit_pci_dev *dev)
{
	return dev->l4dev.sub_vendor;
}


/**
 * Get PCI device subdevice ID.
 *
 * \param dev     device
 *
 * \ingroup DDEKit_pci
 */
unsigned short ddekit_pci_get_sub_device(struct ddekit_pci_dev *dev)
{
	return dev->l4dev.sub_device;
}


/**
 * Get PCI device class ID.
 *
 * \param dev     device
 *
 * \ingroup DDEKit_pci
 */
unsigned       ddekit_pci_get_dev_class(struct ddekit_pci_dev *dev)
{
	return dev->l4dev.dev_class;
}


/**
 * Get PCI device IRQ number.
 *
 * \param dev     device
 *
 * \ingroup DDEKit_pci
 */
unsigned long  ddekit_pci_get_irq(struct ddekit_pci_dev *dev)
{
	return dev->l4dev.irq;
}


/**
 * Get PCI device name.
 *
 * \param dev     device
 *
 * \ingroup DDEKit_pci
 */
char *ddekit_pci_get_name(struct ddekit_pci_dev *dev)
{
	return dev->l4dev.name;
}


/**
 * Get PCI device slot name.
 *
 * \param dev     device
 *
 * \ingroup DDEKit_pci
 */
char *ddekit_pci_get_slot_name(struct ddekit_pci_dev *dev)
{
	return dev->l4dev.slot_name;
}


/**
 * Get PCI device resource at index.
 *
 * \param dev     device
 * \param idx     resource index
 *
 * \ingroup DDEKit_pci
 */
ddekit_pci_res_t *ddekit_pci_get_resource(struct ddekit_pci_dev *dev, unsigned int idx)
{
	if (idx > L4IO_PCIDEV_RES)
		return NULL;

	return (ddekit_pci_res_t *)(&(dev->l4dev.res[idx]));
}


/**
 * Set PCI bus mastering for device.
 *
 * \ingroup DDEKit_pci
 */
void ddekit_pci_set_master(struct ddekit_pci_dev *dev)
{
	l4io_pci_set_master(dev->l4dev.handle);
}

int ddekit_pci_irq_enable(int bus, int slot, int func, int pin, int *irq)
{
	return 0;
}
