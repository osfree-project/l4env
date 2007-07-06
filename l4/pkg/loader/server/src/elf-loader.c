/* $Id$ */
/**
 * \file	loader/server/src/elf-loader.c
 * \brief	simple ELF interpreter
 *
 * \date	05/10/2005
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <assert.h>
#include <stdio.h>
#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/env/errno.h>
#include <l4/env/env.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/util/elf.h>

#include "app.h"
#include "dm-if.h"
#include "elf-loader.h"
#include "fprov-if.h"

//#define DEBUG
const char * const interp = "libld-l4.s.so";

static int
elf_map(app_t *app, l4_addr_t image, l4_addr_t base, l4_addr_t *entry,
	l4_uint32_t *save_phnum, l4_addr_t *save_phdr, int load_hdrs)
{
  ElfW(Ehdr) *ehdr = (ElfW(Ehdr*))image;
  ElfW(Phdr) *phdr;
  l4env_infopage_t *env = app->env;
  l4exec_section_t *l4exc;
  l4exec_section_t section[8];
  void *map_addr;
  l4_addr_t map_offs;
  int i, j, error;
  unsigned section_num = 0;
  int ehdr_loaded = 0, phdr_loaded = 0;

  assert(ehdr);

  *entry = ehdr->e_entry + base;

  phdr = (ElfW(Phdr*))(image + ehdr->e_phoff);
  assert(phdr);

  for (i=0; i<ehdr->e_phnum; i++)
    {
      ElfW(Phdr) *ph = (ElfW(Phdr *))((l4_addr_t)phdr + i*ehdr->e_phentsize);

      if (ph->p_type == PT_PHDR ||
	  ph->p_type == PT_LOAD ||
	  ph->p_type == PT_INTERP ||
	  ph->p_type == PT_DYNAMIC)
	{
	  const char *name = ph->p_type == PT_PHDR
				? "pt_phdr"
				: ph->p_type == PT_LOAD
					? "pt_load"
					: ph->p_type == PT_INTERP
                                                ? "pt_interp"
                                                : "pt_dynamic";

	  l4_addr_t beg  = l4_trunc_page(ph->p_vaddr);
	  l4_addr_t end  = l4_round_page(ph->p_vaddr+ph->p_memsz);
	  l4_size_t size = end - beg;
#ifdef DEBUG
	  app_msg(app, "sec%02d %08lx-%08lx (%s)", i, beg, end, name);
#endif

	  if (size == 0)
	    {
	      app_msg(app, "Skipping empty section sec%02d at %08lx (%s)",
                           i, beg, name);
	      continue;
	    }

	  if (ph->p_type == PT_PHDR)
	    {
	      if (save_phnum)
		*save_phnum = ehdr->e_phnum;
	      if (save_phdr)
		*save_phdr = base + ph->p_vaddr;
	    }

	  /* check for overlap */
	  for (j=0, l4exc=section; j<section_num; j++, l4exc++)
	    {
	      l4_addr_t sec_beg  = l4exc->addr;
	      l4_size_t sec_size = l4exc->size;
	      l4_addr_t sec_end  = sec_beg + sec_size;

	      if ((beg >= sec_beg || end >= sec_beg) &&
		  (beg <  sec_end || end <  sec_end))
		{
		  l4_size_t new_size, add_beg, add_end;

#ifdef DEBUG
		  app_msg(app, "Merge %08lx-%08lx / %08lx-%08lx",
			        beg, end, sec_beg, sec_end);
#endif

                  add_beg  = beg < sec_beg ? sec_beg - beg : 0;
                  add_end  = end > sec_end ? end - sec_end : 0;
                  new_size = add_beg + sec_size + add_end;

                  if (new_size > sec_size)
                    {
                      if ((error = l4dm_mem_resize(&l4exc->ds, new_size)))
                        {
                          app_msg(app, "Error %d resizing ds to %zdkB",
			          error, new_size/1024);
                          return error;
                        }
                    }
                  else
                    new_size = sec_size;

		  if ((error = l4rm_attach(&l4exc->ds, new_size,
					   0, L4RM_MAP|L4DM_RW, &map_addr)))
		    {
		      app_msg(app, "Error %d attaching ds size %zdkB",
				   error, new_size/1024);
		      return error;
		    }
		  if (add_beg)
		    memmove(map_addr+add_beg, map_addr, sec_size);
		  if (add_beg)
		    memset(map_addr, 0, add_beg);
		  if (add_end)
		    memset(map_addr+add_beg+sec_size, 0, add_end);

		  l4exc->addr -= add_beg;
		  l4exc->size += add_beg + add_end;

		  /* merge type */
		  l4exc->info.type |=
				(ph->p_flags & PF_R ? L4_DSTYPE_READ    : 0) |
				(ph->p_flags & PF_W ? L4_DSTYPE_WRITE   : 0) |
				(ph->p_flags & PF_X ? L4_DSTYPE_EXECUTE : 0);

		  map_offs = ph->p_vaddr-l4exc->addr;
		  goto next_iter;
		}
	    }

	  /* cannot merge, open new section */
	  if ((error = l4dm_mem_open(env->memserv_id, size,
				     0, 0, name, &l4exc->ds)))
	    {
	      app_msg(app, "Error %d allocating memory for program headers",
			    error);
	      return error;
	    }

	  l4exc->addr      = beg;
	  l4exc->size      = size;
	  l4exc->info.id   = 0;
	  l4exc->info.type = (ph->p_flags & PF_R ? L4_DSTYPE_READ    : 0)
			   | (ph->p_flags & PF_W ? L4_DSTYPE_WRITE   : 0)
			   | (ph->p_flags & PF_X ? L4_DSTYPE_EXECUTE : 0);

	  if (app->flags & APP_ALL_WRITBLE)
	    l4exc->info.type |= L4_DSTYPE_WRITE;

	  if ((error = l4rm_attach(&l4exc->ds, l4exc->size, 0,
				   L4RM_MAP|L4DM_RW, &map_addr)) < 0)
	    {
	      app_msg(app, "Error %d attaching ds size %zdkB",
			   error, l4exc->size/1024);
	      return error;
	    }

	  /* zero memory */
	  memset(map_addr, 0, l4exc->size);
	  map_offs = ph->p_vaddr-l4exc->addr;

	  l4exc++;
	  section_num++;

