#ifndef __CON_HW_PCI_H__
#define __CON_HW_PCI_H__

#include <l4/pci/libpci.h>
#include <l4/generic_io/libio.h>
#include "init.h"

#define __init	__attribute__((section(".text.init")))

struct pci_device_id
{
  unsigned short vendor, device;
  unsigned short svid, sid;
  unsigned long driver_data;
};

int  pci_probe(con_accel_t *accel);
void pci_register(struct pci_device_id *tbl, 
		  int(*probe)(unsigned int bus, unsigned int devfn,
		              struct pci_device_id *dev, con_accel_t *accel));

/* krishna: looks a bit sick, but we want complete L4IO compatibility NOW.

   1. Do not use pcibios_*() or pci_*() from pcilib directly - use these macros.
   2. Test for use_l4io before real execution

   l4io_pdev_t handle is stored in [bus:devfn].
*/

#define PCIBIOS_READ_CONFIG_BYTE(bus, devfn, where, val)	\
	do {							\
	  if (!use_l4io)					\
	    pcibios_read_config_byte(bus, devfn, where, val);	\
	  else							\
	    l4io_pci_readb_cfg((bus<<8)|devfn, where, val);	\
	} while (0)

#define PCIBIOS_READ_CONFIG_WORD(bus, devfn, where, val)	\
	do {							\
	  if (!use_l4io)					\
	    pcibios_read_config_word(bus, devfn, where, val);	\
	  else							\
	    l4io_pci_readw_cfg((bus<<8)|devfn, where, val);	\
	} while (0)

#define PCIBIOS_READ_CONFIG_DWORD(bus, devfn, where, val)	\
	do {							\
	  if (!use_l4io)					\
	    pcibios_read_config_dword(bus, devfn, where, val);	\
	  else							\
	    l4io_pci_readl_cfg((bus<<8)|devfn, where, val);	\
	} while (0)

#define PCIBIOS_WRITE_CONFIG_BYTE(bus, devfn, where, val)	\
	do {							\
	  if (!use_l4io)					\
	    pcibios_write_config_byte(bus, devfn, where, val);	\
	  else							\
	    l4io_pci_writeb_cfg((bus<<8)|devfn, where, val);	\
	} while (0)

#define PCIBIOS_WRITE_CONFIG_WORD(bus, devfn, where, val)	\
	do {							\
	  if (!use_l4io)					\
	    pcibios_write_config_word(bus, devfn, where, val);	\
	  else							\
	    l4io_pci_writew_cfg((bus<<8)|devfn, where, val);	\
	} while (0)

#define PCIBIOS_WRITE_CONFIG_DWORD(bus, devfn, where, val)	\
	do {							\
	  if (!use_l4io)					\
	    pcibios_write_config_dword(bus, devfn, where, val);	\
	  else							\
	    l4io_pci_writel_cfg((bus<<8)|devfn, where, val);	\
	} while (0)



#endif

