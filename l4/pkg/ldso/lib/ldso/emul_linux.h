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

#include <l4/sys/l4int.h>

#define MMAP_START	0xA0000000
#define MMAP_END	0xA8000000

#if (__GNUC__ == 3 && __GNUC_MINOR__ >= 3) || __GNUC__ > 3
#define HIDDEN __attribute__((visibility("hidden")))
#else
#define HIDDEN
#endif

void      _dl_seek(int fd, unsigned pos) HIDDEN;
void*     _dl_alloc_pages(l4_size_t size, l4_addr_t *phys,
			  const char *name) HIDDEN;
void      _dl_free_pages(void *addr, l4_size_t size) HIDDEN;
void      _dl_mmap_list_regions(int only_unseen);

#endif
