/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4io/server/include/res.h
 * \brief  L4Env l4io I/O Server Resource Management Module Interface
 *
 * \date   05/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __L4IO_SERVER_INCLUDE_RES_H_
#define __L4IO_SERVER_INCLUDE_RES_H_

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

#endif
