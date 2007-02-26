/* $Id$ */
/**
 * \file	exec/server/src/bin_obj.cc
 * \brief	Administration of ELF binary objects
 *
 * An ELF binary object is an ELF binary with dependant ELF libraries.
 * Each of these libraries can depend on other ELF libraries.
 *
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the exec package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include <l4/env/errno.h>
#include <l4/exec/errno.h>
#include <l4/exec/exec.h>
#include <string.h>

#include <l4/dm_mem/dm_mem.h>

#include "exc.h"
#include "bin_obj.h"
#include "elf32.h"

#include "config.h"
#include "assert.h"
#include "debug.h"

dsc_array_t *bin_objs = (dsc_array_t*)NULL;

/** constructor */
bin_obj_t::bin_obj_t(l4_uint32_t _id)
  : dsc_obj_t(bin_objs, _id)
{
  free_dep = deps;
  next_dep = deps;
  *free_dep = (exc_obj_t*)NULL;
}

/** destructor */
bin_obj_t::~bin_obj_t()
{
  exc_obj_t **exc_obj = deps;

  for (exc_obj = deps; exc_obj < free_dep; exc_obj++)
    (*exc_obj)->remove_reference();
}

/** add new dependency */
int
bin_obj_t::add_to_dep(exc_obj_t *exc_obj)
{
  /* do we have it already? */
  if (!have_dep(exc_obj))
    {
      if (free_dep >= deps + EXC_MAXLIB+1)
	{
	  msg("More than %d dependant libraries", EXC_MAXLIB);
	  return -L4_ENOMEM;
	}

#ifdef DEBUG_DEPENDANTS
      if (deps[0])
	printf("    add lib \"%s\"\n"
	       " => bin_obj \"%s\"\n",
	       exc_obj->get_pathname(), deps[0]->get_pathname());
#endif
      
      exc_obj->add_reference();

      *free_dep++ = exc_obj;
      *free_dep   = (exc_obj_t*)NULL;
    }

  return 0;
}

int
bin_obj_t::have_dep(exc_obj_t *new_dep)
{
  exc_obj_t **dep;

  for (dep=deps; *dep; dep++)
    if (*dep==new_dep)
      /* already in list */
      return 1;

  /* not found */
  return 0;
}

exc_obj_t*
bin_obj_t::find_exc_obj(const char *fname)
{
  exc_obj_t **exc_obj;

  for (exc_obj=deps; *exc_obj; exc_obj++)
    {
      if (!strcmp((*exc_obj)->get_fname(), fname))
	return *exc_obj;
    }

  msg("library \"%s\" not found", fname);
  return (exc_obj_t*)NULL;
}

exc_obj_t*
bin_obj_t::get_nextdep(void)
{
  return (next_dep < free_dep) 
    ? *next_dep++ 
    : (exc_obj_t*)NULL;
}

int
bin_obj_t::load_libs(l4env_infopage_t *env)
{
  int error;
  exc_obj_t **lib, *dep;
  
  /* ELF standard, page 2-15, Shared Object Dependencies:
   * The dynamic linker first looks at the symbol table of the executable
   * program itself, then at the symbol table of the DT_NEEDED entries
   * (in order), then all second level DT_NEEDED entries, and so on. */
  
  for ( ; (dep=get_nextdep()); )
    {
      if ((error = dep->add_to_env(env)) ||
	  (error = dep->load_libs(env)))
	return error;

      for (lib=dep->get_deps(); *lib; lib++)
	if ((error = add_to_dep(*lib)))
	    return error;
    }

  return 0;
}

/** sanity check: all sections are still relocated? */
int
bin_obj_t::check_relocated(l4env_infopage_t *env)
{
  int i, error;
  
  for (i=0, error=0; i<env->section_num; i++)
    {
      if (env->section[i].info.type & L4_DSTYPE_RELOCME)
	{
	  msg("section %d not yet relocated", i);
	  error = -L4_EINVAL;
	}
    }
  return error;
}

int
bin_obj_t::link_first(l4env_infopage_t *env)
{
  int error;
  exc_obj_t *exc_obj;

  /* link Loader library */
  if (!(exc_obj = find_exc_obj("libloader.s.so")))
    return -L4_EXEC_NOSTANDARD;

  if ((error = check(exc_obj->relocate(env->addr_libloader, env),
		     "relocating libloader library")))
    return -L4_EXEC_CORRUPT;
 
  if ((error = check(exc_obj->link(deps, env),
		     "linking libloader library")))
    return error;

  return 0;
}

int
bin_obj_t::mark_startup_library(l4env_infopage_t *env)
{
  int error;
  exc_obj_t *exc_obj;

  if (!(exc_obj = find_exc_obj("libloader.s.so")))
    return -L4_EXEC_NOSTANDARD;

  if ((error = check(exc_obj->set_section_type(L4_DSTYPE_STARTUP, env),
		     "setting startup flag for libloader")))
    return -L4_EXEC_NOSTANDARD;

  if ((error = check(deps[0]->set_section_type(L4_DSTYPE_STARTUP, env),
		     "setting startup flag for binary")))
    return -L4_EXEC_NOSTANDARD;

  return 0;
}

int
bin_obj_t::link(l4env_infopage_t *env)
{
  int error;
  exc_obj_t **exc_obj;

  /* link symbols of the executable program deps[0] and all dependant 
   * shared libraries deps[1+] */
  for (exc_obj=deps; *exc_obj; exc_obj++)
    if ((error = (*exc_obj)->link(deps, env)))
      return error;

  return 0;
}

/** traverse on all dependent ELF objects and deliver lines information
 * as summary. */
