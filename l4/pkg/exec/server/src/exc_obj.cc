/* $Id$ */
/*!
 * \file server/src/exc_obj.cc
 *
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 * \brief exec object implementation.
 *
 * An exec object holds either an ELF binary or an ELF library.
 */
#include <l4/env/errno.h>
#include <l4/exec/exec.h>

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "exc_obj.h"
#include "config.h"
#include "debug.h"
#include "assert.h"
#include "check.h"

static int exc_obj_sect = 1;
dsc_array_t * exc_objs = 0;
    
/** constructor */
exc_obj_t::exc_obj_t(exc_img_t *img, l4_uint32_t _id)
  : dsc_obj_t(exc_objs, _id), 
    hsecs_num(0), psecs_num(0), stab(0), deps_num(0), textreloc_num(0), 
    flags(0), not_valid(0)
{
  deps[0] = (exc_obj_t*)NULL;
  strncpy(pathname, img->get_pathname(), sizeof(pathname)-1);
  pathname[sizeof(pathname)-1] = '\0';

  set_fname(pathname);
  
  if (!(hsecs = (exc_obj_hsec_t*)malloc(sizeof(exc_obj_hsec_t)*EXC_MAXHSECT)))
    not_valid = 1;
}

/** destructor */
exc_obj_t::~exc_obj_t()
{
  exc_obj_psec_t **psec;
  exc_obj_t **exc_obj;

  /* if not yet junked, junk now */
  junk_hsecs();

  /* free stab section */
  delete stab;
  stab = 0;
  
  /* junk our program sections */
  for (psec=psecs; psec<psecs+psecs_num; psec++)
    {
      Assert (*psec!=NULL);
      (*psec)->remove_reference();
    }
  
  /* decrease references to dependant exc_objs */
  for (exc_obj=deps; exc_obj<deps+deps_num; exc_obj++)
    {
      Assert (*exc_obj!=NULL);
      (*exc_obj)->remove_reference();
    }
}

/** free header sections
 * 
 * \return 0 on success */
int
exc_obj_t::junk_hsecs(void)
{
  free(hsecs);
  hsecs=0;

  return 0;
}

/** return pointer to the nth header section
 * 
 * \param idx		index
 * \return		pointer to header section on success
 * 			NULL on failure */
exc_obj_hsec_t*
exc_obj_t::lookup_hsec(int idx)
{
  /* sanity check */
  if (idx > hsecs_num)
    return 0;

  if (!hsecs)
    return 0;

  if (!hsecs[idx].psec)
    return 0;
  
  return hsecs + idx;
}

/** Add all sections of exc_obj to the L4 environment page
 * 
 * \param env		L4 environment info page
 * \return		0 on success
 * 			- \c -L4_ENOMEM out of memory */
int
exc_obj_t::add_to_env(l4env_infopage_t *env)
{
  int i, error;
  unsigned int sect_id;
  l4exec_section_t *l4exc, *l4exc_stop, *envsec;

  /* check if this EXC object was already added to the infopage */
  sect_id = psecs[0]->l4exc.info.id;
  l4exc_stop = env->section + env->section_num;
  for (l4exc=env->section; l4exc<l4exc_stop; l4exc++)
    if (l4exc->info.id == sect_id)
      /* EXC object found, no need to add it's sections */
      return 0;

  /* copy all sections from EXC object into environment infopage */
  for (i=0; i<psecs_num; i++)
    {
      /* create a shared program section */
      if ((error = psecs[i]->share_to_env(env, &envsec, client_tid)))
	return error;

      if (i==0)
	/* first section: mark begin of sections in envpage of an EXC object */
	envsec->info.type |= L4_DSTYPE_OBJ_BEGIN;
      if (i==psecs_num-1)
	/* last section: mark end of sections in envpage of an EXC object */
	envsec->info.type |= L4_DSTYPE_OBJ_END;
    }
  
  return 0;
}

/** Add the exec object new_exc_obj as dependancy
 * 
 * \param new_exc_obj	exec object to add
 * \return		0 on success */
int
exc_obj_t::add_to_dep(exc_obj_t *new_exc_obj)
{
  if (deps_num >= EXC_MAXDEP)
    {
      msg("More than %d dependencies", EXC_MAXDEP);
      return -L4_ENOMEM;
    }
#ifdef DEBUG_DEPENDANTS
  msg("add dep %s", new_exc_obj->pathname);
#endif
  deps[deps_num++] = new_exc_obj;
  deps[deps_num]   = 0;

  new_exc_obj->add_reference();
  
  return 0;
}

/** Find the appropriate program section an address is associated with.
 * 
 * \param addr		address
 * \param size		size
 * \return		associated header section
 * 			NULL on error */
