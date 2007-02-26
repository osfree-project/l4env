/*!
 * \file	loader/server/src/emu_speedo.h
 * \brief	experimental
 *
 * \date	05/27/2003
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef __EMU_SPEEDO_H_
#define __EMU_SPEEDO_H_

extern void speedo_init  (emu_desc_t *desc);
extern void speedo_handle(emu_desc_t *desc, emu_addr_t ea, l4_umword_t value, 
			  l4_umword_t *dw1, l4_umword_t *dw2, void **reply);
extern void speedo_spec  (emu_desc_t *desc, emu_addr_t ea, l4_umword_t value,
			  l4_umword_t *dw1, l4_umword_t *dw2, void **reply);

#endif

