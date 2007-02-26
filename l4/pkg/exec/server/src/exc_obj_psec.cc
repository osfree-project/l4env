/* $Id$ */
/**
 * \file	exec/server/src/exc_obj_psec.cc
 *
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 * \brief	program section maintenance */

#include <malloc.h>

#include <l4/env/errno.h>
#include <l4/sys/types.h>
#include <l4/l4rm/l4rm.h>
#include <l4/exec/exec.h>

#include <l4/dm_mem/dm_mem.h>
#include <l4/dm_phys/dm_phys.h>

#include "exc_obj_psec.h"

#include "config.h"
#include "assert.h"
#include "debug.h"
#include "bin_obj.h"
#include "exc.h"
#include "check.h"


static l4_addr_t psecs_vaddr[EXC_MAXBIN][L4ENV_MAXSECT];

/* get the index of program section in the psecs_vaddr array */
l4_addr_t*
exc_obj_psec_idx_psec_here(int env_id, int envsec_idx)
{
  int env_idx = env_id & UNIQUE_ID_MASK;
  
  if (env_idx    < 0 || env_idx    >= EXC_MAXBIN ||
      envsec_idx < 0 || envsec_idx >= L4ENV_MAXSECT)
    {
      printf("invalid envpage or minor id (%08x,%d)\n", env_id, envsec_idx);
      return 0;
    }
  
  return &psecs_vaddr[env_idx][envsec_idx];
}

/* get the address of a program section in our address space */
l4_addr_t
exc_obj_psec_here(l4env_infopage_t *env, int envsec_idx)
{
  int env_id = env->id;
  l4_addr_t *addr = exc_obj_psec_idx_psec_here(env_id, envsec_idx);

  return addr ? *addr : 0;
}

/* kill all shared sections of an environment page */
int
exc_obj_psec_kill_from_env(l4env_infopage_t *env)
{
  int i, env_id = env->id;
  l4exec_section_t *l4exc;

  for (i=0, l4exc=env->section; i<env->section_num; i++, l4exc++)
    {
      l4_addr_t *addr;

      /* determine the pointer to the address in our address space */
      if (!(addr = exc_obj_psec_idx_psec_here(env_id, i)) || !*addr)
	{
     	  printf("psec %d of envpage %08x not found (ds=%d)\n",
		 i, env_id, l4exc->ds.id);
	  continue;
	}

      /* if the section is not shared, do nothing because we must not
       * change the original object */
      if ((l4exc->info.type & L4_DSTYPE_SHARE))
	{
	  /* detach + release dataspace */
	  l4dm_mem_release((void*) *addr);
	  l4exc->ds = L4DM_INVALID_DATASPACE;
	}

      /* mark entry as free */
      *addr = 0;
    }

  return 0;
}

/* constructor */
exc_obj_psec_t::exc_obj_psec_t()
  : vaddr(L4_MAX_ADDRESS), link_addr(0)
{
}

/* destructor */
exc_obj_psec_t::~exc_obj_psec_t()
{
  done_ds();
}

/* allocate program section dataspace and attach */
int
exc_obj_psec_t::init_ds(l4_addr_t addr, l4_size_t size_aligned, 
			int direct, int fixed,
			l4_threadid_t dm_id, const char *dbg_name)
{
  if (direct)
    {
      /* need 1-by-1 mapping */
      int error;
      
      if (fixed)
	{
	  error = (l4_addr_t)l4dm_memphys_open(1,
					       addr, size_aligned, 0,
				       	       L4DM_CONTIGUOUS, dbg_name,
			       		       &l4exc.ds);
	}
      else
	{
	  error = (l4_addr_t)l4dm_memphys_open(1,
					       (l4_addr_t)L4DM_MEMPHYS_ANY_ADDR,
					       size_aligned, 
					       8192 /* XXX alignment */,
				       	       L4DM_CONTIGUOUS, dbg_name,
			       		       &l4exc.ds);
	}
      if (error)
	{
	  l4dm_memphys_show_pool_free(1);
	  printf("Can't allocate memory from pool 1 at %08x-%08x\n",
	      addr, addr+size_aligned);
	  return error;
	}

      if ((error = l4rm_attach(&l4exc.ds, size_aligned, 0,
			       L4DM_RW | L4RM_MAP, (void**)&vaddr)))
	{
	  l4dm_close(&l4exc.ds);
	  vaddr = L4_MAX_ADDRESS;
	  return error;
	}
    }
  else
    {
      /* no special mapping needed */
      vaddr = (l4_addr_t)l4dm_mem_ds_allocate_named(size_aligned, 0, dbg_name,
						    &l4exc.ds);
    }
  
  if (vaddr == 0)
    return -L4_ENOMEM;

  return 0;
}

