/* $Id$ */
/**
 * \file 	exec/server/src/elf32.cc
 * \brief	ELF32 exec layer
 *
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the exec package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include <l4/env/errno.h>
#include <l4/sys/consts.h>
#include <l4/exec/elf.h>
#include <l4/exec/exec.h>
#include <l4/exec/errno.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/demangle/demangle.h>

#include <stdio.h>
#ifdef USE_OSKIT
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#include <string.h>

#include "elf.h"
#include "elf32.h"
#include "elf32_dbg.h"

#include "assert.h"
#include "config.h"
#include "debug.h"


elf32_obj_t::elf32_obj_t(exc_img_t *img, l4_uint32_t _id)
  : exc_obj_t(img, _id), dyn_hashtab(0), sym_strtab(0), sym_symtab(0)
{
}

elf32_obj_t::~elf32_obj_t()
{
  if (sym_symtab)
    free((void*)sym_symtab);
  if (sym_strtab)
    free((void*)sym_strtab);
}

/** Find the appropriate program section a symbol is associated with.
 * 
 * \param sym		ELF symbol descriptor
 * \param strtab	ELF symbol string table
 * \return		program section the symbol is associated with
 */
exc_obj_psec_t*
elf32_obj_t::sym_psec(Elf32_Sym *sym, const char *strtab)
{
  /* ELF standard, page 1-18: this member holds the relevant section
   * header table index. */
  if (sym->st_shndx != SHN_UNDEF)
    {
      exc_obj_hsec_t *hsec;

      return ((hsec = lookup_hsec(sym->st_shndx))) 
		    ? hsec->psec 
		    : 0;
    }
  
  msg("Symbol \"%s\" not associated with an EXC section",
	strtab + sym->st_name);
  return 0;
}


/** Load all dependend dynamic libraries.
 * 
 * \param env		L4 environment infopage
 * \return		0 on success
 * 			-L4_EXEC_BADFORMAT
 */
int
elf32_obj_t::load_libs(l4env_infopage_t *env)
{
  int i, error;
  Elf32_Dyn *dyn;
  exc_obj_t *exc_obj;
  exc_obj_hsec_t *hsec, *hsec_str;
  const char *strtab;

  if (flags & EO_LIBS_LOADED)
    return 0;

  Assert(hsecs);
  
  /* walk throug all dynamic sections */
  for (i=0; i<hsecs_num; i++)
    {
      hsec = hsecs + i;
      if (hsec->hs_type == SHT_DYNAMIC)
	{
	  /* ELF standard, page 1-13, figure 1-13:
	   * SHT_DYNAMIC section link is index to associated strtab sect */
	  if (!(hsec_str = lookup_hsec(hsec->hs_link)))
	    return -L4_EXEC_BADFORMAT;

	  strtab = (const char*)hsec_str->vaddr;
	  dyn    = (Elf32_Dyn*)hsec->vaddr;
	  
	  /* Add all first level DT_NEEDED entries (direct dependencies
	   * == entries of the binary object) to the dependency list */
	  for ( ; dyn->d_tag!=DT_NULL; dyn++)
	    {
	      if (dyn->d_tag==DT_NEEDED)
		{
		  /* New dependant library found. Try to create a new EXC
		   * object. If we still have that EXC object, don't create
		   * new object but get pointer to the existing object. */
		  int new_flags = (flags & (EO_LOAD_SYMBOLS | EO_LOAD_LINES));
		  const l4dm_dataspace_t inv_ds = L4DM_INVALID_DATASPACE;
		  if ((error = check(::exc_obj_load_bin(strtab+dyn->d_un.d_val,
							&inv_ds,
							/*force_load=*/0, 
							client_tid, 
							new_flags, &exc_obj,
							env),
				     "adding new elf object \"%s\"",
				     strtab+dyn->d_un.d_val)))
		    return error;
		  
		  /* add direct dependency to exc_obj */
		  add_to_dep(exc_obj);
		}
	    }
	}
    }

  flags |= EO_LIBS_LOADED;

  return 0;
}

/** Create ELF loadable sections. Connect file sections to dataspaces.
 * 
 * Here we need the environment page for the correct dm_id of each section.
 *
 * \param img		ELF image
 * \param env		L4 environment infopage
 * \return		0 on success */