int
bin_obj_t::get_symbols(l4env_infopage_t *env, l4dm_dataspace_t *sym_ds)
{
  int error;
  exc_obj_t **exc_obj;
  l4_size_t size;
  l4dm_dataspace_t ds;
  char *syms;

  if ((error = check_relocated(env)))
    return error;
  
  size = 0;

  /* first, determine size of ds we need */
  for (exc_obj=deps; *exc_obj; exc_obj++)
    {
      l4_size_t s;
      if ((error = (*exc_obj)->get_symbols_size(&s)))
	return error;
      size += s;
    }

  /* no symbols found */
  if (size == 0)
    return -L4_ENOTFOUND;

  size += 1;  /* for terminating \0 */

  msg("Packed %d bytes of symbols", size);
  
  size = l4_round_page(size);

  char ds_name[L4DM_DS_NAME_MAX_LEN];
  strcpy(ds_name, "syms ");
  strncat(ds_name, get_fname(), sizeof(ds_name)-6);
  ds_name[sizeof(ds_name)-1] = '\0';

  if (!(syms = (char*)l4dm_mem_ds_allocate_named(size, L4DM_CONTIGUOUS, 
						 ds_name, &ds)))
    return -L4_ENOMEM;

  if (!l4dm_mem_ds_is_contiguous(&ds))
    {
      msg("Ds %d not contiguous! Releasing...", ds.id);
      l4dm_ds_show(&ds);
      l4dm_mem_release(syms);
      return -L4_ENOMEM;
    }
  
  /* go through all objects and collect symbol information */
  for (exc_obj=deps; *exc_obj; exc_obj++)
    {
      if ((error = (*exc_obj)->get_symbols(env, &syms)))
	{
	  l4dm_mem_release(syms);
	  return error;
	}
    }

  /* detach ds since we do not write anymore to it */
  l4rm_detach(syms);

  *sym_ds = ds;
  return 0;
}

int
bin_obj_t::get_lines(l4env_infopage_t *env, l4dm_dataspace_t *lines_ds)
{
  int error;
  exc_obj_t **exc_obj;
  l4_size_t size;
  l4dm_dataspace_t ds;
  l4_size_t str_sz, lin_sz;
  stab_line_t *lin;
  char *strs, *str;

  if ((error = check_relocated(env)))
    return error;
  
  str_sz = lin_sz = 0;

  /* first, determine size of ds we need */
  for (exc_obj=deps; *exc_obj; exc_obj++)
    {
      l4_size_t str_sz1, lin_sz1;
      if ((error = (*exc_obj)->get_lines_size(&str_sz1, &lin_sz1)))
	return error;
      str_sz += str_sz1;
      lin_sz += lin_sz1;
    }

  /* no symbols found */
  if (lin_sz == 0)
    return -L4_ENOTFOUND;

  lin_sz += sizeof(stab_line_t); /* for terminating entry */

  msg("Packed %d bytes of lines", str_sz+lin_sz);
  
  size = l4_round_page(str_sz+lin_sz);

  char ds_name[L4DM_DS_NAME_MAX_LEN];
  strcpy(ds_name, "lines ");
  strncat(ds_name, get_fname(), sizeof(ds_name)-7);
  ds_name[sizeof(ds_name)-1] = '\0';

  if (!(strs = (char*)l4dm_mem_ds_allocate_named(size, L4DM_CONTIGUOUS, 
						 ds_name, &ds)))
    return -L4_ENOMEM;

  if (!l4dm_mem_ds_is_contiguous(&ds))
    {
      msg("Ds %d not contiguous! Releasing...", ds.id);
      l4dm_ds_show(&ds);
      l4dm_mem_release(strs);
      return -L4_ENOMEM;
    }
  
  lin = (stab_line_t*)strs;
  str = strs + lin_sz;

  /* go through all objects and collect symbol information */
  for (exc_obj=deps; *exc_obj; exc_obj++)
    {
      if ((error = (*exc_obj)->get_lines(env, &str, &lin, str - strs)))
	{
	  l4dm_mem_release(strs);
	  return error;
	}
    }

  *lin++ = (stab_line_t) { 0, 0 };

  /* detach ds since we do not write anymore to it */
  l4rm_detach(strs);

  *lines_ds = ds;
  return 0;
}

int
bin_obj_t::find_sym(const char *fname, const char *symname, 
		    l4env_infopage_t *env, l4_addr_t *addr)
{
  exc_obj_t *exc_obj;
  
  if (!(exc_obj = find_exc_obj(fname)))
    {
      msg("Can't find \"%s\"", fname);
      return -L4_EXEC_BADFORMAT;
    }

  return (exc_obj->find_sym(symname, env, addr));
}

/** Search for the first entry point: l4loader_init of libloader.
 *
 * \param env		L4 environment infopage
 * \return		0 on success */
int
bin_obj_t::set_1st_entry(l4env_infopage_t *env)
{
  int error;

  /* find entry in libloader.s.so */
  if ((error = find_sym("libloader.s.so", "l4loader_init",
		        env, &env->entry_1st)))
    {
      msg("1st entry (l4loader_init) not found (error=%d)", error);
      return error;
    }

  return 0;
}

/** Search for the second entry point: l4env_init.
 *
 * \param env		L4 environment infopage
 * \return		0 on success */
int
bin_obj_t::set_2nd_entry(l4env_infopage_t *env)
{
  int error;
  
  /* find entry for libl4env_s.so */
  if ((error = find_sym("libloader.s.so", "l4env_init",
			env, &env->entry_2nd)))
    {
      msg("2nd entry (l4env_init) not found (error=%d)", error);
      return -L4_EXEC_NOSTANDARD;
    }

  return 0;
}

