/* $Id$ */
/*****************************************************************************/
/**
 * \file	l4io/server/lib-pci/include/pcilib.h
 *
 * \brief	L4Env l4io PCIlib Interface
 *
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/

#ifndef _LIBPCI_PCILIB_H
#define _LIBPCI_PCILIB_H

/*****************************************************************************
 * library initialization                                                    *
 *****************************************************************************/
int PCI_init(void);

/*****************************************************************************
 * dump PCI specific /proc-fs entries                                        *
 *****************************************************************************/
void PCI_dump_procfs(void);

/*****************************************************************************
 * fill I/O specific pci_dev struct with info in Linux pci_dev               *
 *****************************************************************************/
unsigned short pci_linux_to_io(void *linus, void *l4io);

/*****************************************************************************
 * services                                                                  *
 *****************************************************************************/
#ifndef __in_pcilib
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

#endif /* !_LIBPCI_PCILIB_H */
