/* $Id$ */
/**
 * \file	exec/server/src/elf32_dbg.cc
 * \brief	Some code for debugging code for analysis of ELF32 objects
 *
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the exec package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include <l4/util/elf.h>
#include <l4/exec/errno.h>

#include <stdio.h>

#include "elf32.h"
#include "elf32_dbg.h"

#include "config.h"
#include "debug.h"
#include "assert.h"

/** Show one file section entry
 *
 * \param ph		pointer to ELF32 program section
 * \param psec		pointer to associated exec object program section */
#ifdef DEBUG_PH_SECTIONS
void
elf32_img_show_program_section(Elf32_Phdr *ph, exc_obj_psec_t *psec)
{
  int type = ph->p_type;
  char *tstr, sa[32];
  char *lstr, la[16];
  static char *pn[] = {"PT_NULL   ","PT_LOAD   ","PT_DYNAMIC","PT_INTERP ",
		       "PT_NOTE   ","PT_SHLIB  ","PT_PHDR   "};
  if (type >=0 && type < (int)(sizeof(pn)/sizeof(char*)))
    tstr = pn[type];
  else if (type == 0x6474e551)
    tstr = "PT_STACK  ";
  else
    {
      sprintf(sa,"??? (%d)",type);
      tstr=sa;
    }
  if (!psec)
    lstr = "            ";
  else
    {
      sprintf(la,"==> %08lx", psec->vaddr);
      lstr=la;
    }
  printf("[%08x - %08x] %s (%s, %c%c%c, aln %x)\n",
         ph->p_vaddr, ph->p_vaddr+ph->p_memsz, lstr, tstr,
	 ph->p_flags & PF_X ? 'x' : '-',
	 ph->p_flags & PF_W ? 'w' : '-',
	 ph->p_flags & PF_R ? 'r' : '-',
	 ph->p_align);

}
#endif /* DEBUG_PH_SECTIONS */

#ifdef DEBUG_SYMTAB
void
elf32_img_show_symtab(elf32_obj_t *elf32_obj, exc_img_t *img)
{
  int i;
  Elf32_Ehdr *ehdr = (Elf32_Ehdr*)img->vaddr;
  Elf32_Shdr *sh;

  for (i=0; i<ehdr->e_shnum; i++)
    {
      if (!(sh = elf32_obj->img_lookup_sh(i, img)))
	return;
      if (sh->sh_type == SHT_SYMTAB)
	{
	  unsigned entry, entries = sh->sh_size/sh->sh_entsize;
	  Elf32_Sym *symtab = (Elf32_Sym*)(img->vaddr + sh->sh_offset);
	  Elf32_Shdr *sh_link;
	  const char *strtab;

	  if (!(sh_link = elf32_obj->img_lookup_sh(sh->sh_link, img)))
	    return;

	  strtab = (const char*)(img->vaddr + sh_link->sh_offset);

	  printf("Symbol table has %d symbols\n", entries);
	  for (entry=0; entry<entries; entry++)
	    {
	      printf("%08x: (%08x) %s\n",
		  symtab[entry].st_value, 
		  symtab[entry].st_name,
		  strtab + symtab[entry].st_name);
	    }
	}
    }
}
#endif

/** Show one section header table entry
 *
 * \param sh		pointer to ELF32 header section
 * \param strtab	pointer to ELF32 string table
 * \param elf32_obj	pointer to ELF32 object */
#ifdef DEBUG_SH_SECTIONS
static void
elf32_img_show_header_section(Elf32_Shdr *sh, const char *strtab,
			      elf32_obj_t *elf32_obj)
{
  int type = sh->sh_type;
  char *tstr, sa[32];
  const char *nstr;
  static char *sn[] = {"SHT_NULL","SHT_PROGBITS","SHT_SYMTAB","SHT_STRTAB",
      "SHT_RELA","SHT_HASH","SHT_DYNAMIC","SHT_NOTE","SHT_NOBITS","SHT_REL",
      "SHT_SHLIB","SHT_DYNSYM"};

  if (type != SHT_NULL)
    {
      if (type >=0 && type < (int)(sizeof(sn)/sizeof(char*)))
	tstr = sn[type];
      else
	{
	  sprintf(sa,"%08x",type);
	  tstr=sa;
	}
      if (strtab)
	nstr = strtab + sh->sh_name;
      else
	nstr = "(sh_names?)";

      if (sh->sh_addr)
	{
	  if (sh->sh_flags & SHF_ALLOC)
	    {
	      l4_addr_t vaddr;
	      exc_obj_psec_t *psec;

	      if (!(psec = elf32_obj->sh_psec(sh)))
		return;
	      vaddr = SH_ADDR_HERE(sh, psec);

	      printf("[%08x - %08x] => %08lx: %s (%s, aln %x)\n",
		     sh->sh_addr, sh->sh_addr+sh->sh_size, vaddr,
		     nstr, tstr, sh->sh_addralign);
	    }
	  else
	    printf("[%08x - %08x]            : %s (%s, aln %x)\n",
		   sh->sh_addr, sh->sh_addr+sh->sh_size,
		   nstr, tstr, sh->sh_addralign);
	}
      else
	{
	  printf("(%08x           )            : %s (%s, aln %x)\n",
	         sh->sh_offset,
		 nstr, tstr, sh->sh_addralign);
	}
    }
}
#endif

