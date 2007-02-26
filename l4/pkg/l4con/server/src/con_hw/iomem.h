/*!
 * \file	iomem.h
 * \brief	I/O memory stuff
 *
 * \date	07/2002
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef __IOMEM_H_
#define __IOMEM_H_

extern int map_io_mem(l4_addr_t addr, l4_size_t size,
		      const char *id, l4_addr_t *vaddr);

extern void unmap_io_mem(l4_addr_t addr, l4_size_t size,
		         const char *id, l4_addr_t vaddr);

#endif
