/*!
 * \file	elf32_dbg.h
 * \brief	
 *
 * \date	2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the exec package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef __ELF32_DBG_H_
#define __ELF32_DBG_H_

void
elf32_img_show_program_section(Elf32_Phdr *ph, exc_obj_psec_t *psec);

void
elf32_img_show_header_sections(elf32_obj_t *elf32_obj, exc_img_t *img);

int
elf32_img_show_dynamic_sections(elf32_obj_t *elf32_obj, exc_img_t *img);

void
elf32_img_show_symtab(elf32_obj_t *elf32_obj, exc_img_t *img);

void
elf32_obj_show_reloc_entry(Elf32_Rel *rel, elf32_obj_t *elf32_obj,
			   l4exec_section_t *l4exc);

#endif /* __L4_EXEC_ELF32_DBG_H */