/** Show one section header table entry
 *
 * \param elf32_obj	pointer to ELF32 object
 * \param img		pointer to ELF image */
#ifdef DEBUG_SH_SECTIONS
void
elf32_img_show_header_sections(elf32_obj_t *elf32_obj, exc_img_t *img)
{
  int i;
  const char *strtab = (char*)NULL;
  Elf32_Ehdr *ehdr = (Elf32_Ehdr*)img->vaddr;
  Elf32_Shdr *sh;

  Assert(ehdr);

  /* section header name table */
  if (ehdr->e_shstrndx != SHN_UNDEF)
    {
      if (!(sh = elf32_obj->img_lookup_sh(ehdr->e_shstrndx, img)))
	return;
      strtab = (const char*)(img->vaddr + sh->sh_offset);
    }

  printf("=== ELF header sections ===\n");
  for (i=0; i<ehdr->e_shnum; i++)
    {
      if (!(sh = elf32_obj->img_lookup_sh(i, img)))
	return;
      elf32_img_show_header_section(sh, strtab, elf32_obj);
    }
}
#endif /* DEBUG_SH_SECTIONS */

#ifdef DEBUG_DY_SECTIONS
/** Show one dynamic section table entry
 *
 * \param dyn		pointer to the dynamic section table entry
 * \param strtab	pointer to string table */
static void
elf32_img_show_dynamic_section(Elf32_Dyn *dyn, const char *strtab)
{
  int type = dyn->d_tag;
  char *dstr, sa[32];
  const char *sstr;
  static char *dt[] = {"DT_NULL","DT_NEEDED","DT_PLTRELSZ","DT_PLTGOT",
      "DT_HASH","DT_STRTAB","DT_SYMTAB","DT_RELA","DT_RELASZ","DT_RELAENT",
      "DT_STRSZ","DT_SYMENT","DT_INIT","DT_FINI","DT_SONAME","DT_RPATH",
      "DT_SYMBOLIC","DT_REL","DT_RELSZ","DT_RELENT","DT_PLTREL","DT_DEBUG",
      "DT_TEXTREL","DT_JMPREL"};
  if (type >=0 && type < (int)(sizeof(dt)/sizeof(char*)))
    dstr = dt[type];
  else
    {
      sprintf(sa,"unknown (%d)",type);
      dstr=sa;
    }
  switch (type)
    {
    case DT_NEEDED:
      sstr = "";
      if (strtab)
	sstr = strtab + dyn->d_un.d_val;
      printf("%s %s\n", dstr, sstr);
      break;
    }
}
#endif /* DEBUG_DY_SECTIONS */

#ifdef DEBUG_DY_SECTIONS
/** Show all dynamic section table entries.
 *
 * \param elf32_obj	pointer to ELF32 object
 * \param img		pointer to ELF image */
int
elf32_img_show_dynamic_sections(elf32_obj_t *elf32_obj, exc_img_t *img)
{
  Elf32_Shdr *sh, *sh_str;
  Elf32_Ehdr *ehdr = (Elf32_Ehdr*)img->vaddr;

  Assert(ehdr);

  /* search the dynamic section */
  for (int i=0; i<ehdr->e_shnum; i++)
    {
      sh = elf32_obj->img_lookup_sh(i, img);
      if (sh->sh_type == SHT_DYNAMIC)
	{
	  printf("=== ELF dynamic section ===\n");
	  /* ELF standard, page 1-13, figure 1-13:
	   * SHT_DYNAMIC section link is index to assciated strtab section */
	  if (!(sh_str = elf32_obj->img_lookup_sh(sh->sh_link, img)))
	    return -L4_EXEC_BADFORMAT;

	  const char *strtab = (const char*)(img->vaddr + sh_str->sh_offset);
	  Elf32_Dyn *dyn = (Elf32_Dyn*)(img->vaddr + sh->sh_offset);
	  for ( ; dyn->d_tag!=DT_NULL; dyn++)
	    elf32_img_show_dynamic_section(dyn, strtab);
	}
    }

  return 0;
}
#endif /* DEBUG_DY_SECTIONS */

/** Show Elf32 relocation entry
 *
 * \param rel		pointer to relocation entry
 * \param elf32_obj	pointer to ELF32 object
 * \param l4exc		pointer to exec section in the environent info page */
void
elf32_obj_show_reloc_entry(Elf32_Rel *rel, elf32_obj_t *elf32_obj,
		           l4exec_section_t *l4exc)
{
  char *rstr, sa[32];
  static char *rt[] = {"R_386_NONE    ","R_386_32      ","R_386_PC32    ",
      "R_386_GOT32   ","R_386_PLT32   ","R_386_COPY    ","R_386_GLOB_DAT",
      "R_386_JMP_SLOT","R_386_RELATIVE","R_386_GOTOFF  ","R_386_GOTPC   "};
  int type;

  type = ELF32_R_TYPE(rel->r_info);
  if (type >=0 && type < (int)(sizeof(rt)/sizeof(char*)))
    rstr = rt[type];
  else
    {
      sprintf(sa,"  unknown (%02d)",type);
      rstr=sa;
    }
  printf("%s %08x %s\n",
      rstr, rel->r_offset,
      elf32_obj->dyn_symbol(ELF32_R_SYM(rel->r_info)));
}