/* free program section */
int
exc_obj_psec_t::done_ds(void)
{
  if (vaddr != L4_MAX_ADDRESS)
    {
      l4dm_mem_release((void*)vaddr);
    }

  return 0;
}

/** Find the appropritate l4exec_section_t in envpage associated with psec.
 * 
 * \param env		L4 environment infopage
 * \return 		section the program section resides in 
 * 			NULL on error */
l4exec_section_t*
exc_obj_psec_t::lookup_env(l4env_infopage_t *env)
{
  int id = l4exc.info.id;
  l4exec_section_t *l4exc_walk;
  l4exec_section_t *l4exc_stop = env->section + env->section_num;
  
  for (l4exc_walk=env->section; l4exc_walk<l4exc_stop; l4exc_walk++)
    if (id == (int)(l4exc_walk->info.id))
      return l4exc_walk;

  Error("EXC section (id=%d) not found in infopage (%p, %d sects)",
	l4exc.info.id, env, env->section_num);
  return (l4exec_section_t*)NULL;
}

/** Share the program section psec by copy-on-write.
 * 
 * Attach the section to a different virtual address. 
 *
 * \param env		environment infopage 
 * \retval out_l4exc	ptr to new created l4exc section 
 * \return		0 on success */
int
exc_obj_psec_t::share_to_env(l4env_infopage_t *env, 
			     l4exec_section_t **out_l4exc,
			     l4_threadid_t client)
{
  int error;
  int envsec_idx = env->section_num;
  l4dm_dataspace_t new_ds;
  l4_addr_t new_addr;
  l4_size_t size;
  l4_addr_t *psec_vaddr;
  l4exec_section_t *new_l4exc = env->section + env->section_num;

  if (!(psec_vaddr = exc_obj_psec_idx_psec_here(env->id, envsec_idx)))
    return -L4_EINVAL;
  
  if (*psec_vaddr)
    {
      Error("psec_vaddr %p not empty (%08x)\n", psec_vaddr, *psec_vaddr);
      return -L4_ENOMEM;
    }

  new_addr = vaddr;
  size = l4exc.size;

  /* share this section? */
  if (l4exc.info.type & L4_DSTYPE_SHARE)
    {
      /* create associated dataspace for this section */
      if ((error = check(l4dm_copy(&l4exc.ds, L4DM_COW, "", &new_ds),
			"sharing psec dataspace")))
	return error;
      
      /* attach the new dataspace to the memory region */
      if ((error = check(l4rm_attach(&new_ds, size, 0,
				     L4DM_RW | L4RM_MAP, (void **)&new_addr),
			"attaching shared psec dataspace")))
	{
	  l4dm_close(&new_ds);
	  return error;
	}

      if ((error = check(l4dm_share(&new_ds, client, L4DM_RW),
			"sharing psec rights to client")))
	{
	  l4rm_detach((void*)new_addr);
	  l4dm_close(&new_ds);
	  return error;
	}

#ifdef DEBUG_SHARING
      printf("Sharing program section (%08x, %d) to %08x-%08x\n", 
	      env->id, envsec_idx, new_addr, new_addr+size);
#endif
      
      *new_l4exc = l4exc;
      new_l4exc->ds = new_ds;

    }
  else
    {
    
#ifdef DEBUG_SHARING
      printf("Referencing program section (%08x, %d) to %08x-%08x\n",
	      env->id, envsec_idx, new_addr, new_addr+size);
#endif

      *new_l4exc = l4exc;
      
      if ((error = check(l4dm_share(&l4exc.ds, client, L4DM_RO),
			"sharing psec rights to client")))
	return error;
    }

  /* store address of referenced/shared program section */
  *psec_vaddr = new_addr;

  /* add reference to our object */
  add_reference();
 
  *out_l4exc = new_l4exc;
  env->section_num++;

  return 0;
}