int
elf32_obj_t::img_create_psecs(exc_img_t *img, l4env_infopage_t *env)
{
  int i, j;
  Elf32_Phdr *ph;
  Elf32_Ehdr *ehdr = (Elf32_Ehdr*)img->vaddr;
  Elf32_Phdr *phdr;
  l4_addr_t link_addr = 0;
  exc_obj_psec_t *psec;
  int first_section = 1;

  Assert(ehdr);

  phdr = (Elf32_Phdr*)(img->vaddr + ehdr->e_phoff);
  Assert(phdr);

  /* load program sections */
#ifdef DEBUG_PH_SECTIONS
  printf("=== ELF program sections: \"%s\" ===\n", get_fname());
#endif

  for (i=0, j=0; i<ehdr->e_phnum; i++)
    {
      psec = 0;
      ph = (Elf32_Phdr *)((l4_addr_t)phdr + i*ehdr->e_phentsize);

      if (ph->p_type == PT_LOAD)
	{
	  int type, sect_id, k, found;
	  l4_addr_t sec_beg, sec_end;

	  // L4 exec info
	  sec_beg = ph->p_vaddr;
	  sec_end = ph->p_memsz + sec_beg;
	  sec_beg = l4_trunc_page(sec_beg);
	  sec_end = l4_round_page(sec_end);

	  // check if this program section interfers
	  // with other program sections
	  found = 0;
	  for (k=0; k<j; k++)
	    {
	      l4_addr_t psec_beg, psec_end;
	      
	      psec = psecs[k];
	      psec_beg = psec->l4exc.addr;
	      psec_end = psec->l4exc.addr + psec->l4exc.size;
	      
	      if (   (sec_beg >= psec_beg || sec_end >= psec_beg)
		  && (sec_beg <  psec_end || sec_end <  psec_end))
		{
		  msg("Merging psec %08x-%08x with psec %08x-%08x",
		      sec_beg, sec_end, 
		      psec->l4exc.addr, psec->l4exc.addr+psec->l4exc.size);
		  if (sec_beg < psec_beg)
		    psec_beg = sec_beg;
		  if (sec_end > psec_end)
		    psec_end = sec_end;
		  psec->l4exc.addr = psec_beg;
		  psec->l4exc.size = psec_end - psec_beg;

		  // merge types
		  type = 0;
		  if (ph->p_flags & PF_R)
		    type |= L4_DSTYPE_READ;
		  if (ph->p_flags & PF_W) 
		    type |= L4_DSTYPE_WRITE;
		  if (ph->p_flags & PF_X)
		    type |= L4_DSTYPE_EXECUTE;
		  psec->l4exc.info.type |= type;

		  found = 1;
		  break;
		}
	    }
	  if (found)
	    continue;

	  if (j>=EXC_MAXPSECT)
	    {
	      msg("Too many (>%d) loadable program sections", j);
	      return -L4_EXEC_BADFORMAT;
	    }

	  type = 0;
	  sect_id = exc_obj_alloc_sect();

	  if (first_section)
	    {
	      /* first program section of an ELF object */
	      first_section = 0;
	      link_addr = l4_trunc_page(ph->p_vaddr);
	      type |= (L4_DSTYPE_LINKME | L4_DSTYPE_OBJ_BEGIN);
	    }

	  if (ph->p_flags & PF_R)
	    type |= L4_DSTYPE_READ;
	  if (ph->p_flags & PF_W) 
	    type |= L4_DSTYPE_WRITE;
	  if (ph->p_flags & PF_X)
	    type |= L4_DSTYPE_EXECUTE;
	  if (ehdr->e_type == ET_DYN)
	    type |= L4_DSTYPE_RELOCME;

	  if (!(type & L4_DSTYPE_RELOCME) && (sec_beg == 0))
	    {
	      /* Don't allow this because several functions assume
	       * that address==0 means not relocated */
	      msg("Binary section %d relocated to 0", i);
	      return -L4_EXEC_CORRUPT;
	    }

	  /* XXX until now we make every program section "copy on write".
	   * If we are sure that every object file is compiled with the
	   * -fPIC flag (position independant code), then we do not need
	   * to make text segments as "copy on write" */
	  if (flags & EO_SHARE)
	    type |= L4_DSTYPE_SHARE;

	  /* Each section must be paged by someone. After attached to a
	   * pager, this flag is cleared. */
	  type |= L4_DSTYPE_PAGEME;

	  /* Each program section must be reserved in the vm area of the
	   * application. After attaching/reserving to the applications
	   * region manager, this flag is cleared. 
	   * This flag is useless for old-style applications. */
	  type |= L4_DSTYPE_RESERVEME;

	  /* create program section and attach to our address space */
	  if (!(psec = new exc_obj_psec_t()))
	    return -L4_ENOMEM;

	  psec->link_addr = link_addr;
	  
	  psec->l4exc.addr = sec_beg;
	  psec->l4exc.size = sec_end-sec_beg;
	  psec->l4exc.info.id = sect_id;
	  psec->l4exc.info.type = type;

	  psecs[j++] = psec;
	}

#ifdef DEBUG_PH_SECTIONS
      elf32_img_show_program_section(ph, psec);
#endif

    }

  /* return number of sections */
  psecs_num = j;

  // allocate dataspaces
  for (i=0; i<j; i++)
    {
      
      psec = psecs[i];
      
      char dbg_name[32];
      
      sprintf(dbg_name, "psec%d ", i);
      strncat(dbg_name, get_fname(), sizeof(dbg_name)-strlen(dbg_name)-1);
      dbg_name[sizeof(dbg_name)-1] = '\0';

      l4_threadid_t dm_id = env->data_dm_id;
      
      if (psec->l4exc.info.type & L4_DSTYPE_EXECUTE)
	dm_id = env->text_dm_id;
      
      /* Create program section and attach to our address space.
       * If binary is direct mapped, open dataspace contiguous
       * from memory pool 1. If binary is also dynamic linkable, 
       * don't specify the address to open from */
      if ((psec->init_ds(psec->l4exc.addr, psec->l4exc.size, 
			 (flags & EO_DIRECT_MAP),
			 !(flags & EO_DYNAMIC),
			 dm_id, dbg_name)))
	return -L4_ENOMEM; /* don't need to delete psecs since they are
			      ref-counted */
    }

#ifdef DEBUG_PH_SECTIONS
  enter_kdebug("stop: ph_sections");;
#endif
  
  return 0;
}

/** Save all header sections of the ELF object because we need its data for
 * dynamic linking.
 *
 * \param img		ELF image
 * \return		0 on success */
int
elf32_obj_t::img_create_hsecs(exc_img_t *img)
{
  int i;
  Elf32_Ehdr *ehdr = (Elf32_Ehdr*)img->vaddr;
  Elf32_Shdr *sh;
  exc_obj_hsec_t *hsec;
  exc_obj_psec_t *psec;

  Assert(ehdr);

  hsecs_num = 0;

  /* save all section headers which allocate memory */
  for (i=0; i<ehdr->e_shnum; i++)
    {
      sh   = img_lookup_sh(i, img);
      hsec = hsecs + i;

      /* initialize as empty */
      if (i<EXC_MAXHSECT)
	{
	  hsec->vaddr   = 0;
	  hsec->psec    = 0;
	  hsec->hs_type = 0;
	  hsec->hs_link = 0;
	  hsec->hs_size = 0;
	}
      
      /* section allocates memory? */
      if (sh->sh_flags & SHF_ALLOC)
	{
	  /* sanity check */
	  if (i >= EXC_MAXHSECT)
	    {
	      msg("Too many (>%d) loadable header sections", i);
	      return -L4_EXEC_BADFORMAT;
	    }

	  if (!(psec = sh_psec(sh)))
	    {
	      Error("allocatable hsec %d at %08x not found in psecs!", 
		    i, sh->sh_addr);
	      return -L4_EXEC_CORRUPT;
	    }
	  
	  hsec->vaddr   = sh->sh_addr - psec->l4exc.addr + psec->vaddr;
	  hsec->psec    = psec;
	  hsec->hs_type = sh->sh_type;
	  hsec->hs_size = sh->sh_size;
	  hsec->hs_link = sh->sh_link;
	}
    }

  hsecs_num = i;
  
  return 0;
}

/** Free the memory the ELF header sections occupy if the ELF object has
 * no dynamic information included.
 *
 * We can do this because we need the header sections only for linking
 * and for grabbing symbols but linking is not neccessary if there are
 * no dynamic info.
 *
 * \param env		L4 environment infopage 
 * \return		0 on success */
int
elf32_obj_t::img_junk_hsecs_on_nodyn(l4env_infopage_t *env)
{
  if (!(flags & (EO_DYNAMIC | EO_LOAD_SYMBOLS)))
    {
      junk_hsecs();
      flags |= EO_LIBS_LOADED;
    }

  return 0;
}

