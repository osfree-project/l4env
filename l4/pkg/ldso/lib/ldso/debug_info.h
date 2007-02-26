/**
 * \file   ldso/lib/ldso/debug_info.h
 * \brief  Symbols support
 *
 * \date   01/2006
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef _DEBUG_INFO_H
#define _DEBUG_INFO_H

struct elf_resolve;

void _dl_debug_info_add(struct elf_resolve *tpnt, int infile,
			const char *header, unsigned long libaddr);
void _dl_debug_info_del(struct elf_resolve *tpnt);
void _dl_debug_info_sum(void);

#endif
