/** $Id$ */
/*****************************************************************************/
/**
 * \file   l4io/server/lib-pci/include/pcilib_pci.h
 * \brief  L4Env l4io PCIlib Interface (linux/pci.h extracts)
 *
 * \date   05/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __L4IO_SERVER_LIB_PCI_INCLUDE_PCILIB_PCI_H_
#define __L4IO_SERVER_LIB_PCI_INCLUDE_PCILIB_PCI_H_

void *pci_find_device(unsigned int vendor, unsigned int device, const void *from);
void *pci_find_class(unsigned int class, const void *from);
void *pci_find_slot(unsigned int bus, unsigned int devfn);

int pci_read_config_byte(void *dev, int where, l4_uint8_t * val);
int pci_read_config_word(void *dev, int where, l4_uint16_t * val);
int pci_read_config_dword(void *dev, int where, l4_uint32_t * val);
int pci_write_config_byte(void *dev, int where, l4_uint8_t val);
int pci_write_config_word(void *dev, int where, l4_uint16_t val);
int pci_write_config_dword(void *dev, int where, l4_uint32_t val);

int pci_enable_device(void *dev);
int pci_disable_device(void *dev);
void pci_set_master(void *dev);
int pci_set_power_state(void *dev, int state);

#endif