/** Find the sections needed for dynamic linking of an ELF object.
 * 
 * Dynamic sections are not mandatory.
 *
 * Warning: We assume here, that the dynamic section is marked as an
 *          allocateable sections and so was loaded by img_fill_psecs()!
 *
 * \param img		ELF image
 * \return 		0 on success */
int
elf32_obj_t::img_save_info(exc_img_t *img)
{
  int i, j, s_link;
  Elf32_Ehdr *ehdr = (Elf32_Ehdr*)img->vaddr;
  exc_obj_hsec_t *hsec;

  Assert(ehdr);
  Assert(hsecs);

  for (i=0; i<hsecs_num; i++)
    {
      hsec = hsecs + i;
      if (hsec->hs_type == SHT_HASH)
	{
	  if (dyn_hashtab)
	    msg("WARNING: Has more than one hash table");

	  /* hash table found, store offset in our address space */
	  dyn_hashtab = (Elf32_Word*)hsec->vaddr;
	  
	  /* appropriate symbol table in our address space */
	  s_link = hsec->hs_link;
	  if (!(hsec = lookup_hsec(s_link)))
	    {
	      msg("Binary corrupt, section link %d=>%d not found", 
		  i, s_link);
	      return -L4_EXEC_CORRUPT;
	    }
	  dyn_symtab = (Elf32_Sym*)hsec->vaddr;
	  
	  /* appropriate symbol string table in our address space */
	  j = hsec - hsecs;
	  s_link = hsec->hs_link;
	  if (!(hsec = lookup_hsec(s_link)))
	    {
	      msg("Binary corrupt, section link %d=>%d not found", 
		  j, s_link);
	      return -L4_EXEC_CORRUPT;
	    }
	  dyn_strtab = (const char*)hsec->vaddr;

	  flags |= EO_DYNAMIC;
	}
    }

  /* warning only, dyn_hashtab is 0 */
  if (!dyn_symtab)
    msg("Has no dynamic info");
  
  return 0;
}

/** Create ELF loadable sections.
 * 
 * Connect file sections to dataspaces.
 *
 * \param img		ELF image
 * \return		0 on success */
int
elf32_obj_t::img_fill_psecs(exc_img_t *img)
{
  int i;
  Elf32_Shdr *sh;
  Elf32_Ehdr *ehdr = (Elf32_Ehdr*)img->vaddr;
  exc_obj_psec_t *psec;
  l4_addr_t target_addr;

  Assert(ehdr);

  for (i=0; i<ehdr->e_shnum; i++)
    {
      target_addr = 0;
      sh = img_lookup_sh(i, img);

      /* only sections with SHF_ALLOC needs space in memory */
      if (sh->sh_flags & SHF_ALLOC)
	{
	  /* find the appropriate ELF program section */
	  if (!(psec = sh_psec(sh)))
	    return -L4_EXEC_BADFORMAT;

	  if (sh->sh_size > 0)
	    {
	      target_addr = SH_ADDR_HERE(sh, psec);

	      if (sh->sh_type == SHT_NOBITS)
		{
		  /*  MEMSET (e.g. bss) */
		  memset((char*)target_addr, 0, sh->sh_size);
		}
	      else
		{
		  /* MEMCPY from file (text, rodata, data, ...)
		   * XXX We copy here, maybe we should map it
		   * once we have a dataspace manager in this server */
		  memcpy((char*)target_addr,
			 (char*)(img->vaddr + sh->sh_offset),
			 sh->sh_size);
		}
	    }
	}
    }

  /* set permissions of program sections to read only */
  for (i=0; i<psecs_num; i++)
    {
      int error;
      l4_uint32_t rights = L4DM_RO;
      
      psec = psecs[i];

      /* If section is not shared, make it writable if needed. Shared
       * sections are copy-on-write */
      if (  !(psec->l4exc.info.type & L4_DSTYPE_SHARE)
	  && (psec->l4exc.info.type & L4_DSTYPE_WRITE))
  	rights = L4DM_RW;
      
      if ((error = check(l4dm_share(&psec->l4exc.ds, client_tid, rights),
			"setting permissions of ds")))
	return 0;
    }

#ifdef DEBUG_SH_SECTIONS
  elf32_img_show_header_sections(this, img);
  enter_kdebug("stop: sh_sections");
#endif
#ifdef DEBUG_SYMTAB
  elf32_img_show_symtab(this, img);
  enter_kdebug("stop: symtab");
#endif

  return 0;
}

/** Save symbols of ELF object. */
int
elf32_obj_t::img_save_symbols(exc_img_t *img)
{
  int i;
  Elf32_Ehdr *ehdr = (Elf32_Ehdr*)img->vaddr;
  Elf32_Shdr *sh_sym, *sh_str;

  if (!(flags & EO_LOAD_SYMBOLS))
    return 0;

  for (i=0; i<ehdr->e_shnum; i++)
    {
      if (!(sh_sym = img_lookup_sh(i, img)))
	break;
      
      if (sh_sym->sh_type == SHT_SYMTAB)
	{
	  unsigned int sz_str, sz_sym;

	  if (!(sh_str = img_lookup_sh(sh_sym->sh_link, img)))
	    break;

	  sz_str = sh_str->sh_size;
	  sz_sym = sh_sym->sh_size;
	  
	  if (   !(sym_strtab = (const char*)malloc(sz_str))
	      || !(sym_symtab = (Elf32_Sym*)malloc(sz_sym)))
	    {
	      if (sym_strtab)
		{
		  free((void*)sym_strtab);
		  sym_strtab = 0;
		}
	      if (sym_symtab)
		{
		  free((void*)sym_symtab);
		  sym_symtab = 0;
		}
	      msg("Can't alloc %d bytes for symol information", sz_str+sz_sym);
	      break;
	    }

	  memcpy((void*)sym_strtab,
	         (void*)(img->vaddr + sh_str->sh_offset), sz_str);
	  memcpy((void*)sym_symtab, 
	         (void*)(img->vaddr + sh_sym->sh_offset), sz_sym);

	  num_symtab = sz_sym/sizeof(Elf32_Sym);

	  msg("Saved %d bytes of symbols", sz_str+sz_sym);

	  // well done
	  return 0;
	}
    }

  /* warning only, sym_symtab is 0 */
  msg("Has no symbols");
  return 0;
}
  
