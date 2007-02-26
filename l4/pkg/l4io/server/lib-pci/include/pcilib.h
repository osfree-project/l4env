/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4io/server/lib-pci/include/pcilib.h
 * \brief  L4Env l4io PCIlib Interface
 *
 * \date   05/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __L4IO_SERVER_LIB_PCI_INCLUDE_PCILIB_H_
#define __L4IO_SERVER_LIB_PCI_INCLUDE_PCILIB_H_

/** PCI library initialization */
int PCI_init(void);

/** dump PCI specific /proc-fs entries */
void PCI_dump_procfs(void);

/** fill I/O specific pci_dev struct with info in Linux pci_dev */
unsigned short PCI_linux_to_io(void *linus, void *l4io);

#endif