next_iter:
	  /* copy content of program section into dataspace */
	  memcpy(map_addr+map_offs, (char*)image+ph->p_offset, ph->p_filesz);

          /* XXX Copy ELF header and program sections. The ldso interpreter
           *     needs both structures for linking. If I would find a way
           *     to explicitely add these structions to the ldso binary we
           *     would not need this hack. */
          if (load_hdrs)
            {
              if (beg        <= 0 &&
                  beg + size >= sizeof(ElfW(Ehdr)))
                {
                  memcpy(map_addr+0, (char*)image, sizeof(ElfW(Ehdr)));
                  ehdr_loaded = 1;
                }
              if (beg        <= ehdr->e_phoff &&
                  beg + size >= ehdr->e_phoff + ehdr->e_phnum *
                                                sizeof(ElfW(Phdr)))
                {
                  memcpy(map_addr+ehdr->e_phoff, (char*)image+ehdr->e_phoff,
                         ehdr->e_phnum * sizeof(ElfW(Phdr)));
                  phdr_loaded = 1;
                }
            }

	  if ((error = l4rm_detach(map_addr)))
	    {
	      app_msg(app, "Error %d detaching ds", error);
	      return error;
	    }
	}
    }

  /* sanity check */
  if (env->section_num + section_num > L4ENV_MAXSECT)
    {
      app_msg(app, "Cannot pass sections to application");
      return -L4_ENOMEM;
    }

  if (load_hdrs && (!ehdr_loaded || !phdr_loaded))
    {
      app_msg(app, "Cannot allocate phdr for ehdr/phdr");
      return -L4_ENOMEM;
    }

  /* attach all allocated dataspaces to our pager */
  for (i=0, l4exc=section; i<section_num; i++, l4exc++)
    {
      app_area_t *aa;

      /* fill in section in L4env infopage */
      l4exc->addr += base;
      memcpy(env->section+env->section_num, l4exc, sizeof(*l4exc));
      env->section_num++;

      if ((error = app_attach_ds_to_pager(app,
					  &l4exc->ds, l4exc->addr,
					  l4exc->size, l4exc->info.type,
					  l4exc->info.type & L4_DSTYPE_WRITE
						  ? L4DM_RW
						  : L4DM_RO,
					  /*attach=*/0,
					  "program section", &aa)))
	return error;
    }

  return 0;
}


