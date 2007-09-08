#include "local.h"

#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/list.h>

/* will include $(CONTRIB)/drivers/pci/pci.h */
#include "pci.h"

DECLARE_INITVAR(dde26_pci);

/** PCI device descriptor */
typedef struct l4dde_pci_dev {
	struct list_head      next;         /**< chain info */
	struct ddekit_pci_dev *ddekit_dev;  /**< corresponding DDEKit descriptor */
	struct pci_dev        *linux_dev;   /**< Linux descriptor */
} l4dde_pci_dev_t;


/*******************************************************************************************
 ** PCI data                                                                              **
 *******************************************************************************************/
/** List of Linux-DDEKit PCIDev mappings */
static LIST_HEAD(pcidev_mappings);

/** PCI bus */
static struct pci_bus *pci_bus = NULL;

static int l4dde26_pci_read(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 *val);
static int l4dde26_pci_write(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 val);

/** PCI operations for our virtual PCI bus */
static struct pci_ops dde_pcibus_ops = {
	.read = l4dde26_pci_read,
	.write = l4dde26_pci_write,
};

/*******************************************************************************************
 ** Mapping DDELinux data to DDEKit data                                                   **
 *******************************************************************************************/

/** Add DDEKit device to our list of known devices.
 *
 * Allocate Linux device for a DDEKit PCI dev and fill in necessary values.
 */
static void _add_ddekit_device(struct ddekit_pci_dev *dev, int bus, int slot, int func)
{
	l4dde_pci_dev_t *pci_dev = NULL;
	int i;

	pci_dev = kmalloc(sizeof(l4dde_pci_dev_t), GFP_KERNEL);
	Assert(pci_dev);

	pci_dev->ddekit_dev = dev;
	pci_dev->linux_dev = pci_scan_single_device(pci_bus, PCI_DEVFN(slot, func));
	/* Since we know, that there must be a device with this function, we do not
	 * tolerate initialization failure. */
	Assert(pci_dev->linux_dev);

	DEBUG_MSG("Detected device: %lx:%lx",
	          pci_dev->linux_dev->vendor,
	          pci_dev->linux_dev->device);
	if (!pci_dev->linux_dev)  {
		kfree(pci_dev);
		return;
	}

	list_add_tail(&pci_dev->next, &pcidev_mappings);
}


/**
 * Map Linux pci_dev to a DDEKit PCI device.
 *
 * \param dev      Linux device to map.
 * \return ddekit_dev if available
 * \return NULL       if no corresponding DDEKit device exists.
 */
static struct ddekit_pci_dev *_linux_to_ddekit(struct pci_dev *dev)
{
	struct list_head *p, *h;
	struct ddekit_pci_dev *ret = NULL;

	h = &pcidev_mappings;
	list_for_each(p, h) {
		l4dde_pci_dev_t *d = list_entry(p, l4dde_pci_dev_t, next);
		if (d->linux_dev == dev) {
			ret = d->ddekit_dev;
			break;
		}
	}

	Assert(ret);
	return ret;
}


/*******************************************************************************************
 ** Read/write PCI config space. This is simply mapped to the DDEKit functions.           **
 *******************************************************************************************/
static int l4dde26_pci_read(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 *val)
{
	switch(size)
	{
		case 1:
			return ddekit_pci_readb(bus->number, PCI_SLOT(devfn),
			                        PCI_FUNC(devfn), where, (u8 *)val);
			break;
		case 2:
			return ddekit_pci_readw(bus->number, PCI_SLOT(devfn),
			                        PCI_FUNC(devfn), where, (u16 *)val);
			break;
		case 4:
			return ddekit_pci_readl(bus->number, PCI_SLOT(devfn),
			                        PCI_FUNC(devfn), where, val);
			break;
	}

	return 0;
}

