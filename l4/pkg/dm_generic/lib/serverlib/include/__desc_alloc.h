/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_generic/server-lib/include/__desc_alloc.h
 * \brief  Generic dataspace manager library, dataspace/client descriptor 
 *         allocation
 *
 * \date   11/21/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2000-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/
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
