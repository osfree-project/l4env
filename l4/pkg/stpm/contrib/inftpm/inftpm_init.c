/**
 * inftpm_init.c - init functions
 *
 * A device driver for a Infineon TPM (SLD9630TT)
 *
 * Copyright (C) 2004 Bernhard Kauer <kauer@os.inf.tu-dresden.de>
 */

#include <linux/init.h>

#include "inftpm.h"
#include "inftpm_init.h"

enum TPM_DEV_REGISTER
{
	TPM_DEV_ID1    = 0x20,
	TPM_DEV_ID2,
	TPM_DEV_CFGL   = 0x26,
	TPM_DEV_CFGH,
	TPM_DEV_DAR    = 0x30,
	TPM_DEV_IOLIMH = 0x60,
	TPM_DEV_IOLIML,
	TPM_DEV_IRQSEL = 0x70,
	TPM_DEV_IRTYPE,
	TPM_DEV_IDVENL = 0xF1,
	TPM_DEV_IDVENH,
	TPM_DEV_IDPDL,
	TPM_DEV_IDPDH,
};


/**
 * Change a register of the pci bus.
 *
 * This should only work with 4 Byte aligned offset, because we
 * use dword access.
 *
 * Returns the new value of this register.
 */
static u32 inline __init
tpm_pci_set_register(struct pci_dev *pci_dev, u32 offset, u32 mask, u32 value)
{
	u32 tmp;
	debug("offs %08x mask %08x value %08x",offset,mask,value);
	pci_read_config_dword(pci_dev, offset, &tmp);
	tmp = (tmp & mask) |  value;
	pci_write_config_dword(pci_dev, offset, tmp);
	pci_read_config_dword(pci_dev, offset, &tmp);	
	debug("tmp %08x res %08x",tmp, tmp & ~mask);
	return tmp & ~mask;
}

/**
 * Set a bit in the pci bus. 
 *
 * This should only work with 4 Byte aligned offset, because we
 * use dword access.
 *
 * Returns the new value.
 */
static inline u32 __init
tpm_pci_set_bit(struct pci_dev *pci_dev, int offset, int bit, int value)
{	
	return tpm_pci_set_register(
		pci_dev,
		offset,
		(~(1<<bit)),
		(value?1:0)<<bit
		)>> bit;
}


/**
 * Init the lpc bus in the ich5 through the pci bus.
 */
int __init
tpm_pci_init(struct pci_dev *pci_dev, u16 base_addr)
{
        u32 res;

	if (pci_enable_device(pci_dev))
		return -EIO;

	pci_read_config_dword(pci_dev, PCI_GEN1_DEC, &res);
	debug("GEN1_DEC: %08x",res);
	pci_read_config_dword(pci_dev, PCI_GEN2_DEC, &res);
	debug("GEN2_DEC: %08x",res);

	if (!request_region(base_addr,16,MODULE_NAME))
	{
		error("port region %4x-%4x conflicts with another device",
		      base_addr,base_addr+15);
		return -EBUSY;
	}
	
	// init ICH5 (enable LPC via LPC_EN)
	res = tpm_pci_set_bit(pci_dev, PCI_GEN1_DEC, 29, 1);
	if (!res) {
		error("cannot enable LPC");
		return -ENODEV;
	}

	// set generic decode range 2 register
	res = tpm_pci_set_register(pci_dev, 
				   PCI_GEN2_DEC, 
				   0xFFFF0000,
				   (base_addr & 0xFFF0) | 0x1);
	if ((res & 0xFFF1) != ((base_addr & 0xFFF0) | 0x1)){
		error("set gen2reg failed");
		return -ENODEV;
	}

	return 0;
}

/**
 * Reset the pci bus and free the resources.
 */
int __exit
tpm_pci_exit(struct pci_dev *pci_dev, u16 base_addr)
{
	// reset GEN2
	tpm_pci_set_register(pci_dev, 
			     PCI_GEN2_DEC, 
			     0xFFFF0000,
			     0);
	// disable LPC
	tpm_pci_set_bit(pci_dev, PCI_GEN1_DEC, 29, 0);

	// relase port region
	release_region(base_addr,16);

	return 0;
}

/**
 * Enable the index/data register pair to access the
 * configuration register of the tpm.
 */
static inline void 
tpm_dev_enable_registers(u16 config_addr)
{
	outb(0x55,config_addr);
}

/**
 * Disable the index/data register pair to access the
 * configuration register of the tpm.
 */
static inline void 
tpm_dev_disable_registers(u16 config_addr)
{
        outb(0xAA,config_addr);
}

/**
 * Set a device configuration register.
 */
static inline void
tpm_dev_set_register(u16 config_addr, char addr, char value)
{
	outb(addr,config_addr);
	outb(value,config_addr+1);
}
/**
 * Get a device configuration register.
 */
