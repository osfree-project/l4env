#ifndef __L4_EXEC_ELF32_DBG_H
#define __L4_EXEC_ELF32_DBG_H

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