/** Save lines of ELF object. */
int
elf32_obj_t::img_save_lines(exc_img_t *img)
{
  int i, error;
  Elf32_Ehdr *ehdr = (Elf32_Ehdr*)img->vaddr;
  Elf32_Shdr *sh_sym, *sh_str;
  const char *strtab = 0;

  if (!(flags & EO_LOAD_LINES))
    return 0;

  if (   (ehdr->e_shstrndx == SHN_UNDEF)
      || (!(sh_str = img_lookup_sh(ehdr->e_shstrndx, img))))
    return 0;

  strtab = (const char*)(img->vaddr + sh_str->sh_offset);

  for (i=0; i<ehdr->e_shnum; i++)
    {
      if (!(sh_sym = img_lookup_sh(i, img)))
	break;
      
      if (   (   sh_sym->sh_type == SHT_PROGBITS
	      || sh_sym->sh_type == SHT_STRTAB)
	  && (!strcmp(sh_sym->sh_name + strtab, ".stab")))
	{
	  if (!(sh_str = img_lookup_sh(sh_sym->sh_link, img)))
	    break;

	  char dbg_name[32];
	  snprintf(dbg_name, sizeof(dbg_name), "stab %s", get_fname());

	  if ((stab = new exc_obj_stab_t(dbg_name)))
	    {
	      if ((error = stab->add_section(
		  (const stab_entry_t*)(img->vaddr + sh_sym->sh_offset),
		  (const char*)        (img->vaddr + sh_str->sh_offset),
		  sh_sym->sh_size/sh_sym->sh_entsize)))
		{
		  msg("Error %d adding stab section", error);
		  delete stab;
		  stab = 0;
		  break;
		}
	    }
	  else
	    {
	      msg("Can't create stab object");
	      break;
	    }
	}
    }

  if (!stab)
    msg("Has no lines");
  else
    stab->truncate();

  return 0;
}


/** Load ELF file and create appropriate dataspaces.
 *
 * \param img		ELF image
 * \param env		L4 environment infopage
 * \return		0 on success */
int
elf32_obj_t::img_copy(exc_img_t *img, l4env_infopage_t *env)
{
  int error;
  Elf32_Ehdr *ehdr = (Elf32_Ehdr*)img->vaddr;

  Assert(ehdr);

  /* program entry point */
  entry = (l4_addr_t)ehdr->e_entry;

#ifdef DEBUG_DY_SECTIONS
  if ((error = elf32_img_show_dynamic_sections(this, img)))
    return error;
  enter_kdebug("stop: dy_sections");
#endif
  
  if (/* create psecs according ELF objects program sections */
      (error = img_create_psecs(img, env)) ||

      /* save data from header sections because we need it for dyn. linking */
      (error = img_create_hsecs(img)) ||

      /* get necessary information so we can work without the file image */
      (error = img_save_info(img)) ||
      
      /* load header sections into appropriate psecs */
      (error = img_fill_psecs(img)) ||

      /* save symbol information for Fiasco kernel debugger */
      (error = img_save_symbols(img)) ||
     
      /* save symbol information for Fiasco kernel debugger */
      (error = img_save_lines(img)) ||
     
      /* if ELF object has no dynamic info, we do not need hsecs anymore */
      (error = img_junk_hsecs_on_nodyn(env)))

    ;

  return error;
}

/** Return the pointer to ELF header section with index i.
 * 
 * Works on the plain ELF image.
 *
 * \param i		header section index
 * \param img		ELF image
 * \returns		pointer to ELF header sections in ELF image */
Elf32_Shdr*
elf32_obj_t::img_lookup_sh(int i, exc_img_t *img)
{
  Elf32_Ehdr *ehdr = (Elf32_Ehdr*)img->vaddr;

  Assert(ehdr);

  /* sanity check */
  if (i >= ehdr->e_shnum)
    {
      msg("Want to access section %d (have %d)", i, ehdr->e_shnum);
      return 0;
    }
  
  return (Elf32_Shdr*)(img->vaddr + ehdr->e_shoff + i*ehdr->e_shentsize);
}

/** Find the symbol in the ELF object.
 * Search in dynamic symbol table
 * 
 * \param symname	symbol name to find
 * \param need_global	<>0 if symbol has to be global
 * \retval _sym		found symbol table entry 
 * \return		0 on success (symbol found)
 * 			1 symbol found but undefined (objdump says *UND*)
 * 			  and symbol is weak
 * 			2 symbol found and symbol is weak
 * 			-L4_ENOTFOUND if symbol not found or found but
 * 			  undefined (objdump says *UND*) */
int
elf32_obj_t::find_sym(const char *symname, int need_global, Elf32_Sym** _sym)
{
  int x, y;
  int nbucket, nchain;
  Elf32_Word *hashtab, *bucket, *chain;
  Elf32_Sym *sym;

  /* determine hash value */
  x = elf_hash((const unsigned char*)symname);

  /* search the symbol in all ELF libraries of binary object */
  hashtab = dyn_hashtab;
  nbucket = *hashtab++;
  nchain  = *hashtab++;
  bucket  = hashtab;
  chain   = bucket + nbucket;
  for (y = bucket[x % nbucket]; y ; y = chain[y])
    {
      if (y >= nchain)
	{
	  msg("ELF: y > nchain");
	  return -L4_EXEC_CORRUPT;
	}

      if (!strcmp(dyn_symbol(y), symname))
	{
	  /* symbol found */
	  sym = dyn_symtab + y;
	  /* if st_shndx is 0, then this symbol is UNDEFINED 
	   * therefore ignore it in this ELF object */
	  if (!sym->st_shndx)
	    {
	      if (ELF32_ST_BIND(sym->st_info)==STB_WEAK)
		return 1; /* symbol is weak */
	      else
		return -L4_ENOTFOUND; /* symbol found but undefined */
	    }
	  
	  /* return "not found" if the symbol is local bound
	   * but we need a global bound symbol */
	  if (need_global && ELF32_ST_BIND(sym->st_info)==STB_LOCAL)
	    return -L4_ENOTFOUND;

	  /* found */
	  *_sym = sym;
	  return (ELF32_ST_BIND(sym->st_info)==STB_WEAK)
			? 2 /* symbol found but weak */
			: 0 /* symbol found */;
	}
    }
  
  return -L4_ENOTFOUND; /* symbol not found */
}

/** Find the relocation address <reloc_addr> the address <addr> was relocated
 * in the context of the envpage <env>. */
