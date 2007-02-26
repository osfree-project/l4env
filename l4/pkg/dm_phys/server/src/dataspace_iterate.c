/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/dataspace_iterate.c
 * \brief  Internal dataspace descriptor handling, iterate dataspace list
 *
 * \date   02/03/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/dm_generic/dsmlib.h>

/* DMphys includes */
#include "__dataspace.h"
#include "__debug.h"

/*****************************************************************************
 *** global data
 *****************************************************************************/

/* dataspace list iterator function data
 * we run single-threaded, its save to define them globally
 */

static dmphys_ds_iter_fn_t iterator_fn;      ///< iterator function
static void *              iterator_data;    ///< iterator function data
static l4_threadid_t       restrict_owner;   ///< restrict to dataspace owner
static l4_uint32_t         iterate_flags;    ///< Flags

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  DSMlib iterator function wrapper
 * 
 * \param  desc          DSMlib dataspace descriptor
 * \param  data          Iterator function data, ignored
 */
/*****************************************************************************/ 
static void
__wrap_iterator(dsmlib_ds_desc_t * desc, void * data)
{
  dmphys_dataspace_t * ds = data;
  l4_threadid_t ds_owner;

  /* get internal dataspace descriptor */
  ds = dsmlib_get_dsm_ptr(desc);
  ASSERT((ds != NULL) && (ds->desc == desc));

  /* check dataspace owner */
  ds_owner = dsmlib_get_owner(desc);
  if (l4_is_invalid_id(restrict_owner)        || 
      ((iterate_flags & L4DM_SAME_TASK) && 
       (l4_tasknum_equal(restrict_owner, ds_owner))) ||
      l4_task_equal(restrict_owner, ds_owner))
    iterator_fn(ds, iterator_data);
}

/*****************************************************************************
 *** DMphys internal API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Iterate dataspace list
 * 
 * \param  fn            Function to execute
 * \param  data          Function data pointer
 * \param  owner         Restrict to dataspace owner, if set to #L4_INVALID_ID
 *                       use all dataspaces
 * \param  flags         Flags:
 *                       - #L4DM_SAME_TASK  use all dataspaces owned by 
 *                                          threads of the task specified by
 *                                          \a owner
 */
/*****************************************************************************/ 
void
dmphys_ds_iterate(dmphys_ds_iter_fn_t fn, void * data, l4_threadid_t owner,
		  l4_uint32_t flags)
{
  /* set iterator function / data */
  iterator_fn = fn;
  iterator_data = data;
  restrict_owner = owner;
  iterate_flags = flags;

  /* iterate */
  dsmlib_dataspaces_iterate(__wrap_iterator, NULL);
}
