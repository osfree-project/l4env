/*
 * tpm.c
 *
 * Device driver for TCPA TPM (trusted platform module).
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * Note, this chip is not interrupt driven (only polling) and can have
 * very long timeouts (minutes!). Hence the unusual calls to schedule_timeout.
 *
 * - Added official (misc) device number and removed multiple chip support
 * - Added suspend/resume support
 */
#include <linux/module.h>
#include <linux/version.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/sound.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <linux/smp_lock.h>
#include <linux/miscdevice.h>
#include <linux/tpm.h>

#include <asm/io.h>
#include <asm/dma.h>
#include <asm/uaccess.h>
#include <asm/hardirq.h>

/* #define	TPM_DEBUG  define this for verbose logging*/
#define ERROR_CHECKING

#define	TPM_MINOR			224	/* officially assigned */
#define	TPM_MODULE_NAME			"tpm"

#define	TPM_SCHEDULE_TIMEOUT		10	/* in jiffies (about 100 ms) */
#define	TPM_TIMEOUT			((2*60*HZ)/TPM_SCHEDULE_TIMEOUT)

#define	TCPA_ATMEL_BASE			0x400

#define	PCI_DEVICE_ID_INTEL_ICH2LPC	0x2440
#define	PCI_DEVICE_ID_INTEL_ICH3LPCM	0x248C
#define	PCI_DEVICE_ID_INTEL_ICH4LPC	0x24C0
#define	PCI_DEVICE_ID_INTEL_ICH4LPCM	0x24CC

/* PCI configuration addresses */
#define	PCI_GEN_PMCON_1			0xA0
#define	PCI_GEN1_DEC			0xE4
#define	PCI_LPC_EN			0xE6
#define	PCI_GEN2_DEC			0xEC

/* TPM addresses */
#define	TPM_ADDR			0x4E
#define	TPM_DATA			0x4F

/* write status bits */
#define	STATUS_BIT_ABORT		0x01
#define	STATUS_BIT_LASTBYTE		0x04

/* read status bits */
#define	STATUS_BIT_BUSY			0x01
#define	STATUS_BIT_DATA_AVAIL		0x02
#define	STATUS_BIT_REWRITE		0x04

enum {
        ICH2LPC = 0,
        ICH3LPCM,
        ICH4LPC,
        ICH4LPCM
};

static char *chip_names[] = {
	"Intel ICH2 LPC",
	"Intel ICH3 LPC-M",
	"Intel ICH4 LPC",
	"Intel ICH4 LPC-M"
};

struct tpm_chip {
	struct pci_dev	*pci_dev;	/* PCI device stuff */
	u16		pci_id;
	int		type;		/* chipset type */
	u16		base;
	u8		version[4];	/* version id */
	u8		vendor[4+1];	/* vendor id */
	u8		data[2048];	/* request/reply buffer */
	int		initialized;	/* initialized flag */
	int		open;		/* exclusive open flag */
};

static struct tpm_chip	tpm_chip;