int
elf32_obj_t::addr_to_reloc_offs(l4_addr_t addr, l4env_infopage_t *env,
				l4_addr_t *reloc_offs)
{
  exc_obj_psec_t *psec;
  l4exec_section_t *l4exc;
  l4_addr_t l4exc_reloc;
  
  if (/* 1st: find to which program section the address belongs to */
         !(psec = addr_psec(addr))
      /* 2nd: find the l4exec_section_t in the envpage the program section
       * belongs to */
      || !(l4exc = psec->lookup_env(env)) 
      /* 3rd: determine the relocation address of the program section */
      || !(l4exc_reloc = env_reloc_addr(l4exc, env)))
    {
      msg("Cannot find address %08x", addr);
      return -L4_EXEC_CORRUPT;
    }

  if (l4exc->info.type & L4_DSTYPE_RELOCME)
    {
      /* Symbol found but section not relocated yet */
      msg("ELF object %s section %d not relocated",
	  get_fname(), l4exc-env->section);
      return -L4_EXEC_BADFORMAT;
    }

  *reloc_offs = l4exc_reloc 
	      - psec->link_addr; /* => relative to the beginning of psec */
  return 0;
}

/** Find the relocation address <reloc_addr> the symbol <sym> was relocated
 * in the context of the envpage <env>. */
int
elf32_obj_t::sym_to_reloc_offs(Elf32_Sym *sym, const char *strtab,
			       l4env_infopage_t *env, l4_addr_t *reloc_offs)
{
  exc_obj_psec_t *psec;
  l4exec_section_t *l4exc;
  l4_addr_t l4exc_reloc;

  if (/* 1st: find to which program section the symbol belongs to */
         !(psec = sym_psec(sym, dyn_strtab))
      /* 2nd: find the l4exec_section_t in the envpage the program section
       * belongs to */
      || !(l4exc = psec->lookup_env(env))
      /* 3rd: determine the relocation address of the program section */
      || !(l4exc_reloc = env_reloc_addr(l4exc, env)))
    return -L4_EXEC_BADFORMAT;

  if (l4exc->info.type & L4_DSTYPE_RELOCME)
    {
      /* Symbol found but section not relocated yet */
      msg("ELF object %s section %d not relocated",
	  get_fname(), l4exc-env->section);
      return -L4_EXEC_BADFORMAT;
    }

  *reloc_offs = l4exc_reloc;
  return 0;
}

/** Determine the address of the symbol in our address space so that we
 * can change the content the symbol points to. */
int
elf32_obj_t::sym_vaddr(Elf32_Sym *sym, l4_addr_t *vaddr)
{
  exc_obj_psec_t *psec;

  /* find out to which program section the symbol belongs to */
  if (!(psec = sym_psec(sym, dyn_strtab)))
    return -L4_EXEC_BADFORMAT;

  *vaddr = psec->vaddr + sym->st_value - psec->link_addr;
  return 0;
}

/** Find the symbol in the relocated ELF library.
 * 
 * \param symname	name of symbol
 * \param env		L4 environment infopage
 * \retval addr		address of symbol
 * \return		0 on success
 * 			1 symbol found but undefined (objdump says *UND*)
 * 			  and symbol is weak
 * 			2 symbol found and symbol is weak
 * 			-L4_ENOTFOUND if symbol not found or found but
 * 			  undefined (objdump says *UND*)
 * 			-L4_EXEC_BADFORMAT */
int
elf32_obj_t::find_sym(const char *symname, l4env_infopage_t *env,
		      l4_addr_t *addr)
{
  int error;
  l4_addr_t reloc_offs;
  Elf32_Sym *sym;
  
  /* find global symbol in ELF object */
  if ((error = find_sym(symname, 1, &sym)))
    return error;

  /* find information about the symbol */
  if (sym_to_reloc_offs(sym, dyn_strtab, env, &reloc_offs))
    return -L4_EXEC_BADFORMAT;

  *addr = sym->st_value + reloc_offs;
  return 0;
}

