/* $Id$ */
/**
 * \file        exec/server/src/dsc.cc
 * \brief       descriptor stuff
 *
 * \date        10/2000
 * \author      Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the exec package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include "dsc.h"

#include <malloc.h>
#include <string.h>
#include <l4/env/errno.h>

#include "assert.h"

/** constructor */
dsc_array_t::dsc_array_t(l4_uint32_t n)
  : entries(n), unique_id(UNIQUE_ID_ADD)
{
}

int
dsc_array_t::init(void)
{
  int size = entries * sizeof(dsc_obj_t*);
  if (!(dsc_objs = (dsc_obj_t**)malloc(size)))
    return -L4_ENOMEM;

  memset(dsc_objs, 0, size);
  dsc_free_obj = dsc_objs;

  return 0;
}

/** alloc one descriptor entry */
int
dsc_array_t::alloc(dsc_obj_t ***dsc_obj, l4_uint32_t *id)
{
  for (int i=entries; i>0; dsc_free_obj = dsc_objs)
    {
      while ((dsc_free_obj < dsc_objs + entries) && i--)
	{
	  if (!*dsc_free_obj)
	    {
	      *dsc_obj = dsc_free_obj;
	      *dsc_free_obj = (dsc_obj_t*)1; /* mark as allocated */
	      *id = (dsc_free_obj - dsc_objs) | get_next_unique_id();
	      return 0;
	    }
	  dsc_free_obj++;
	}
    }

  return -L4_ENOMEM;
}

/* free a descriptor entry */
int
dsc_array_t::free(l4_uint32_t id)
{
  l4_uint32_t idx = id & UNIQUE_ID_MASK;
  
  if (idx < 0 || idx >= entries)
    {
      Error("invalid dsc_obj %08x", id);
      return -L4_ENOTFOUND;
    }
  
  dsc_obj_t **dsc_obj = dsc_objs + idx;

  /* allocated but not yet used */
  if (*dsc_obj != (dsc_obj_t*)1)
    {
      /* not allocated/used */
      if (!*dsc_obj)
	{
	  Error("dsc_obj %08x already freed", id);
	  return -L4_ENOTFOUND;
	}
      
      if ((**dsc_obj).get_id() != id)
	{
	  Error("invalid dsc_obj at %p: id %08x, stored is %08x", 
	      dsc_obj, id, (**dsc_obj).get_id());
	  return -L4_EINVAL;
	}
    }

  *dsc_obj = 0;
  return 0;
}

/** get next unique identifier */
l4_uint32_t
dsc_array_t::get_next_unique_id(void)
{
  return (unique_id += UNIQUE_ID_ADD);
}


/** return object pointer */
dsc_obj_t*
dsc_array_t::lookup(l4_uint32_t id)
{
  l4_uint32_t idx = id & UNIQUE_ID_MASK;
  
  return (idx < 0 || idx >= entries) 
    ? (dsc_obj_t*)NULL
    : *(dsc_objs + idx);
}

/** find object using it's pathname */
dsc_obj_t*
dsc_array_t::find(const char *pathname)
{
  dsc_obj_t **dsc_obj;

  for (dsc_obj=dsc_objs; dsc_obj<dsc_objs+entries; dsc_obj++)
    {
      if (*dsc_obj)
	{
	  if (strstr((*dsc_obj)->get_pathname(), pathname))
	    return *dsc_obj;
	}
    }

  /* not found */
  return (dsc_obj_t*)NULL;
}