static int __init
tpm_init(struct pci_dev *pci_dev, struct tpm_chip *chip)
{
	u32 flags, lpcenable, tmp;

	/* init ICH (enable LPC) */
	pci_read_config_dword(pci_dev, PCI_GEN1_DEC, &lpcenable);
	lpcenable |= 0x20000000;
	pci_write_config_dword(pci_dev, PCI_GEN1_DEC, lpcenable);
	if (chip->type == ICH3LPCM || chip->type == ICH4LPCM) {
		pci_read_config_dword(pci_dev, PCI_GEN1_DEC, &lpcenable);
		if ((lpcenable & 0x20000000) == 0) {
			printk(KERN_ERR TPM_MODULE_NAME
				": cannot enable LPC\n");
			return -ENODEV;
		}
	}

	/* initialize TPM */
	pci_read_config_dword(pci_dev, PCI_GEN2_DEC, &tmp);
	if (chip->type == ICH2LPC || chip->type == ICH4LPC)
		tmp = (tmp & 0xFFFF0000) | (chip->base & 0xFFF0);
	else if (chip->type == ICH3LPCM || chip->type == ICH4LPCM)
		tmp = (tmp & 0xFFFF0000) | (chip->base & 0xFFF0) | 0x00000001;
	pci_write_config_dword(pci_dev, PCI_GEN2_DEC, tmp);

	if (chip->type == ICH3LPCM || chip->type == ICH4LPCM) { /* power mgt */
		pci_read_config_dword(pci_dev, PCI_GEN_PMCON_1, &tmp);
		tmp |= 0x00000004; /* enable CLKRUN */
		pci_write_config_dword(pci_dev, PCI_GEN_PMCON_1, tmp);
	}
#if 0
	save_flags(flags); cli();
#else
	local_irq_save(flags);
#endif
        outb(0x0D, TPM_ADDR);		/* unlock 4F */
        outb(0x55, TPM_DATA);
        outb(0x0A, TPM_ADDR);		/* int disable */
        outb(0x00, TPM_DATA);
        outb(0x08, TPM_ADDR);		/* base addr lo */
        outb(chip->base & 0xFF, TPM_DATA);
        outb(0x09, TPM_ADDR);		/* base addr hi */
        outb((chip->base & 0xFF00) >> 8, TPM_DATA);
        outb(0x0D, TPM_ADDR);          /* lock 4F */
        outb(0xAA, TPM_DATA);
#if 0
	restore_flags(flags);
#else
	local_irq_restore(flags);
#endif

	return 0;
}

static int __init
tpm_probe(struct pci_dev *pci_dev, const struct pci_device_id *pci_id)
{
	memset(&tpm_chip, 0, sizeof(tpm_chip));

	if (pci_enable_device(pci_dev))
		return -EIO;

	tpm_chip.pci_dev = pci_dev;
	tpm_chip.pci_id = pci_id->device;
	tpm_chip.type = pci_id->driver_data;
	tpm_chip.base = TCPA_ATMEL_BASE;

	if (tpm_init(pci_dev, &tpm_chip))
		return -ENODEV;

	/* query chip for its version number */
	outb(0x00, TPM_ADDR);
	if ((tpm_chip.version[0] = inb(TPM_DATA)) != 0xFF) {
		outb(0x01, TPM_ADDR);
		tpm_chip.version[1] = inb(TPM_DATA) ;
		outb(0x02, TPM_ADDR);
		tpm_chip.version[2] = inb(TPM_DATA);
		outb(0x03, TPM_ADDR);
		tpm_chip.version[3] = inb(TPM_DATA);
	} else {
		printk(KERN_INFO TPM_MODULE_NAME ": version query failed\n");
		return -ENODEV;
	}

	/* query chip for its vendor id */
	outb(0x04, TPM_ADDR);
	if ((tpm_chip.vendor[0] = inb(TPM_DATA)) != 0xFF) {
		outb(0x05, TPM_ADDR);
		tpm_chip.vendor[1] = inb(TPM_DATA);
		outb(0x06, TPM_ADDR);
		tpm_chip.vendor[2] = inb(TPM_DATA);
		outb(0x07, TPM_ADDR);
		tpm_chip.vendor[3] = inb(TPM_DATA);
		tpm_chip.vendor[4] = '\0';
	} else {
		printk(KERN_INFO TPM_MODULE_NAME ": vendor query failed\n");
		return -ENODEV;
	}

	tpm_chip.initialized = 1;

	printk(KERN_INFO TPM_MODULE_NAME
		" found at 0x%x (%s), version %d.%d.%d.%d, vendor '%s'\n",
		tpm_chip.base, chip_names[pci_id->driver_data],
		tpm_chip.version[0], tpm_chip.version[1],
		tpm_chip.version[2], tpm_chip.version[3], tpm_chip.vendor);

	return 0;
}

static void __devexit
tpm_remove(struct pci_dev *pci_dev)
{
	/* nothing */
}