/** Link relocation entry */
int
elf32_obj_t::link_sym(Elf32_Rel *rel, 		 /* relocation entry */
		      l4_addr_t rel_l4exc_vaddr, /* addr of relocation section
						  * in our address space */
		      exc_obj_psec_t *rel_psec,
    		      l4exec_section_t *rel_l4exc,
		      l4_addr_t rel_l4exc_reloc,
		      exc_obj_t **bin_deps,
		      l4env_infopage_t *env)
{
  int debug, ret;
  int rel_type;
  int sym_nr;			/* number of symbol in table (for debugging) */
  Elf32_Word *source;		/* address of symbol in our address space */
  Elf32_Word *target;		/* address (here) the patch should applied to */
  const char *symname;		/* ptr to name of symbol */
  Elf32_Sym *sym, *sym_weak;	/* target symbol rel entry is associated with */
  elf32_obj_t *sym_exc_obj;	/* exc_obj of target symbol of rel */
  elf32_obj_t *sym_exc_obj_weak;
  exc_obj_psec_t *psec;		/* exc_obj section of target of rel */
  l4exec_section_t *l4exc;	/* envpage section for target of rel */
  l4_addr_t l4exc_vaddr;	/* addr in our address space of target of rel */
  l4_addr_t l4exc_reloc;	/* relocated addr of area for target of rel */
  l4_addr_t reloc_offs;
  
  /* no debug info for next symbol */
#ifdef DEBUG_RELOC_ENTRIES
  debug = 1;
#else
  debug = 0;
#endif

  rel_type = ELF32_R_TYPE(rel->r_info);

  /* target address which to patch */
  target = (Elf32_Word*)(rel_l4exc_vaddr + rel->r_offset 
					 - rel_psec->l4exc.addr);

  if (   (l4_addr_t)target <  rel_l4exc_vaddr 
      || (l4_addr_t)target >= rel_l4exc_vaddr + rel_l4exc->size)
    {
      msg("target addr %p out of range %08x-%08x",
	   target, rel_l4exc_vaddr, rel_l4exc_vaddr+rel_l4exc->size);
      debug = 1;
    }

  if (debug)
    {
      elf32_obj_show_reloc_entry(rel, this, rel_l4exc);
      msg(" target addr %p = %08x + %08x - %08x",
	   target, rel_l4exc_vaddr, rel->r_offset, rel_psec->l4exc.addr);
    }

  /* Handle Relocation entry depending of the type */
  symname = dyn_symbol(ELF32_R_SYM(rel->r_info));
  if (!(rel_psec->l4exc.info.type & L4_DSTYPE_WRITE))
    msg("  in ro segment: \"%s\"", symname);

  switch (rel_type)
    {
#ifdef ARCH_x86
    case R_386_JMP_SLOT:
    case R_386_32:
    case R_386_PC32:
    case R_386_GLOB_DAT:
    case R_386_COPY:
#endif
#ifdef ARCH_arm
    case R_ARM_JUMP_SLOT:
    case R_ARM_ABS32:
    case R_ARM_REL32:
    case R_ARM_GLOB_DAT:
    case R_ARM_COPY:
#endif
      /* Dynamic linking: find symbol in binary and dependant libraries 
       * (if bin_obj!=0) or only in the exc_obj itself (if bin_obj==0) */
      if (bin_deps)
	{
	  sym              = 0;
	  sym_weak         = 0;
	  sym_exc_obj      = 0;
	  sym_exc_obj_weak = 0;
	  /* Search in binary/deps of the binary object */
	  for (elf32_obj_t **dep=(elf32_obj_t**)bin_deps; *dep; dep++)
	    {
	      /* Search symbol in object list. The symbol has to be defined 
	       * global if it is situated in another object than our's. */
	      ret = (*dep)->find_sym(symname, *dep != this, &sym);

	      if (ret == 0)
	       	{
		  /* symbol found */
		  sym_exc_obj = *dep;
	      	  break;
		}
	      else if ((ret == 1) && (*dep == this))
		{
		  /* Symbol found but is undefined and weak. It is
		   * also in our exc_obj so don't link it. */
		  msg("Symbol %s is weak and undefined", symname);
		  return 0;
		}
	      else if (ret == 2)
		{
		  /* Symbol found, not undefined but is weak. Save it
		   * so if we don't find another weak symbol, use this.
		   * Continue searching */
		  sym_exc_obj_weak = *dep;
		  sym_weak         = sym;
		}
	      /* else symbol not found so continue searching */
	    }
	  if (!sym_exc_obj)
	    {
	      if (sym_exc_obj_weak)
		{
		  /* we did not found another symbol which is not weak
		   * so use the weak symbol */
		  sym         = sym_weak;
		  sym_exc_obj = sym_exc_obj_weak;
		}
	      else
		{
		  /* binary warning */
		  bin_deps[0]->msg("Symbol %s NOT FOUND", symname);
		  rel_l4exc->info.type |= L4_DSTYPE_ERRLINK;
		  return 0; /* link errors are "weak" ... */
		}
	    }
	}
      else
	{
	  /* Search only in one EXC object, the symbol may
	   * be local or global */
	  if ((find_sym(symname, 0, &sym)))
	    {
	      msg("Symbol \"%s\" NOT FOUND", symname);
	      rel_l4exc->info.type |= L4_DSTYPE_ERRLINK;
	      return 0; /* link errors are weak ... */
	    }
	  sym_exc_obj = this;
	}

      /* Preserve absolute symbols as they are */
      if (sym->st_shndx == SHN_ABS)
	{
	  *target = sym->st_value;
	  break;
	}

      if (   !(psec = sym_exc_obj->sym_psec(sym, dyn_strtab)) 
	  || !(l4exc = psec->lookup_env(env)) 
	  || !(l4exc_reloc = env_reloc_addr(l4exc, env)))
	{
	  /* section of target not found */
	  msg("Section %d of symbol %s not found in target",
	      sym->st_shndx, sym->st_name + dyn_strtab);
	  return -L4_EXEC_CORRUPT;
	}

      if (l4exc->info.type & L4_DSTYPE_RELOCME)
	{
	  msg("ELF obj \"%s\"not yet relocated", sym_exc_obj->get_fname());
	  return -L4_EXEC_BADFORMAT;
	}

      sym_nr = sym-sym_exc_obj->dyn_symtab;

      switch (rel_type)
	{
#ifdef ARCH_x86
	case R_386_PC32:
#endif
#ifdef ARCH_arm
	case R_ARM_REL32:
#endif
	  /* patch relative */
	  *target += sym->st_value + l4exc_reloc
		   - rel->r_offset - rel_l4exc_reloc;

    	  if (debug)
	    printf("#%6d: write %08x "
		   "(%s, %08x, info %x)\n"
		   "       (added %08x + %08x - %08x - %08x)\n",
    		   sym_nr, *target, sym_exc_obj->get_fname(), 
		   sym->st_value, sym->st_info,
		   sym->st_value, l4exc_reloc,
		   rel->r_offset, rel_l4exc_reloc);
	  break;

#ifdef ARCH_x86
	case R_386_COPY:
#endif
#ifdef ARCH_arm
	case R_ARM_COPY:
#endif
	  /* copy data associated with shared object's symbol */
	  if (!(l4exc_vaddr = exc_obj_psec_here(env, l4exc-env->section)))
	    return -L4_EXEC_CORRUPT;

	  source = (Elf32_Word*)(l4exc_vaddr + sym->st_value 
					     - psec->l4exc.addr);
	  if (debug)
    	    {
	      printf(" target addr %p = %08x + %08x - %08x\n",
	       	  target, rel_l4exc_vaddr, rel->r_offset, rel_psec->l4exc.addr);
	      printf(" source addr %p = %08x + %08x - %08x\n",
		  source, l4exc_vaddr, sym->st_value,
		  psec->l4exc.addr);

	      printf("#%6d: *%p = *%p size %d "
		     "(%s, %08x, info %x)\n",
       		     sym_nr, target, source, sym->st_size,
		     sym_exc_obj->get_fname(), 
		     sym->st_value, sym->st_info);
	    }

	  memcpy(target, source, sym->st_size);
	  break;

	default:
    	  /* R_386_JMP_SLOT, R_386_32, R_386_GLOB_DAT */
	  /* patch absolute */
	  *target = l4exc_reloc + sym->st_value - psec->link_addr;
		      
	  if (debug)
	    printf("#%6d: write %08x (%s, %08x, info %x)\n"
		   "        (wrote %08x + %08x - %08x)\n",
     		   sym_nr, *target, sym_exc_obj->get_fname(), 
		   sym->st_value, sym->st_info,
		   l4exc_reloc, sym->st_value, psec->link_addr);
		      
	  break;
    	}
		  
      break;

#ifdef ARCH_x86
    case R_386_RELATIVE:
#endif
#ifdef ARCH_arm
    case R_ARM_RELATIVE:
#endif
      /* Dynamic linking: relative address */
      if (debug)
	printf("            *%p: %08x => ", target, *target);
      if (addr_to_reloc_offs((l4_addr_t)rel->r_offset, env, &reloc_offs))
	return -L4_EXEC_CORRUPT;
      
      if (debug)
	printf("%08x\n", *target + reloc_offs);
      
      /* patch (offset) */
      *target += reloc_offs;
      break;

    default:
      printf("    ==> how to deal? (type=%d, sym=%x)\n",
	  ELF32_R_TYPE(rel->r_info), ELF32_R_SYM (rel->r_info));
    }

  if (debug)
    enter_kdebug("stop");

  return 0;
}

