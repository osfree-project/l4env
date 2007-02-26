/* $Id$ */
/*****************************************************************************/
/**
 * \file	generic_io/idl/generic_io-sys.h
 *
 * \brief	L4Env I/O Server IPC interface constants
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

#ifndef _IO_IO_SYS_H
#define _IO_IO_SYS_H

/* IPC function numbers */

/* misc if */
#define req_l4_io_register_client		0x01
#define req_l4_io_unregister_client		0x02
#define req_l4_io_map_info			0x04

/* res if */
#define req_l4_io_request_region		0x11
#define req_l4_io_release_region		0x12
#define req_l4_io_request_mem_region		0x13
#define req_l4_io_release_mem_region		0x14
#define req_l4_io_request_dma			0x15
#define req_l4_io_release_dma			0x16

/* pci if */
#define req_l4_io_pci_find_slot			0x20
#define req_l4_io_pci_find_device		0x21
#define req_l4_io_pci_find_class		0x22

#define req_l4_io_pci_enable_device		0x30
#define req_l4_io_pci_disable_device		0x31
#define req_l4_io_pci_set_master		0x32
#define req_l4_io_pci_set_power_state		0x33


#define req_l4_io_pci_read_config_byte		0x41
#define req_l4_io_pci_read_config_word		0x42
#define req_l4_io_pci_read_config_dword		0x43
#define req_l4_io_pci_write_config_byte		0x44
#define req_l4_io_pci_write_config_word		0x45
#define req_l4_io_pci_write_config_dword	0x46

#endif /* !_IO_IO_SYS_H */