static int
tpm_open(struct inode *inode, struct file *file)
{
	if (!tpm_chip.initialized)
		return -ENODEV;
	if (tpm_chip.open)
		return -EBUSY;
	tpm_chip.open = 1;
#if 0
	MOD_INC_USE_COUNT;
#endif
	return 0;
}

static int
tpm_release(struct inode *inode, struct file *file)
{
	tpm_chip.open = 0;
#if 0
	MOD_DEC_USE_COUNT;
#endif
	return 0;
}

static u32
decode_u32(char *buf)
{
	u32 val = buf[0];
	val = (val << 8) | (u8) buf[1];
	val = (val << 8) | (u8) buf[2];
	val = (val << 8) | (u8) buf[3];
	return val;
}

static u8
crc8(char *buf, size_t count)
{
	u8 crc;
	int i;

	for (crc = 0xFF; count > 0; buf++, count--) {
		for (i = 0; i < 8; i++) {
			if (crc & 0x80)
				crc = (crc << 1) ^ 0x31;
			else
				crc <<= 1;
		}
		crc ^= *buf;
	}
	return crc;
}

static int
tpm_recv(struct tpm_chip *chip, u8 *buf, size_t count)
{
	int i, status=0;
	u8 *hdr = buf;
	u32 size;


	/*
	 * Wait for data available to go high and busy to go away.
	 * This can be a really long wait, up to 2 minutes, for some
	 * commands. Since the chip doesn't do interrupts, we have to
	 * poll.
	 */
	for (i = 0; i < TPM_TIMEOUT; i++) {
		status = inb(chip->base+1);
		if ((status & STATUS_BIT_BUSY) == 0)
			break;
		current->state = TASK_UNINTERRUPTIBLE;
		schedule_timeout(TPM_SCHEDULE_TIMEOUT);
	}
	if (i == TPM_TIMEOUT) {
		printk(KERN_ERR TPM_MODULE_NAME ": timeout 1\n");
		return -EIO;
	}

	/* wait while the chip is thinking about the command */
	for (i = 0; i < TPM_TIMEOUT; i++) {
		status = inb(chip->base+1);
		status &= (STATUS_BIT_DATA_AVAIL|STATUS_BIT_BUSY);
		if (status == STATUS_BIT_DATA_AVAIL)
			break;
		current->state = TASK_UNINTERRUPTIBLE;
		schedule_timeout(TPM_SCHEDULE_TIMEOUT);
	}
	if (i == TPM_TIMEOUT) {
		printk(KERN_ERR TPM_MODULE_NAME ": timeout 2\n");
		return -EIO;
	}
	if (status != STATUS_BIT_DATA_AVAIL) {
		printk(KERN_ERR TPM_MODULE_NAME
			": timeout waiting for data available\n");
		return -EIO;
	}

	/* start reading header */
	if (count < 6)
		return -EIO;
	for (i = 0; i < 6; i++) {
		status = inb(chip->base+1);
		if ((status & STATUS_BIT_DATA_AVAIL) == 0) {
			printk(KERN_ERR TPM_MODULE_NAME
				": error reading header\n");
			return -EIO;
		}
		*buf++ = inb(chip->base);
	}

	/* size of the data received */
	size = decode_u32(hdr + 2);
	if (count < size)
		return -EIO;

	/* read all the data available */
	for ( ; i < size; i++) {
		status = inb(chip->base+1);
		if ((status & STATUS_BIT_DATA_AVAIL) == 0) {
			printk(KERN_ERR TPM_MODULE_NAME
				": error reading data\n");
			return -EIO;
		}
		*buf++ = inb(chip->base);
	}

	/* make sure data available is gone */
	status = inb(chip->base+1);
	if (status & STATUS_BIT_DATA_AVAIL) {
		printk(KERN_ERR TPM_MODULE_NAME
			": data available is stuck\n");
		return -EIO;
	}

	/* send and abort */
	outb(STATUS_BIT_ABORT, chip->base + 1);

#ifdef ERROR_CHECKING
	/* verify CRC, if TPM has included it */
	if (*hdr != 0) {
		u8 oldcrc = *hdr, crc;
		*hdr = 0;
		if ((crc = crc8(hdr, size)) == 0)
			crc = 0xFF;
		if (crc != oldcrc) {
			printk(KERN_ERR TPM_MODULE_NAME
				": bad CRC on read\n");
			return -EIO;
		}
	}
#endif /* ERROR_CHECKING */

	return size;
}