static int l4dde26_pci_write(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 val)
{
	switch(size)
	{
		case 1:
			return ddekit_pci_writeb(bus->number, PCI_SLOT(devfn),
			                        PCI_FUNC(devfn), where, val);
			break;
		case 2:
			return ddekit_pci_writew(bus->number, PCI_SLOT(devfn),
			                        PCI_FUNC(devfn), where, val);
			break;
		case 4:
			return ddekit_pci_writel(bus->number, PCI_SLOT(devfn),
			                        PCI_FUNC(devfn), where, val);
			break;
	}

	return 0;
}

/**
 * pci_enable_wake - enable device to generate PME# when suspended
 *
 * Set the bits in the device's PM Capabilities to generate PME# when
 * the system is suspended.
 *
 * \param dev      PCI device to operate on
 * \param state    Current state of device.
 * \param enable   Flag to enable or disable generation
 *
 * \return -EIO    returned if device doesn't have PM Capabilities. 
 * \return -EINVAL is returned if device supports it, but can't generate wake events.
 * \return 0       if operation is successful.
 * 
 */
int pci_enable_wake(struct pci_dev *dev, pci_power_t state, int enable)
{
	WARN_UNIMPL;
	return 0;
}


/**
  * pci_enable_device - Initialize device before it's used by a driver.
  *
  * Initialize device before it's used by a driver. Ask low-level code
  * to enable I/O and memory. Wake up the device if it was suspended.
  * Beware, this function can fail.
  *
  * \param dev     PCI device to be initialized
  *
  */
int
pci_enable_device(struct pci_dev *dev)
{
	CHECK_INITVAR(dde26_pci);
	return ddekit_pci_enable_device(_linux_to_ddekit(dev));
}


/**
 * pci_disable_device - Disable PCI device after use
 *
 * Signal to the system that the PCI device is not in use by the system
 * anymore.  This only involves disabling PCI bus-mastering, if active.
 *
 * \param dev     PCI device to be disabled
 */
void pci_disable_device(struct pci_dev *dev)
{
	CHECK_INITVAR(dde26_pci);
	ddekit_pci_disable_device(_linux_to_ddekit(dev));
}


void pci_fixup_device(enum pci_fixup_pass pass, struct pci_dev *dev)
{
	WARN_UNIMPL;
}

void pci_set_master(struct pci_dev *dev)
{
	CHECK_INITVAR(dde26_pci);
	ddekit_pci_set_master(_linux_to_ddekit(dev));
}


int pci_create_sysfs_dev_files(struct pci_dev *pdev)
{
	return 0;
}

/*******************************************************************************************
 ** Initialization function                                                               **
 *******************************************************************************************/

extern int pci_init(void);
extern int pci_driver_init(void);

/** Initialize DDELinux PCI subsystem.
 */
void l4dde26_init_pci(void)
{
	struct ddekit_pci_dev *last = NULL;
	struct ddekit_pci_dev *next = NULL;
	int err;

	ddekit_pci_init();

	pci_bus = pci_create_bus(NULL, 0, &dde_pcibus_ops, NULL);

	/* Now that DDEKit has initialized its PCI devices, we can parse the bus again to add
	 * Linux PCI structs.
	 *
	 * We need to do this before calling any Linux PCI functions since these want
	 * to have the pci_devices list & co. populated.
	 */
	while (1) {
		int bus = DDEKIT_PCI_ANY_ID;
		int slot = DDEKIT_PCI_ANY_ID;
		int func = DDEKIT_PCI_ANY_ID;

		next = ddekit_pci_find_device(&bus, &slot, &func, last);
#if 0
		DEBUG_MSG("find_device: %x, %x, %x, %p", bus, slot, func, last);
#endif
		if (!next)
			break;

		last = next;
		_add_ddekit_device(next, bus, slot, func);
	}
	pci_bus_add_devices(pci_bus);

	INITIALIZE_INITVAR(dde26_pci);
}

subsys_initcall(l4dde26_init_pci);
