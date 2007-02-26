/**
 * \file	loader/server/src/elf-loader.h
 * \brief	
 *
 * \date	
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef __ELF_LOADER_H_
#define __ELF_LOADER_H_

const char * const interp;

int elf_map_binary(app_t *app);
int elf_map_ldso(app_t *app, l4_addr_t app_addr);

#endif
