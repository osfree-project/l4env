/**
 * \file   ldso/lib/ldso/emul_linux.h
 * \brief  Adaption layer for Linux system calls to L4env
 *
 * \date   2005/05/12
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef _EMUL_LINUX_H
#define _EMUL_LINUX_H

#define MMAP_START	0x00100000
#define MMAP_END	0x06000000

void mmap_list_regions(void);

#endif
