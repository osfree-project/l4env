#ifndef PCI_SUPPORT_H
#define PCI_SUPPORT_H

#include <l4/sys/l4int.h>

struct pci_device;

void      io_support_init(int l4io);
l4_addr_t io_support_remap(l4_addr_t phys_addr, l4_size_t size);
void      io_support_unmap(l4_addr_t virt_addr);
void      io_support_scan_pci_bus (int type, struct pci_device *dev);
void      io_support_pci_read_config_byte (unsigned bus, unsigned devfn,
					   unsigned where, l4_uint8_t *value);
void      io_support_pci_write_config_byte (unsigned bus, unsigned devfn, 
					    unsigned where, l4_uint8_t value);
void      io_support_pci_read_config_word (unsigned bus, unsigned devfn, 
					   unsigned where, l4_uint16_t *value);
void      io_support_pci_write_config_word (unsigned bus, unsigned device_fn,
					    unsigned where, l4_uint16_t value);
void      io_support_pci_read_config_dword (unsigned bus, unsigned devfn,
					    unsigned where, l4_uint32_t *value);
void      io_support_pci_write_config_dword (unsigned bus, unsigned devfn,
					     unsigned where, l4_uint32_t value);

#endif
