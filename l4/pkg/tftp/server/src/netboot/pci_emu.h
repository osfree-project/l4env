#ifndef PCI_EMU_H
#define PCI_EMU_H

#define pcibios_read_config_byte(bus, devfn, where, value) \
	io_support_pci_read_config_byte(bus, devfn, where, value)

#define pcibios_read_config_word(bus, devfn, where, value) \
	io_support_pci_read_config_word(bus, devfn, where, value)

#define pcibios_read_config_dword(bus, devfn, where, value) \
	io_support_pci_read_config_dword(bus, devfn, where, value)

#define pcibios_write_config_byte(bus, devfn, where, value) \
	io_support_pci_write_config_byte(bus, devfn, where, value)

#define pcibios_write_config_word(bus, devfn, where, value) \
	io_support_pci_write_config_word(bus, devfn, where, value)

#define pcibios_write_config_dword(bus, devfn, where, value) \
	io_support_pci_write_config_dword(bus, devfn, where, value)

#include "io_support.h"

#endif
