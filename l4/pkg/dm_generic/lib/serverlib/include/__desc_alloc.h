/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_generic/server-lib/include/__desc_alloc.h
 * \brief  Generic dataspace manager library, dataspace/client descriptor 
 *         allocation
 *
 * \date   11/21/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _DM_GENERIC___DESC_ALLOC_H
#define _DM_GENERIC___DESC_ALLOC_H

/* DM_generic includes */
#include <l4/dm_generic/dsmlib.h>

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

/* init descriptor allocation */
int
dsmlib_init_desc_alloc(dsmlib_get_page_fn_t get_page_fn, 
		       dsmlib_free_page_fn_t free_page_fn);

/* allocate dataspace descriptor */
dsmlib_ds_desc_t *
dsmlib_alloc_ds_desc(void);

/* release dataspace descriptor */
void
dsmlib_free_ds_desc(dsmlib_ds_desc_t * desc);

/* allocate client descriptor */
dsmlib_client_desc_t *
dsmlib_alloc_client_desc(void);

/* free client descriptor */
void
dsmlib_free_client_desc(dsmlib_client_desc_t * desc);

#endif /* !_DM_GENERIC___DESC_ALLOC_H */
