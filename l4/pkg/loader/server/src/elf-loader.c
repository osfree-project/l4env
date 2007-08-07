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
  struct
  {
    l4_addr_t beg;
    l4_addr_t end;
  } s[32];
  ElfW(Ehdr) *ehdr = (ElfW(Ehdr*))image;
  ElfW(Phdr) *phdr;
  l4env_infopage_t *env = app->env;
  l4exec_section_t *l4exc;
  l4exec_section_t section[16];
  void *map_addr, *section_map[16];
  l4_addr_t map_offs;
  unsigned s_num, section_num = 0;
  int i, j, error = 0, ehdr_loaded = 0, phdr_loaded = 0;

  memset(section_map, 0, sizeof(section_map));

  assert(ehdr);
  *entry = ehdr->e_entry + base;

  phdr = (ElfW(Phdr*))(image + ehdr->e_phoff);
  assert(phdr);

  /* at first scan all input sections to determine which program sections we
   * have to create */
  for (i = 0, s_num = 0; i < ehdr->e_phnum; i++)
    {
      ElfW(Phdr) *ph = (ElfW(Phdr *))((l4_addr_t)phdr + i*ehdr->e_phentsize);

      if (ph->p_type == PT_PHDR ||
	  ph->p_type == PT_LOAD ||
	  ph->p_type == PT_INTERP ||
	  ph->p_type == PT_DYNAMIC)
	{
          s[s_num].beg  = l4_trunc_page(ph->p_vaddr);
          s[s_num].end  = l4_round_page(ph->p_vaddr+ph->p_memsz);
          s_num++;
        }
    }

  /* now merge overlapping sections */
restart:
  for (i = 0; i < s_num; i++)
    {
      if (s[i].end)
        {
          for (j = 0; j < s_num; j++)
            {
              if (i != j &&
                  s[j].end &&
                  (s[i].beg >= s[j].beg || s[i].end >= s[j].beg) &&
                  (s[i].beg <  s[j].end || s[i].end <  s[j].end))
                {
                  s[j].beg = s[i].beg < s[j].beg ? s[i].beg : s[j].beg;
                  s[j].end = s[i].end > s[j].end ? s[i].end : s[j].end;
                  s[i].end = 0;
                  goto restart;
                }
            }
        }
    }

  /* now actually create the sections */
  for (i = 0, l4exc = section; i < s_num; i++)
    {
      if (s[i].end)
        {
          l4exc->addr = s[i].beg;
          l4exc->size = s[i].end - s[i].beg;
	  if ((error = l4dm_mem_open(env->memserv_id, l4exc->size,
				     0, 0, "program section", &l4exc->ds)))
	    {
	      app_msg(app, "Error %d allocating %zdMB memory for program headers",
			    error, (l4exc->size + (1<<19) - 1) / (1 << 20));
              goto cleanup;
            }
	  if ((error = l4rm_attach(&l4exc->ds, l4exc->size, 0,
				   L4RM_MAP|L4DM_RW, &map_addr)) < 0)
	    {
	      app_msg(app, "Error %d attaching ds size %zdkB",
			   error, l4exc->size/1024);
	      goto cleanup;
	    }
          l4exc->info.id   = 0;
          l4exc->info.type = 0;
	  memset(map_addr, 0, l4exc->size);
          section_map[l4exc-section] = map_addr;
          l4exc++;
        }
    }
  section_num = l4exc - section;

  /* fill the pre-created sections */
  for (i = 0; i < ehdr->e_phnum; i++)
    {
      ElfW(Phdr) *ph = (ElfW(Phdr *))((l4_addr_t)phdr + i*ehdr->e_phentsize);

      if (ph->p_type == PT_PHDR ||
	  ph->p_type == PT_LOAD ||
	  ph->p_type == PT_INTERP ||
	  ph->p_type == PT_DYNAMIC)
	{
#ifdef DEBUG
	  const char *name = ph->p_type == PT_PHDR
				? "pt_phdr"
				: ph->p_type == PT_LOAD
					? "pt_load"
                                        : ph->p_type == PT_INTERP
                                                ? "pt_interp"
                                                : "pt_dynamic";
#endif

	  l4_addr_t beg  = l4_trunc_page(ph->p_vaddr);
	  l4_addr_t end  = l4_round_page(ph->p_vaddr+ph->p_memsz);
	  l4_size_t size = end - beg;
#ifdef DEBUG
	  app_msg(app, "  sec %02d %08lx-%08lx (%s)", i, beg, end, name);
#endif

	  if (ph->p_type == PT_PHDR)
	    {
	      if (save_phnum)
		*save_phnum = ehdr->e_phnum;
	      if (save_phdr)
		*save_phdr = base + ph->p_vaddr;
	    }

          for (j = 0; j < section_num; j++)
            {
              if (ph->p_vaddr             >= section[j].addr &&
                  ph->p_vaddr+ph->p_memsz <= section[j].addr+section[j].size)
                break;
            }

          if (j >= section_num)
            {
              app_msg(app, "Huh? Did not found pre-allocated section");
              error = -L4_ENOTFOUND;
              goto cleanup;
            }

	  section[j].info.type |= (ph->p_flags & PF_R ? L4_DSTYPE_READ    : 0) |
                                  (ph->p_flags & PF_W ? L4_DSTYPE_WRITE   : 0) |
                                  (ph->p_flags & PF_X ? L4_DSTYPE_EXECUTE : 0);

	  if (app->flags & APP_ALL_WRITBLE)
	    section[j].info.type |= L4_DSTYPE_WRITE;

          map_addr = section_map[j];
	  map_offs = ph->p_vaddr - section[j].addr;

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

	}
    }

  /* sanity check */
  if (env->section_num + section_num > L4ENV_MAXSECT)
    {
      app_msg(app, "Cannot pass sections to application");
      error = -L4_ENOMEM;
      goto cleanup;
    }

  if (load_hdrs && (!ehdr_loaded || !phdr_loaded))
    {
      app_msg(app, "Cannot allocate phdr for ehdr/phdr");
      error = -L4_ENOMEM;
      goto cleanup;
    }

  /* attach all allocated dataspaces to our pager */
  for (i = 0, l4exc = section; i < section_num; i++, l4exc++)
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
	goto cleanup;
    }

cleanup:
  for (i = 0; i < sizeof(section_map) / sizeof(section_map[0]); i++)
    if (section_map[i])
      l4rm_detach(section_map[i]);

  return error;
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
	      printf("Invalid interpreter found: %s\n", interp);
	      return -L4_EINVAL;
	    }

	  /* Indicate that we have found an interpreter section. */
	  return -ELF_INTERPRETER;
	}
    }

  return 0;
}
