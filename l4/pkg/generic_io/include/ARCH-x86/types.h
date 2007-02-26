/* $Id$ */
/*****************************************************************************/
/**
 * \file	generic_io/include/l4/generic_io/types.h
 *
 * \brief	L4Env I/O Client API Types
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

#ifndef _L4IO_TYPES_H
#define _L4IO_TYPES_H

/* L4 includes */
#include <l4/sys/types.h>

/*****************************************************************************/
/**
 * \brief IO driver types.
 * \ingroup grp_misc
 *
 * One dword encoding driver source and type. 
 *
 * <em>This is a proposal/idea. A final version of driver/server descriptions
 * had to be more extensive and _not_ only for driver servers.</em>
 *
 * 0x00000000 is reserved.
 *
 * -# src
 *	- 00 ... native L4
 *	- 01 ... Linux
 *	- 10 ... OSKit
 *	- 11 ... others
 * -# dsi
 *	- 7:4 ... major version
 *	- 3:0 ... minor version
 * -# blk/class
 *	- 0xxxxxxx ... character device
 *		- x0000000 ... serio
 *		- x0000100 ... snd
 *	- 1xxxxxxx ... block device
 */
/*****************************************************************************/
typedef struct l4io_drv
{
  unsigned src:2;		/**< source of driver */
  unsigned dsi:8;		/**< DSI version supported */
  unsigned class:8;		/**< driver class */
  unsigned padding:14;		/**< place holder */
} l4io_drv_t;

#define L4IO_DRV_INVALID ((l4io_drv_t){0,0,0,0})
				/**< invalid type */

/*****************************************************************************/
/**
 * \brief PCI resources (regions).
 * \ingroup grp_misc
 */
/*****************************************************************************/
typedef struct l4io_res
{
  unsigned long start;		/**< begin of region */
  unsigned long end;		/**< end of region */
  unsigned long flags;		/**< flags for PCI resource regions */
} l4io_res_t;

/*****************************************************************************/
/**
 * \brief PCI device handle.
 * \ingroup grp_misc
 */
/*****************************************************************************/
typedef unsigned short l4io_pdev_t;

/*****************************************************************************/
/**
 * \brief PCI device information (struct).
 * \ingroup grp_misc
 */
/*****************************************************************************/
typedef struct l4io_pci_dev
{
  unsigned char bus;		/* PCI bus number */
  unsigned char devfn;		/* encoded device [7:3] & function [2:0] index */
  unsigned short vendor;
  unsigned short device;
  unsigned short sub_vendor;
  unsigned short sub_device;
  unsigned long class;		/* 3 bytes: (base,sub,prog-if) */

  unsigned long irq;
#define L4IO_PCIDEV_RES	12	/**< number of PCI resource regions */
  l4io_res_t res[L4IO_PCIDEV_RES];	/* resource regions used by device:
					 * 0-5  standard PCI regions (base addresses)
					 * 6    expansion ROM
					 * 7-10 unused for devices */
  char name[80];
  char slot_name[8];

  l4io_pdev_t handle;		/* handle for this device */
} l4io_pci_dev_t;

/*****************************************************************************/
/**
 * \brief   I/O Info Page Structure.
 * \ingroup grp_misc
 *
 * This is the L4Env's I/O server info page.
 * We have 4KB and fill it 0...L4_PAGESIZE-1.
 *
 * \krishna DDE libraries (resp. all io clients) have to do some assembler
 * magic to put their symbols at the right place. (look into package \c dde_test
 * resp. \c io \c (examples/dummy/))
 */
/*****************************************************************************/
struct l4io_info
{
  l4_uint32_t magic;                    /**< magic number */
  volatile l4_uint32_t jiffies;         /**< jiffies */
  l4_uint32_t hz;                       /**< update frequency for jiffies (HZ) */
  volatile struct {
    long tv_sec, tv_usec;
  } xtime;                              /**< xtime */
  l4_uint8_t padding[L4_PAGESIZE - 24]; /**< place holder */
  l4_uint32_t omega0;                   /**< omega0 flag (1 if started) */
} __attribute__ ((aligned(L4_PAGESIZE)));

typedef struct l4io_info l4io_info_t;	/**< io info page type */

#define L4IO_INFO_MAGIC	0x496f6f49	/**< io magic is "IooI" */

#endif /* !_L4IO_TYPES_H */
