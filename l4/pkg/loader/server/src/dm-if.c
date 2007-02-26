/* $Id$ */
/**
 * \file	loader/server/src/dm-if.c
 *
 * \date	06/11/2001
 * \author	Frank Mehnert
 *
 * \brief	Helper functions for communication with dataspace managers */

#include "dm-if.h"

#include <stdio.h>

#include <l4/sys/consts.h>
#include <l4/env/errno.h>
#include <l4/env/env.h>
#include <l4/l4rm/l4rm.h>
#include <l4/dm_mem/dm_mem.h>

l4_threadid_t app_dm_id = L4_INVALID_ID;	/* default dataspace manager */
l4_threadid_t app_image_dm = L4_INVALID_ID;	/* dm for file image */
l4_threadid_t app_text_dm = L4_INVALID_ID;	/* dm for text section */
l4_threadid_t app_data_dm = L4_INVALID_ID;	/* dm for data section */
l4_threadid_t app_bss_dm = L4_INVALID_ID;	/* dm for bss section */
l4_threadid_t app_stack_dm = L4_INVALID_ID;	/* dm for stack */

/** Create a dataspace
 *
 * \param dm_id		thread_id of dataspace manager to use
 * \param size		size of dataspace
 * \retval addr		address of attached dataspace
 * \retval ds		dataspace descriptor
 * \retval rg		l4rm region descriptor
 * \return		0 on success
 * 			-L4_ENOMEM if not enough memory available */
int
create_ds(l4_threadid_t dm_id, l4_size_t size,
	  l4_addr_t *addr, l4dm_dataspace_t *ds, const char *dbg_name)
{
  int error;

  if ((error = l4dm_mem_open(dm_id, size, 0, 0, dbg_name, ds)))
    return error;

  if (addr)
    {
      if ((error = l4rm_attach(ds, size, 0, L4DM_RW, (void **)addr)))
	return error;

      memset(*(void**)addr, 0, size);
    }

  return 0;
}

/** Junk an attached dataspace
 *
 * \param ds		dataspace id (ignored if L4DM_INVALID_DATASPACE)
 * \param addr		dataspace map address (ignored if -1) */
int
junk_ds(l4dm_dataspace_t * ds, l4_addr_t addr)
{
  int error;

  if (addr)
    {
      /* detach from region manager */
      if ((error = l4rm_detach((void *)addr)))
	{
	  printf("Error %d detaching dataspace\n", error);
	  return error;
	}
    }

  if (!l4dm_is_invalid_ds(*ds))
    {
      /* close dataspace */
      if ((error = l4dm_close(ds)))
	{
	  printf("Error %d closing dataspace\n", error);
	  return error;
	}
    }

  return 0;
}

/** Determine physical address of dataspace
 *
 * \param ds		dataspace
 * \retval phys_addr	physical addr of dataspace
 */
int
phys_ds(l4dm_dataspace_t *ds, l4_size_t size, l4_addr_t *phys_addr)
{
  l4_addr_t addr;
  l4_size_t psize;
  int error;

  if ((error = l4dm_mem_ds_phys_addr(ds, 0, L4DM_WHOLE_DS, &addr, &psize)))
    {
      printf("Error %d (%s) requesting physical addr of ds %d\n",
	     error, l4env_errstr(error), ds->id);
      return -L4_EINVAL;
    }

  /* sanity check */
  if (psize < size)
    {
      printf("ds %d not contiguous as it should be\n", ds->id);
      return -L4_EINVAL;
    }

  *phys_addr = addr;
  return 0;
}

/** Init dm helper stuff */
int
dm_if_init(void)
{
  l4_threadid_t id;

  /* dataspace manager */
  id = l4env_get_default_dsm();
  if (l4_is_invalid_id(id))
    {
      printf("No dataspace manager found!\n");
      return -L4_ENODM;
    }
  app_image_dm = id;
  app_text_dm = id;
  app_data_dm = id;
  app_bss_dm = id;
  app_stack_dm = id;
  app_dm_id = id;

  return 0;
}