static ssize_t
tpm_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	int len;

#ifdef TPM_DEBUG
	printk("tpm_read\n");
#endif
	if (count == 0)
		return 0;
	if (count > sizeof(tpm_chip.data))
		count = sizeof(tpm_chip.data);
	if ((len = tpm_recv(&tpm_chip, tpm_chip.data, count)) < 0)
		return len;
	if (copy_to_user(buf, tpm_chip.data, len))
		return -EFAULT;
	return len;
}

static int
tpm_send(struct tpm_chip *chip, u8 *buf, size_t count)
{
	int i, status=0;

	/* send abort and check for busy bit to go away */
	outb(STATUS_BIT_ABORT, chip->base + 1);
	for (i = 0; i < TPM_TIMEOUT; i++) {
		status = inb(chip->base+1);
		if ((status & STATUS_BIT_BUSY) == 0)
			break;
		current->state = TASK_UNINTERRUPTIBLE;
		schedule_timeout(TPM_SCHEDULE_TIMEOUT);
	}
	if (i == TPM_TIMEOUT) {
		printk(KERN_ERR TPM_MODULE_NAME ": timeout 3\n");
		return -EIO;
	}

#ifdef ERROR_CHECKING
	/* calculate CRC if there is not one already */
	if (*buf == 0) {
		u8 crc = crc8(buf, count);
		if (crc == 0)
			crc = 0xFF;
		*buf = crc;
	}
#endif /* ERROR_CHECKING */

	/* write n - 1 bytes */
	for (i = 0; i < count-1; i++)
		outb(buf[i], chip->base);

#ifdef ERROR_CHECKING
	outb(STATUS_BIT_LASTBYTE, chip->base + 1);
#endif /* ERROR_CHECKING */

	/* send last byte */
	outb(buf[i], chip->base);
	for (i = 0; i < TPM_TIMEOUT; i++) {
		status = inb(chip->base+1);
		if (status & (STATUS_BIT_BUSY|STATUS_BIT_DATA_AVAIL|
						STATUS_BIT_REWRITE))
			break;
		current->state = TASK_UNINTERRUPTIBLE;
		schedule_timeout(TPM_SCHEDULE_TIMEOUT);
	}
	if (i == TPM_TIMEOUT) {
		printk(KERN_ERR TPM_MODULE_NAME ": timeout 4\n");
		return -EIO;
	}

#ifdef ERROR_CHECKING
	if (status & STATUS_BIT_REWRITE) {
		printk(KERN_ERR TPM_MODULE_NAME
			": write retry (status = 0x%x)\n", status);
		return -EIO;
	}
#endif /* ERROR_CHECKING */

	if ((status & (STATUS_BIT_BUSY|STATUS_BIT_DATA_AVAIL)) == 0) {
		printk(KERN_ERR TPM_MODULE_NAME
			": write timeout (status = 0x%x)\n", status);
		return -EIO;
	}
	return count;
}

static ssize_t
tpm_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
#ifdef TPM_DEBUG
	printk("tpm_write\n");
#endif
	if (count == 0)
		return 0;
	if (count > sizeof(tpm_chip.data))
		count = sizeof(tpm_chip.data);
	
	if (copy_from_user(tpm_chip.data, buf, count))
		return -EFAULT;

	return tpm_send(&tpm_chip, tpm_chip.data, count);
}

