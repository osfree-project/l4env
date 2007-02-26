/*!
 * \file	loader/server/src/emu_tulip.h
 * \brief	experimental
 *
 * \date	2003
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef __EMU_TULIP_H_
#define __EMU_TULIP_H_

extern void tulip_init  (emu_desc_t *desc);
extern void tulip_handle(emu_desc_t *desc, emu_addr_t ea, l4_umword_t value, 
			 l4_umword_t *dw1, l4_umword_t *dw2, void **reply);
extern void tulip_spec  (emu_desc_t *desc, emu_addr_t ea, l4_umword_t value,
			 l4_umword_t *dw1, l4_umword_t *dw2, void **reply);

#endif