/** Relocate/Link the entry */
int
elf32_obj_t::link_entry(l4env_infopage_t *env)
{
  l4_addr_t reloc_offs;

  if (addr_to_reloc_offs(entry, env, &reloc_offs))
    return -L4_EXEC_CORRUPT;

  msg("Relocating entry %08x => %08x", entry, entry+reloc_offs);
  env->entry_2nd = entry + reloc_offs;
  return 0;
}

/** Link ELF32 object.
 *
 * Search symbols in associated binary object libraries and  patch the
 * appropriate section.
 *
 * An exec section in the infopage holds the address and size where a
 * section lays in the target address space.
 * 
 * The symbols we try to link here can be determined by "objdump -R <file>"
 *
 * \param bin_deps	pointer to dependent ELF objects
 * \param env		L4 environment infopage
 * \return		0 on success
 * 			-L4_EINVAL */
int
elf32_obj_t::link(exc_obj_t **bin_deps, l4env_infopage_t *env)
{
  int i, cnt, error;
  exc_obj_hsec_t *hsec;
  l4exec_section_t *l4exc;        /* envpage section for target of rel */
  exc_obj_psec_t *rel_psec_cache; /* cache of rel_psec */

  /* rel_xxx refers to the section the relocation entry is situated in.
   * psec, l4exc, l4exc_addr refers to the sections the target symbol is
   * situated in. */

  /* check if exc object was already linked */
  if (!(l4exc = psecs[0]->lookup_env(env)))
    return -L4_EINVAL;
  
  if (!(l4exc->info.type & L4_DSTYPE_LINKME))
    return 0;

  if (!hsecs)
    {
      msg("Header sections already junked");
      return -L4_EINVAL;
    }

  msg("Linking");

  /* clear link flag */
  l4exc->info.type &= ~L4_DSTYPE_LINKME;

  /* Init cache of rel_psec. This should not be necessary because all
   * relocation entries of a relocation section should refer to the same
   * program section, but that seems to be not true anytime. */
  rel_psec_cache = 0;

  /* walk through all file sections searching for all relocation sections */
  for (i=0; i<hsecs_num; i++)
    {
      exc_obj_psec_t *rel_psec = 0;   /* exc_obj section of relocation entry */
      l4exec_section_t *rel_l4exc = 0;/* envpage section for rel entry sect */
      l4_addr_t rel_l4exc_vaddr = 0;  /* addr in our address space of rel sec */
      l4_addr_t rel_l4exc_reloc = 0;  /* relocated addr of area for rel entry */
      Elf32_Rel *rel;                 /* ptr to current relocation entry */

      hsec = hsecs + i;
      if (hsec->hs_type == SHT_REL)
	{
#ifdef DEBUG_RELOC_ENTRIES
	  printf("== %s: Relocation (REL) section (section %d, #%d)\n",
	          get_fname(), i, hsec->hs_size/sizeof(Elf32_Rel));
#endif
	  rel = (Elf32_Rel*)hsec->vaddr;

	  /* Walk through all relocation entries */
	  for (cnt=hsec->hs_size/sizeof(Elf32_Rel); cnt; rel++,cnt--)
	    {
	      // ensure that relocation entry belongs to same program section
	      if (!rel_psec || !rel_psec->contains(rel->r_offset))
		{
		  if (!(rel_psec = addr_psec(rel->r_offset)))
		    return -L4_EXEC_CORRUPT;

		  /* Count relocation entries in text segment (which prevent
		   * sharing of the section). In normal case there should not
		   * be any but if we has shared libraries which have non-PIC
		   * code ... */
		  if (!(rel_psec->l4exc.info.type & L4_DSTYPE_WRITE))
		    textreloc_num += hsec->hs_size/sizeof(Elf32_Rel);
		}

	      if (rel_psec != rel_psec_cache)
    		{
		  if (   !(rel_l4exc       = rel_psec->lookup_env(env))
		      || !(rel_l4exc_reloc = env_reloc_addr(rel_l4exc, env))
		      || !(rel_l4exc_vaddr = exc_obj_psec_here(env,
						   rel_l4exc - env->section)))
		    return -L4_EXEC_CORRUPT;
		}

	      if ((error = link_sym(rel, rel_l4exc_vaddr, rel_psec, rel_l4exc, 
				    rel_l4exc_reloc, bin_deps, env)))
		return error;
	    }
	}
    }

  /* In case we had to modity a text segment show warning */
  if (textreloc_num)
    msg("Has %d relocation entries in ro segment!", textreloc_num);

  /* Link entry point. This should only be needed in rare cases were we
   * boot relocateable binaries. */
  return link_entry(env);
}

int
elf32_obj_t::get_symbols(l4env_infopage_t *env, char **str)
{ 
  if (sym_strtab && sym_symtab)
    {
      const bool only_size = (*str == 0);
      unsigned int i;
      Elf32_Sym *sym;

      for (i=0, sym=sym_symtab; i<num_symtab; i++, sym++)
	{
	  exc_obj_psec_t *psec;
	  l4exec_section_t *l4exc;
	  l4_addr_t l4exc_reloc;
	  const char *s_name = sym_strtab + sym->st_name;
	  const char *d      = cplus_demangle(s_name, DMGL_ANSI | DMGL_PARAMS);
	  if (d)
	    s_name = d;

	  if (   (sym->st_shndx >= SHN_LORESERVE)   // ignore special symbols
       	      || (sym->st_value == 0)               // ignore NIL symbols
	      || (*s_name == '\0')                  // ignore unnamed symbols
	      || (!memcmp(s_name, "Letext", 6))     // ignore Letext symbols
	      || (!memcmp(s_name, "_stext", 6)))    // ignore _stext symbols
	    {
	      if (d)
		free((void*)d);
	      continue;
	    }

	  if (sym->st_shndx == SHN_UNDEF)
	    psec = addr_psec(sym->st_value);
	  else
	    psec = sym_psec(sym, sym_strtab);

	  if (psec)
	    {
	      if (!only_size)
		{
		  if (   !(l4exc = psec->lookup_env(env))
		      || !(l4exc_reloc = env_reloc_addr(l4exc, env)))
		    {
		      if (d)
			free((void*)d);
		      return -L4_EINVAL;
		    }

		  // really make symbol line
		  *str += sprintf(*str, "%08x   %.100s\n",
		                  l4exc_reloc+sym->st_value-psec->link_addr,
		                  s_name);
		}
	      else
		{
		  // only get size
		  int l = strlen(s_name);
		  if (l > 100)
		    l = 100;
		  *str += 11+l+1;
		}
	    }
	  else
	    {
	      // symbol points to a not allocated header section
	      // so ignore it silently
	    }
	  if (d)
	    free((void*)d);
	}
 
      *str++; // terminating '\0'
    }

  return 0;
}