static int
tpm_ioctl(struct inode *inode, struct file *file,
		unsigned int cmd, unsigned long arg)
{
	int i, status;

#ifdef TPM_DEBUG
	printk("tpm_ioctl\n");
#endif
	switch (cmd) {
	case TPMIOC_CANCEL:
	        outb(STATUS_BIT_ABORT, tpm_chip.base + 1);
		for (i = 0; i < TPM_TIMEOUT; i++) {
			status = inb(tpm_chip.base+1);
			if ((status & STATUS_BIT_BUSY) == 0)
				break;
			current->state = TASK_UNINTERRUPTIBLE;
			schedule_timeout(TPM_SCHEDULE_TIMEOUT);
		}
		if (i == TPM_TIMEOUT) {
			printk(KERN_ERR TPM_MODULE_NAME ": timeout 5\n");
			return -EIO;
		}
		return 0;
	default:
		return -ENOTTY;
	}
	return -ENODEV;
}

#ifdef CONFIG_PM
static u8 tpm_savestate[] = {
	0, 193,		/* TPM_TAG_RQU_COMMAND */
	0, 0, 0, 10,	/* blob length, bytes */
	0, 0, 0, 152	/* TPM_ORD_SaveState */
};

/*
 * We are about to suspend. Save the TPM state
 * so that it can be restored.
 */
static int
tpm_pm_suspend(struct pci_dev *pci_dev, u32 pm_state)
{
	tpm_send(&tpm_chip, tpm_savestate, sizeof(tpm_savestate));
	return 0;
}

/*
 * Resume from a power safe. The BIOS already restored
 * the TPM state.
 */
static int
tpm_pm_resume(struct pci_dev *pci_dev)
{
	tpm_init(pci_dev, &tpm_chip);
	return 0;
}
#endif /* CONFIG_PM */

static struct pci_device_id tpm_pci_tbl [] __initdata = {
	{PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_ICH2LPC,
	 PCI_ANY_ID, PCI_ANY_ID, 0, 0, ICH2LPC},
	{PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_ICH3LPCM,
	 PCI_ANY_ID, PCI_ANY_ID, 0, 0, ICH3LPCM},
	{PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_ICH4LPC,
	 PCI_ANY_ID, PCI_ANY_ID, 0, 0, ICH4LPC},
	{PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_ICH4LPCM,
	 PCI_ANY_ID, PCI_ANY_ID, 0, 0, ICH4LPCM},
	{0,}
};

MODULE_DEVICE_TABLE (pci, tpm_pci_tbl);


static struct pci_driver tpm_pci_driver = {
        name:           TPM_MODULE_NAME,
        id_table:       tpm_pci_tbl,
        probe:          tpm_probe,
        remove:         __devexit_p(tpm_remove),
#ifdef CONFIG_PM
        suspend:        tpm_pm_suspend,
        resume:         tpm_pm_resume,
#endif /* CONFIG_PM */
};

static struct file_operations tpm_fops = {
	owner:		THIS_MODULE,
	llseek:		no_llseek,
	open:		tpm_open,
	read:		tpm_read,
	write:		tpm_write,
	ioctl:		tpm_ioctl,
	release:	tpm_release
};

static struct miscdevice tpm_dev = {
	TPM_MINOR,
	"tpm",
	&tpm_fops
};

static int __init
init_tpm(void)
{
#if 0
	if (!pci_present())
		return -ENODEV;
#endif

	if (pci_register_driver(&tpm_pci_driver) || !tpm_chip.initialized) {
		pci_unregister_driver(&tpm_pci_driver);
		return -ENODEV;
        }

	if (misc_register(&tpm_dev)) {
		printk(KERN_ERR TPM_MODULE_NAME
			": unable to misc_register minor %d\n", TPM_MINOR);
		pci_unregister_driver(&tpm_pci_driver);
		return -ENODEV;
	}

	return 0;
}

static void __exit
cleanup_tpm(void)
{
	misc_deregister(&tpm_dev);
	pci_unregister_driver(&tpm_pci_driver);
#ifdef TPM_DEBUG
	printk(TPM_MODULE_NAME ": cleanup\n");
#endif
}

module_init(init_tpm);
module_exit(cleanup_tpm);

MODULE_AUTHOR("Leendert van Doorn (leendert@watson.ibm.com)");
MODULE_DESCRIPTION("TPM Driver");
MODULE_LICENSE("GPL");

