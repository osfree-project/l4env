/**
 * inftpm.c
 *
 * A device driver for a Infineon TPM (SLD9630TT)
 *
 * Copyright (C) 2004 Bernhard Kauer <kauer@os.inf.tu-dresden.de>
 *
 * version: 0.2
 * - simple polling -> no irq until know
 * - write/read semantic (but write blocks for a response)
 * - no abort defined
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include "inftpm.h"
#include "inftpm.h"
#include "inftpm_init.h"
#include "inftpm_io.h"
#include "inftpm_tl.h"

// "Intel ICH5 (82801EB) LPC"
#define	PCI_DEVICE_ID_INTEL_ICH5LPC	0x24D0
// "Intel 82801DBM (ICH4-M) LPC "
#define PCI_DEVICE_ID_INTEL_ICH4LPC	0x24CC

enum
{
	ICH4LPC,
	ICH5LPC,
};


//vendor layer
struct tpm_vl_header {
	u8 version;
	u8 channel;
	u32 size;
} __attribute__ ((packed));

#define TPM_VL_VERSION       0x01
#define TPM_VL_CHANNEL_TPM   0x0B


// XXX what is the right value here?
#define MAX_BUFFER_SIZE  2048

struct tpm_data {
	struct pci_dev *pci_dev;
	u16    base;
	u16    config_addr;
	int    unused;
	struct {
		struct tpm_vl_header header;
		u8 buf[MAX_BUFFER_SIZE];
	} data;
};

static struct tpm_data tpm_data;

/**
 * Init the tpm, so that it works correctly.
 */
static int __init
tpm_init(struct pci_dev *pci_dev, u16 config_addr, u16 base)
{
	int res;
	debug("> config %04x base %04x", config_addr, base);
	if ((res=tpm_pci_init(pci_dev,base)))
		return res;
	if ((res=tpm_dev_init(config_addr,base)))
		return res;       
	if ((res=tpm_io_init(base)))
	    return res;
	debug("< = %d",res);
	return 0;
}

static int __init
tpm_pci_probe(struct pci_dev *pci_dev, const struct pci_device_id *pci_id)
{
	debug(">");
	memset(&tpm_data, 0, sizeof(tpm_data));

	tpm_data.pci_dev = pci_dev;
	switch (pci_id->driver_data)
		{
		case ICH4LPC:
			tpm_data.base = ICH4_IO_BASE_ADDR;
			tpm_data.config_addr = ICH4_CONFIG_ADDR;
			break;
		case ICH5LPC:
			tpm_data.base = ICH5_IO_BASE_ADDR;
			tpm_data.config_addr = ICH5_CONFIG_ADDR;
			break;
		default:
			error("unknown PCI device");
			return -ENODEV;
		}

	if (tpm_init(pci_dev, tpm_data.config_addr, tpm_data.base)){
		error("tpm_init() failed!");
		// free resources
		tpm_pci_exit(tpm_data.pci_dev,tpm_data.base);
		return -ENODEV;
	}

	tpm_data.unused = 1;
	debug("<");
	return 0;
}

static void __devexit
tpm_pci_remove(struct pci_dev *pci_dev)
{
	debug("<>");
	tpm_data.unused = 0;
	tpm_pci_exit(tpm_data.pci_dev,tpm_data.base);
}

static int
tpm_fops_open(struct inode *inode, struct file *file)
{
	debug(">");
	if (!tpm_data.unused)
		return -EBUSY;
	tpm_data.unused = 0;
	debug("<");
	return 0;
}

static int
tpm_fops_release(struct inode *inode, struct file *file)
{
	tpm_data.unused = 1;
	// XXX perhaps we should clear the buffer
	// tpm_data.data.header.size = 0;
	debug("<");
	return 0;
}

static ssize_t
tpm_fops_read(struct file *file, char *buf, size_t count, loff_t *pos)
{
	debug("> count %d pos %d",count,pos);
	if (count == 0)
		return 0;

	if (count > tpm_data.data.header.size)
		count = tpm_data.data.header.size;
	else if (count > tpm_data.data.header.size)
		error("read not enough bytes");

	// reset the read data
	tpm_data.data.header.size = 0;

	if (copy_to_user(buf, tpm_data.data.buf, count))
		return -EFAULT;
	
	debug("< = %d",count);
	return count;
}

