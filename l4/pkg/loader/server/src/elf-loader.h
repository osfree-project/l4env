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

#define ELF_BADFORMAT       2001    /* invalid file format */
#define ELF_BADARCH         2002    /* invalid architecture */
#define ELF_CORRUPT         2003    /* file is corrupt */
#define ELF_NOSTANDARD      2004    /* defined entry point not found */
#define ELF_LINK            2005    /* errors while linking */
#define ELF_INTERPRETER     2006    /* contains INTERP section */

extern const char * const interp;

int elf_map_binary(app_t *app);
int elf_map_ldso(app_t *app, l4_addr_t app_addr);

int elf_check_ftype(const l4_addr_t img, const l4_size_t size,
                    const l4env_infopage_t *env);

#endif /* ! __ELF_LOADER_H_ */
