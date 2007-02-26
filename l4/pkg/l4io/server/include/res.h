/* $Id$ */
/*****************************************************************************/
/**
 * \file	l4io/server/include/res.h
 *
 * \brief	L4Env l4io I/O Server Resource Management Module Interface
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

#ifndef _IO_RES_H
#define _IO_RES_H

int io_res_init(io_client_t*);

/* PCIlib callbacks */
int callback_request_region(unsigned long, unsigned long);
int callback_request_mem_region(unsigned long, unsigned long);
void callback_announce_mem_region(unsigned long, unsigned long);
int callback_handle_pci_device(unsigned short vendor, unsigned short device);
int add_device_inclusion(const char*s);
int add_device_exclusion(const char*s);

/* IO port space */
#define MAX_IO_PORTS	0xffff	/**< 64K IO ports */
/* IO memory space */
#define MAX_IO_MEMORY	0xffffffff	/**< some GB IO memory;
					 * remember: physically addressed */
/* ISA DMA */
#define MAX_ISA_DMA	8	/**< 8 DMA channels */

#ifndef NO_DOX
#ifdef DEBUG
void list_res(void);
#else
#define list_res()
#endif
#endif

#endif /* !_IO_RES_H */