/** Load program sections of binary which must be interpreted by libld-l4.s.so.
 * \param app		application
 * \return		0 if success */
int
elf_map_binary(app_t *app)
{
  l4env_infopage_t *env = app->env;

  app_msg(app, "Loading binary");
  return elf_map(app, app->image, /*base=*/0,
		 &env->entry_2nd, &env->phnum, &env->phdr, 0);
}


/** Load the libld-l4.s.so interpreter.
 * \param app		application
 * \param app_addr	load address inside application
 * \return		0 on success */
int
elf_map_ldso(app_t *app, l4_addr_t app_addr)
{
  l4_addr_t addr;
  l4_size_t size;
  l4env_infopage_t *env = app->env;
  l4dm_dataspace_t ds;
  int error;

  if ((error = load_file(interp,
			 env->fprov_id, env->memserv_id,
			 /*search_path=*/cfg_libpath, /*contiguous=*/0,
			 &addr, &size, &ds) < 0))
    {
      app_msg(app, "Cannot load %s", interp);
      return error;
    }

  app_msg(app, "Loading ldso");
  if ((error = elf_map(app, addr, /*base=*/app_addr, &env->entry_1st, 0, 0, 1)))
    return error;

  junk_ds(&ds, addr);
  return 0;
}

/**
 * \param img     Pointer to binary image in memory
 * \param size    Size of image
 * \param env     L4Env info page
 *
 * \return 0                 Image is ELF for given architecture
 *         -ELF_INTERPRETER  Found an interpreter section.
 *
 *         -L4_EINVAL        Invalid input data,
 *                             or invalid interpreter detected
 *         -ELF_BADFORMAT    Image is not ELF for given arch
 *         -ELF_CORRUPT      Image is corrupt
 */
int
elf_check_ftype(const l4_addr_t img, const l4_size_t size,
                const l4env_infopage_t *env)
{
  ElfW(Ehdr) *ehdr = (ElfW(Ehdr*))img;
  ElfW(Phdr) *phdr;
  ElfW(Phdr) *ph;
  int i, j;

  if (!ehdr)
    return -L4_EINVAL;

  /* access ELF header */
  if (size < sizeof(ElfW(Ehdr)))
    return -ELF_BADFORMAT;

  /* sanity check for valid ELF header */
  if ((ehdr->e_ident[EI_MAG0] != ELFMAG0) ||
      (ehdr->e_ident[EI_MAG1] != ELFMAG1) ||
      (ehdr->e_ident[EI_MAG2] != ELFMAG2) ||
      (ehdr->e_ident[EI_MAG3] != ELFMAG3))
    return -ELF_BADFORMAT;

  if ((ehdr->e_ident[EI_CLASS] != env->ver_info.arch_class) ||
      (ehdr->e_ident[EI_DATA]  != env->ver_info.arch_data)  ||
      (ehdr->e_machine         != env->ver_info.arch))
    return -ELF_BADFORMAT;

  /* some more ELF sanity checks */
  if (size < ehdr->e_phoff + sizeof(ElfW(Phdr)))
    return -ELF_CORRUPT;

  /* ELF sanity check */
  if (size < ehdr->e_phoff + ehdr->e_phnum*ehdr->e_phentsize - 1)
    /* not enough space for all program sections */
    return -ELF_CORRUPT;

  /* ELF sanity check */
  if (size < ehdr->e_shoff + sizeof(ElfW(Shdr)))
    return -ELF_CORRUPT;

  /* ELF sanity check */
  if (size < ehdr->e_shoff + ehdr->e_shnum*ehdr->e_shentsize - 1)
    return -ELF_CORRUPT;

  /* Check interpreter */
  phdr = (ElfW(Phdr*))(img + ehdr->e_phoff);
  if (!phdr)
    return -ELF_CORRUPT;

  for (i = 0, j = 0; i < ehdr->e_phnum; i++)
    {
      ph = (ElfW(Phdr *))((l4_addr_t)phdr + i*ehdr->e_phentsize);

      if (ph->p_type == PT_INTERP)
	{
	  const char *interp = (const char*)(img + ph->p_offset);

	  if (strcmp(interp, "libld-l4.s.so"))
	    {
	      printf("Invalid interpreter found: %s", interp);
	      return -L4_EINVAL;
	    }

	  /* Indicate that we have found an interpreter section. */
	  return -ELF_INTERPRETER;
	}
    }

  return 0;
}
