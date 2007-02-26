/**
 * \file   l4rm/lib/src/userptr.c
 * \brief  User-defined pointers for regions.
 *
 * \date   2006-11-09
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */

/* (c) 2006 Technische Universitaet Dresden
 * This file is part of L4Env, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/env/errno.h>

/* private includes */
#include <l4/l4rm/l4rm.h>
#include "__region.h"

void * l4rm_get_userptr(const void *addr)
{
	void *ptr = 0;

	l4rm_lock_region_list();

	l4rm_region_desc_t *region = l4rm_find_used_region((l4_addr_t)addr);
	if (region)
		ptr = region->userptr;

	l4rm_unlock_region_list();
	return ptr;
}

int l4rm_set_userptr(const void *addr, void *ptr)
{
	int err = -L4_EINVAL;
	l4rm_lock_region_list();

	l4rm_region_desc_t *region = l4rm_find_used_region((l4_addr_t)addr);
	if (region) {
		region->userptr = ptr;
		err = 0;
	}

	l4rm_unlock_region_list();
	return err;
}