static ssize_t
tpm_fops_write(struct file *file, const char *buf, size_t count, loff_t *pos)
{
	int res;
	debug("> count %d pos %d",count,pos);

	if (count == 0)
		return 0;
	if (tpm_data.data.header.size != 0)
	{
		error("write with read data");
		tpm_data.data.header.size = 0;
	}
	if (count > sizeof(tpm_data.data.buf))
		count = sizeof(tpm_data.data.buf);

	if (copy_from_user(tpm_data.data.buf, buf, count))
		return -EFAULT;

	tpm_data.data.header.version = TPM_VL_VERSION;
	tpm_data.data.header.channel = TPM_VL_CHANNEL_TPM;

	tpm_data.data.header.size = htonl(count);
	res = tpm_tl_transmit(tpm_data.base, 
			      (u8 *)&tpm_data.data, 
			      count+sizeof(tpm_data.data.header),
			      sizeof(tpm_data.data));
	tpm_data.data.header.size = ntohl(tpm_data.data.header.size);

	if (res < 0)
	{
		tpm_data.data.header.size = 0;
		debug("< != %d",res);
		return res;
	}
	else
	{
		if (tpm_data.data.header.version != TPM_VL_VERSION)
		{
			tpm_data.data.header.size = 0;
			error("vl version mismatch");
			return -EIO;
		}
		if (tpm_data.data.header.channel != TPM_VL_CHANNEL_TPM)
		{
			error("vl channel mismatch");
			tpm_data.data.header.size = 0;
			return -EIO;
		}
		debug("< = %d",count);
		return count;
	}
}

static int
tpm_fops_ioctl(struct inode *inode, struct file *file,
		unsigned int cmd, unsigned long arg)
{
	int res=0;
	debug("<> cmd %u arg %lu",cmd,arg);
	switch (cmd) {
	case TPM_IOCTL_RESET:
		error("reset TPM");
		res=tpm_io_reset(tpm_data.base);
		break;
	case TPM_IOCTL_STATUS:
		res=tpm_io_get_status(tpm_data.base);
		break;
	case TPM_IOCTL_DEV_INIT:
		error("dev reinit");
		res=tpm_dev_reinit(tpm_data.config_addr, tpm_data.base);
		break;
	default:
		debug("unknown cmd %u",cmd);
		res=-ENODEV;
	}
	return res;
}

struct file_operations tpm_fops = {
	owner:		THIS_MODULE,
	open:		tpm_fops_open,
	read:		tpm_fops_read,
	write:		tpm_fops_write,
	ioctl:		tpm_fops_ioctl,
	release:	tpm_fops_release
};




static struct pci_device_id tpm_pci_tbl [] __initdata = {
	{PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_ICH5LPC,
	 PCI_ANY_ID, PCI_ANY_ID, 0, 0, ICH5LPC},
	{PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_ICH4LPC,
	 PCI_ANY_ID, PCI_ANY_ID, 0, 0, ICH4LPC},
	{0,}
};

MODULE_DEVICE_TABLE (pci, tpm_pci_tbl);



static struct pci_driver tpm_pci_driver = {
        name:           MODULE_NAME,
        id_table:       tpm_pci_tbl,
        probe:          tpm_pci_probe,
        remove:         __devexit_p(tpm_pci_remove),
};

static struct miscdevice tpm_dev = {
	TPM_MINOR_NR,
	MODULE_NAME,
	&tpm_fops
};

static int __init
tpm_mod_init(void)
{
	if (pci_register_driver(&tpm_pci_driver) || !tpm_data.unused) {
		pci_unregister_driver(&tpm_pci_driver);
		return -ENODEV;
        }

	if (misc_register(&tpm_dev)) {
		error("unable to misc_register minor %d", TPM_MINOR_NR);
		pci_unregister_driver(&tpm_pci_driver);
		return -ENODEV;
	}

	return 0;
}

static void __exit
tpm_mod_exit(void)
{
	misc_deregister(&tpm_dev);
	pci_unregister_driver(&tpm_pci_driver);
}

module_init(tpm_mod_init);
module_exit(tpm_mod_exit);

MODULE_AUTHOR("Bernhard Kauer <kauer@os.inf.tu-dresden.de>");
MODULE_DESCRIPTION("Driver for Infineon TPM");
MODULE_LICENSE("GPL");