exc_obj_psec_t*
exc_obj_t::range_psec(l4_addr_t addr, l4_size_t size)
{
  int i;
  exc_obj_psec_t *psec;
    
  for (i=0; i<psecs_num; i++)
    {
      psec = psecs[i];
      if (addr      >= psec->l4exc.addr &&
	  addr+size <= psec->l4exc.addr+psec->l4exc.size)
	/* address found */
	return psec;
    }

#if 0
  msg("Range %08x-%08x not found", addr, addr+size);
  for (i=0; i<psecs_num; i++)
    {
      psec = psecs[i];
      printf("psec %d: %08x-%08x\n", 
	  i, psec->l4exc.addr, psec->l4exc.addr+psec->l4exc.size);
    }
#endif
  return 0;
}

/** Find the appropritate address in envpage associated with psec.
 * 
 * Return the relocating address of the appropriate area. This function
 * is needed because addresses of symbols are relative to the address of the
 * first section in the area.
 * 
 * \param l4exc		exec object program section
 * \param env		L4 environment infopage
 * \return		accociated program section
 * 			NULL otherwise */
l4_addr_t
exc_obj_t::env_reloc_addr(l4exec_section_t *l4exc, l4env_infopage_t *env)
{
  for (; l4exc >= env->section; l4exc--)
    if (l4exc->info.type & L4_DSTYPE_OBJ_BEGIN)
      {
	if (l4exc->info.type & L4_DSTYPE_RELOCME)
	  {
	    printf("Section %d in environment infopage not relocated yet\n",
	   	   l4exc-env->section);
	    return 0;
	  }
	return l4exc->addr;
      }

  Error("Bad l4exc section %p (env=%p)", l4exc, env);
  return 0;
}

/** Link EXC objects of environment page.
 * 
 * \param env		L4 environment infopage
 * \return		0 on success
 * 			- \c -L4_EINVAL section(s) already relocated */
int
exc_obj_t::relocate(l4_addr_t reloc_addr, l4env_infopage_t *env)
{
  int i;

  msg("Relocating to %08x", reloc_addr);

  /* relocate all program sections */
  for (i=0; i<psecs_num; i++)
    {
      l4exec_section_t *l4exc;
      
      if (!(l4exc = psecs[i]->lookup_env(env)))
	return -L4_EINVAL;
      
      if (!l4exc->info.type & L4_DSTYPE_RELOCME)
	{
	  msg("Already relocated");
	  return -L4_EINVAL;
	}

      l4exc->addr += reloc_addr;
      l4exc->info.type &= ~L4_DSTYPE_RELOCME;
    }
  
  return 0;
}

int
exc_obj_t::set_section_type(l4_uint16_t type, l4env_infopage_t *env)
{
  int i;

  msg("Set section type to %04x", type);

  /* relocate all program sections */
  for (i=0; i<psecs_num; i++)
    {
      l4exec_section_t *l4exc;
      
      if (!(l4exc = psecs[i]->lookup_env(env)))
	return -L4_EINVAL;
      
      l4exc->info.type |= type;
    }
  
  return 0;
}



int
exc_obj_alloc_sect(void)
{
  return exc_obj_sect++;
}


extern int 
elf32_obj_new(exc_img_t *img, exc_obj_t **exc_obj, l4env_infopage_t *env,
	      l4_uint32_t _id);

extern int 
elf64_obj_new(exc_img_t *img, exc_obj_t **exc_obj, l4env_infopage_t *env,
	      l4_uint32_t _id);

/* create new exc object according to file format */
int
exc_obj_load_bin(const char *fname, int force_load, l4_threadid_t client,
		 int flags, exc_obj_t **exc_obj, l4env_infopage_t *env)
{
  int error = 0;

  *exc_obj = 0;
  dsc_obj_t **dsc_obj;
 
  /* Either we are forced to load the exec object (usually the
   * app binary itself) or we are not forced to load it. In the
   * latter case, look if we have it already. If yes - ok, if
   * not - load it. */
  if (   force_load
      || !(*exc_obj = static_cast<exc_obj_t*>(exc_objs->find(fname))))
    {
      exc_img_t img(fname, env);
      l4_uint32_t  id;
      
      /* Do we still have enough exc_obj entries? */
      if ((error = check(exc_objs->alloc(&dsc_obj, &id),
			 "allocating object descriptor")))
	return error;

      /* get image from file provider */
      if ((error = img.load(env)))
	{
	  exc_objs->free(id);
	  return error;
	}
      
      if (!(error = ::elf32_obj_new(&img, exc_obj, env, id)) ||
	  !(error = ::elf64_obj_new(&img, exc_obj, env, id)))
	{
	  /* save anchor */
	  *dsc_obj = *exc_obj;

	  /* if we are not forced to load the exc_obj, make sharing possible */
	  if (!force_load)
	    (**exc_obj).set_flag(EO_SHARE);
	  
	  (**exc_obj).set_flag(flags);
	  (**exc_obj).set_client(client);

	  /* create object from image */
	  error = (*exc_obj)->img_copy(&img, env);
	}

      if (error)
	{
	  /* free exc_obj descriptor */
	  delete *exc_obj;
	}
    }

  return error;
}