static inline char
tpm_dev_get_register(u16 config_addr, u8 reg)
{
	outb(reg,config_addr);
	return inb(config_addr+1);
}

/**
 * Set the base addr in the device config registers.
 */
static inline void
tpm_dev_set_base_ioaddr(u16 config_addr, short value)
{
	debug("config_addr %08x value %04x",config_addr,value);
	tpm_dev_set_register(config_addr, TPM_DEV_IOLIMH,
			     ((value & 0xFF00) >> 0x08));
	tpm_dev_set_register(config_addr, TPM_DEV_IOLIML,
			     value & 0x00FF);
}

/**
 * Set the irq nr and the irq ctrl.
 */
static inline void
tpm_dev_set_irq(u16 config_addr, int irq_nr, int irq_ctrl)
{
	tpm_dev_set_register(config_addr, TPM_DEV_IRQSEL,
			     (irq_nr & 0x1F));
	tpm_dev_set_register(config_addr,TPM_DEV_IRTYPE, 
			     irq_ctrl);
}

/**
 * Set the "device enable register".
 */
static inline void
tpm_dev_set_dar(u16 config_addr, char value)
{
	// we support only one logical dev here!
	tpm_dev_set_register(config_addr, TPM_DEV_DAR,
			 value & 0x1);
}

/**
 * Compare a 16 bit value with two device register.
 * Returns 0 if they are equal.
 */
static inline int
tpm_dev_cmp_two_register(u16 config_addr,u8 base, u16 value)
{
	int res;
	res = tpm_dev_get_register(config_addr, base) & 0xff;
	res = res | (tpm_dev_get_register(config_addr, base+1) << 8);
	if (res != value)
		debug("check: %04x vs. %04x",res,value);
	return res != value;
}

/**
 * Check whether a tpm is available.
 */
static int __init
tpm_dev_check_default_config(u16 config_addr)
{

	tpm_dev_enable_registers(config_addr);

	if (tpm_dev_cmp_two_register(config_addr, 
				 TPM_DEV_ID1, 
				 TPM_DEV_ID_VALUE))
	{
		error("check id failed");
		return -ENODEV;
	}
	
	if (tpm_dev_cmp_two_register(config_addr, 
				     TPM_DEV_IDVENL, 
				     TPM_DEV_VEN_VALUE))
	{
		
		error("check vendor failed");
		return -ENODEV;
	}

	if (tpm_dev_cmp_two_register(config_addr, 
				     TPM_DEV_IDPDL, 
				     TPM_DEV_PCI_VALUE))
	{
		error("check pci ids failed");
		return -ENODEV;
	}

	// Do not disable the register here! Some bogus bios set them
	// to 2E, so we have to call tpm_set_config_addr directly
	// after this function which should fix this bug.

	// tpm_dev_disable_registers(config_addr);

	debug("checks successful");
	return 0;
}

/**
 * Set the config addr in the device config register of the tpm.
 */
static inline int
tpm_dev_set_config_addr(u16 config_addr)
{
	int res = 0;

	tpm_dev_enable_registers(config_addr);
	if (tpm_dev_cmp_two_register(config_addr,
				     TPM_DEV_CFGL,
				     config_addr))
	{
		error("config addr invalid!");
		tpm_dev_set_register(config_addr,
				     TPM_DEV_CFGL,
				     config_addr & 0x00FF);
		tpm_dev_set_register(config_addr,
				     TPM_DEV_CFGH,
				     config_addr >> 8);
		res = 1;
	}
	tpm_dev_disable_registers(config_addr);
	return res;
}

/**
 * Config the tpm via the lpc bus.
 */
static void
tpm_dev_config_tpm(u16 config_addr, short base_io_addr)
{
	debug("config_tpm");
	tpm_dev_enable_registers(config_addr);
	tpm_dev_set_base_ioaddr(config_addr,base_io_addr);
	// XXX we have no irqs yet
	tpm_dev_set_irq(config_addr,0,0x2);
	debug("enable dar");
	tpm_dev_set_dar(config_addr,1);
	tpm_dev_disable_registers(config_addr);
}

/**
 * Init the tpm via the device control register.
 */
int __init
tpm_dev_init(u16 config_addr, u16 base_addr)
{
	int res;
	if ((res = tpm_dev_check_default_config(config_addr)))
		return res;
	
	if ((tpm_dev_set_config_addr(config_addr)))
		error("set config_addr to %02x",config_addr);
	
	tpm_dev_config_tpm(config_addr,base_addr);
	return res;
}


/**
 * Reinit the tpm after a reset.
 */
int
tpm_dev_reinit(u16 config_addr, u16 base_addr)
{
  if ((tpm_dev_set_config_addr(config_addr)))
	  error("set config_addr to %02x",config_addr);
  tpm_dev_config_tpm(config_addr,base_addr);
  return 0;
}