int
elf32_obj_t::get_symbols_size(l4_size_t *sym_sz)
{
  char *len = 0;
  
  get_symbols(0, &len);
  *sym_sz = (l4_size_t)len;
  
  return 0;
}

int
elf32_obj_t::get_lines(l4env_infopage_t *env, char **str, stab_line_t **lin,
		       unsigned str_offs)
{
  stab_line_t *l, *l_end;
  
  if (!stab)
    return 0;
  
  l = *lin;
  stab->get_lines(str, lin);
  l_end = *lin;
 
  for (l_end = *lin; l<l_end; l++)
    {
      if (l->line < 0xfffd)
	{
	  /* relocate line information */
	  l4_addr_t reloc_offs;

	  if (!addr_to_reloc_offs(l->addr, env, &reloc_offs))
	    {
	      /* add relocation information */
	      l->addr += reloc_offs;
	    }
	  /* else section not found, ignore silently */
	}
      else
	{
	  /* add relative string field offset */
	  l->addr += str_offs;
	}
    }
  
  return 0;
}

int
elf32_obj_t::get_lines_size(l4_size_t *str_size, l4_size_t *lin_size)
{
  if (stab)
    return stab->get_size(str_size, lin_size);

  *str_size = 0;
  *lin_size = 0;
  
  return 0;
}

/** Check if we have a valid ELF32 binary image.
 * 
 * \param img		ELF image
 * \param env		L4 environment infopage
 * \param verbose	0=be quiet, 1=be verbose
 * \return		0 on success
 * 			-L4_EXEC_BADFORMAT
 * 			-L4_EXEC_CORRUPT */
int
elf32_obj_check_ftype(exc_img_t *img, l4env_infopage_t *env, int verbose)
{
  Elf32_Ehdr *ehdr = (Elf32_Ehdr*)img->vaddr;

  if (!ehdr)
    {
      if (verbose)
	img->msg("File not mapped");
      return -L4_EINVAL;
    }

  /* access ELF header */
  if (img->size < sizeof(Elf32_Ehdr))
    {
      if (verbose)
	img->msg("File too short");
      return -L4_EXEC_BADFORMAT;
    }

  /* sanity check for valid ELF header */
  if ((ehdr->e_ident[EI_MAG0] != ELFMAG0) ||
      (ehdr->e_ident[EI_MAG1] != ELFMAG1) ||
      (ehdr->e_ident[EI_MAG2] != ELFMAG2) ||
      (ehdr->e_ident[EI_MAG3] != ELFMAG3))
    {
      if (verbose)
	img->msg("Bad ELF header");
      return -L4_EXEC_BADFORMAT;
    }

  /* check for valid architecture */
  if ((ehdr->e_ident[EI_CLASS] != env->ver_info.arch_class) ||
      (ehdr->e_ident[EI_DATA]  != env->ver_info.arch_data)  ||
      (ehdr->e_machine         != env->ver_info.arch))
    {
      if (verbose)
	img->msg("Bad ELF architecture");
      return -L4_EXEC_BADFORMAT;
    }

  /* some more ELF sanity checks */
  if (img->size < ehdr->e_phoff + sizeof(Elf32_Phdr))
    {
      if (verbose)
	img->msg("File corrupt (phdr)");
      return -L4_EXEC_CORRUPT;
    }

  /* ELF sanity check */
  if (img->size < ehdr->e_phoff + ehdr->e_phnum*ehdr->e_phentsize - 1)
    {
      /* not enough space for all program sections */
      if (verbose)
	img->msg("File corrupt (ph entries)");
      return -L4_EXEC_CORRUPT;
    }

  /* ELF sanity check */
  if (img->size < ehdr->e_shoff + sizeof(Elf32_Shdr))
    {
      if (verbose)
	img->msg("File corrupt (shdr)");
      return -L4_EXEC_CORRUPT;
    }

  /* ELF sanity check */
  if (img->size < ehdr->e_shoff + ehdr->e_shnum*ehdr->e_shentsize - 1)
    {
      if (verbose)
	img->msg("File corrupt (sh entries)");
      return -L4_EXEC_CORRUPT;
    }

  return elf32_obj_check_interp(img, verbose);
}

/** Check if binary contains an interpreter section. 
 * Must be handled by the interpreter then, not by l4exec.
 *
 * \param img		ELF image
 * \return		0 if binary contains NO interp section
 * 			-L4_EXEC_INTERPRETER if binary contains interp sect */
int
elf32_obj_check_interp(exc_img_t *img, int verbose)
{
  int i, j;
  Elf32_Phdr *ph;
  Elf32_Ehdr *ehdr = (Elf32_Ehdr*)img->vaddr;
  Elf32_Phdr *phdr;

  Assert(ehdr);

  phdr = (Elf32_Phdr*)(img->vaddr + ehdr->e_phoff);
  Assert(phdr);

  /* walk through program sections */
  for (i=0, j=0; i<ehdr->e_phnum; i++)
    {
      ph = (Elf32_Phdr *)((l4_addr_t)phdr + i*ehdr->e_phentsize);

      if (ph->p_type == PT_INTERP)
	{
	  const char *interp = (const char*)(img->vaddr + ph->p_offset);
	  if (verbose)
	    img->msg("Interpreter section found, contains \"%s\"", interp);

	  /* binaries containing ld-linux.so are interpreted by l4exec */
	  if (!strcmp(interp, "libld-l4.s.so"))
	    return -L4_EXEC_INTERPRETER;
	}
    }

  return 0;
}

/** Create new ELF32 object if img points to a valid ELF32 file image.
 * 
 * \param img		ELF image
 * \param env		L4 environment infopage
 * \param id		internal id of exc object
 * \retval exc_obj	pointer to new created ELF object
 * \return		0 on success
 * 			-L4_EXEC_BADFORMAT
 * 			-L4_EXEC_CORRUPT */
int
elf32_obj_new(exc_img_t *img, exc_obj_t **exc_obj, l4env_infopage_t *env,
              l4_uint32_t id)
{
  int error;

  if ((error = (elf32_obj_check_ftype(img, env, /*verbose=*/1))) < 0)
    return error;

  /* found ELF32 object */
  if (!(*exc_obj = new elf32_obj_t(img, id))
      || (*exc_obj)->failed())
    return -L4_ENOMEM;

  return 0;
}
