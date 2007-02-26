/*!
 * \file	loader/server/src/emu_ide.h
 * \brief	experimental
 *
 * \date	2003
 * \author	Frank Mehnert <<fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef __EMU_IDE_H_
#define __EMU_IDE_H_

#include "app.h"

extern void ide_init  (emu_desc_t *desc);
extern void ide_handle(emu_desc_t *desc, emu_addr_t ea, l4_umword_t value,
		       l4_umword_t *dw1, l4_umword_t *dw2, void **reply);

#endif

