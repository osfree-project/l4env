#include <l4/pci/libpci.h>
#include <l4/generic_io/libio.h>
#include <l4/sys/kdebug.h>
#include <l4/util/macros.h>
#include <l4/l4rm/l4rm.h>
#include <l4/sigma0/sigma0.h>
#include "io_support.h"
#include "types.h"

unsigned long virt_offset = 0;

static int use_l4io;

struct pci_device
{
  uint32_t			class;
  uint16_t			vendor, dev_id;
  const char			*name;
  unsigned int			membase;
  unsigned int			ioaddr;
  unsigned int			romaddr;
  unsigned char			devfn;
  unsigned char			bus;
  const struct pci_driver	*driver;
};

extern int scan_drivers (int type, uint32_t class, uint16_t vendor, 
			 uint16_t device, const struct pci_driver *last_driver,
			 struct pci_device *dev);
extern int scan_pci_bus (int type, struct pci_device *dev);

void
io_support_init(int l4io)
{
  use_l4io = l4io;
  if (!l4io)
    pcibios_init();
}

l4_addr_t
io_support_remap(l4_addr_t phys_addr, l4_size_t size)
{
  l4_addr_t virt_addr;
  l4_offs_t offset;
  l4_uint32_t rg;
  int error;

  if (use_l4io)
    {
      if (!(virt_addr = l4io_request_mem_region(phys_addr, size, 0, &offset)))
	Panic("Can't request memory region from l4io.");

      LOG_printf("Mapped I/O memory %08lx => %08lx+%06lx [%dkB] via l4io\n",
	     phys_addr, virt_addr, offset, size >> 10);

      return virt_addr;
    }
  else
    {
      extern l4_threadid_t l4rm_task_pager_id;

      offset     = phys_addr - l4_trunc_superpage(phys_addr);
      size       = l4_round_superpage(offset+size);
      phys_addr  = l4_trunc_superpage(phys_addr);

      if ((error = l4rm_area_reserve(size, L4RM_LOG2_ALIGNED, &virt_addr, &rg)))
	Panic("Error %d reserving region size=%dMB for memory",
	    error, size>>20);

      LOG_printf("Mapping I/O memory %08lx => %08lx+%06lx [%dkB]\n",
	    phys_addr+offset, virt_addr, offset, size>>10);

      if ((error = l4sigma0_map_iomem(l4rm_task_pager_id, phys_addr,
				      virt_addr, size, 0)))
	{
	  switch (error)
	    {
	    case -2: Panic("IPC error mapping I/O memory");
	    case -3: Panic("No fpage received mapping I/O memory");
	    case -4: Panic("I/O memory address is below 2GB (0x80000000),\n"
			   "don't know how to map it as device super I/O "
			   "page.");
	    }
	}

      return virt_addr + offset;
    }
}

void
io_support_unmap(l4_addr_t virt_addr)
{
#if 0
  if (use_l4io)
    {
      int error;

      if ((error = l4io_release_mem_region(virt_addr, size)) < 0)
	Panic("Error %d releasing region %08x-%08x at l4io", 
	    error, virt_addr, virt_addr+size);
      LOG_printf("Unmapped I/O memory via l4io\n");
    }
  else
    {
      if (l4rm_area_release_addr((void*)(virt_addr & L4_SUPERPAGEMASK)))
	Panic("Error releasing region %08x-%08x",
	    virt_addr, virt_addr+size);

      LOG_printf("Unmapped I/O %memory. WARNING: Not unmapped!\n");
    }
#else
  LOG_printf("WARNING: iounmap not implemented (%08lx)\n", virt_addr);
#endif
}

void
io_support_scan_pci_bus (int type, struct pci_device *dev)
{
  if (use_l4io)
    {
      l4io_pdev_t start = 0;
      l4io_pci_dev_t new;
      l4_uint32_t ioaddr, membase;
      unsigned int bus, devfn;
      int reg;

      for (;;)
	{
	  if (l4io_pci_find_class (PCI_CLASS_NETWORK_ETHERNET<<8, start, &new))
	    return;

	  scan_drivers(type, 0, new.vendor, new.device, 0, dev);
	  if (dev->driver)
	    {
	      dev->bus    = bus   = new.bus;
	      dev->devfn  = devfn = new.devfn;
	      dev->class  = PCI_CLASS_NETWORK_ETHERNET;
	      dev->vendor = new.vendor;
	      dev->dev_id = new.device;

	      l4io_pci_readl_cfg((bus<<8)|devfn, PCI_BASE_ADDRESS_1, &membase);

	      dev->membase = membase;

	      for (reg=PCI_BASE_ADDRESS_0; reg<=PCI_BASE_ADDRESS_5; reg+=4)
		{
		  l4io_pci_readl_cfg((bus<<8)|devfn, reg, &ioaddr);
		  if (  (ioaddr & PCI_BASE_ADDRESS_IO_MASK) == 0
		      ||(ioaddr & PCI_BASE_ADDRESS_SPACE_IO) == 0)
		    continue;

		  dev->ioaddr = ioaddr & PCI_BASE_ADDRESS_IO_MASK;
		  break;
		}

	      return;
	    }

	  start = new.handle;
	}
    }
  else
    scan_pci_bus(type, dev);
}

void
io_support_pci_read_config_byte (unsigned bus, unsigned devfn,
				 unsigned where, l4_uint8_t *value)
{
  int ret = (use_l4io) ? l4io_pci_readb_cfg((bus<<8)|devfn, where, value)
		       : pcibios_read_config_byte(bus, devfn, where, value);
  if (ret)
    enter_kdebug("pci_read_byte");
}

void
io_support_pci_write_config_byte (unsigned bus, unsigned devfn,
				  unsigned where, l4_uint8_t value)
{
  int ret = (use_l4io) ? l4io_pci_writeb_cfg((bus<<8)|devfn, where, value)
		       : pcibios_write_config_byte(bus, devfn, where, value);
  if (ret)
    enter_kdebug("pci_write_byte");
}

void
io_support_pci_read_config_word (unsigned bus, unsigned devfn,
				 unsigned where, l4_uint16_t *value)
{
  int ret = (use_l4io) ? l4io_pci_readw_cfg((bus<<8)|devfn, where, value)
		       : pcibios_read_config_word(bus, devfn, where, value);
  if (ret)
    enter_kdebug("pci_read_word");
}

void
io_support_pci_write_config_word (unsigned bus, unsigned devfn,
				  unsigned where, l4_uint16_t value)
{
  int ret = (use_l4io) ? l4io_pci_writew_cfg((bus<<8)|devfn, where, value)
		       : pcibios_write_config_word(bus, devfn, where, value);
  if (ret)
    enter_kdebug("pci_write_word");
}

void
io_support_pci_read_config_dword (unsigned bus, unsigned devfn,
				  unsigned where, l4_uint32_t *value)
{
  int ret = (use_l4io) ? l4io_pci_readl_cfg((bus<<8)|devfn, where, value)
		       : pcibios_read_config_dword(bus, devfn, where, value);
  if (ret)
    enter_kdebug("pci_read_dword");
}

void
io_support_pci_write_config_dword (unsigned bus, unsigned devfn,
				   unsigned where, l4_uint32_t value)
{
  int ret = (use_l4io) ? l4io_pci_writel_cfg((bus<<8)|devfn, where, value)
		       : pcibios_write_config_dword(bus, devfn, where, value);
  if (ret)
    enter_kdebug("pci_write_dword");
}
