/*
 * \brief   Nitpicker client library
 * \date    2007-06-05
 * \author  Thomas Zimmermann <tzimmer@os.inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2007 Thomas Zimmermann <tzimmer@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nitpicker package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/nitpicker/nitpicker.h>
#include <l4/nitpicker/nitpicker-client.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/names/libnames.h>
#include <stdio.h>
#include <stdlib.h>
#include "private.h"


struct nitpicker *nitpicker_open() {
	struct nitpicker *np;

	np = (struct nitpicker*)calloc(1, sizeof(struct nitpicker));
    
	if (!np) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}

	if (!names_waitfor_name("Nitpicker", &np->srv, 100000)) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}

	return np;
}


void nitpicker_close(struct nitpicker *np) {
	if (!nitpicker_is_valid(np)) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return;
	}

	free(np);
}


int nitpicker_is_valid(const struct nitpicker *np) {
	return !!np; }


int nitpicker_get_screen_info(const struct nitpicker   *np,
                              unsigned int             *width,
                              unsigned int             *height,
                              nitpicker_pixel_format_t *format) {
	int w, h, bpp;
	CORBA_Environment env = dice_default_environment;

	if (!nitpicker_is_valid(np)) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return -1;
	}

	if (nitpicker_get_screen_info_call(&np->srv, &w, &h, &bpp, &env) < 0) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return -2;
	}

	if (width)
		*width = w;

	if (height)
		*height = h;

	if (format) {
		switch (bpp) {
			case 16:
				*format = RGB16;
				break;
			case 32:
				*format = RGBA32;
				break;
			default:
				printf("Failure at %s:%d\n", __FILE__, __LINE__);
				return -3;
		}
	}

	return 0;
}


int nitpicker_donate_memory(const struct nitpicker *np,
                            unsigned int            max_views,
                            unsigned int            max_buffers,
                            l4dm_dataspace_t       *ds) {
	void *addr;

	if (!nitpicker_is_valid(np) || !max_views ||!max_buffers || !ds) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return -1;
	}

	addr = l4dm_mem_ds_allocate(1024*max_views*max_buffers,
	                            L4DM_CONTIGUOUS|L4RM_LOG2_ALIGNED|L4RM_MAP,
	                            ds);

	if (!addr) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return -2;
	}

	return nitpicker_donate_dataspace(np, max_views, max_buffers, ds);
}


int nitpicker_donate_dataspace(const struct nitpicker *np,
                               unsigned int            max_views,
                               unsigned int            max_buffers,
                               const l4dm_dataspace_t *ds) {
	CORBA_Environment env = dice_default_environment;

	if (!nitpicker_is_valid(np) || !max_views ||!max_buffers || !ds) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return -1;
	}

	if (l4dm_share(ds, np->srv, L4DM_RW)) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return -2;
	}

	if (nitpicker_donate_memory_call(&np->srv, ds, max_views,
	                                               max_buffers, &env) < 0) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return -3;
	}

	return 0;
}


void nitpicker_remove_dataspace(struct nitpicker *np, l4dm_dataspace_t *ds) {
	if (!nitpicker_is_valid(np) || !ds) {
	    printf("Failure at %s:%d\n", __FILE__, __LINE__);
	    return;
	}

	l4dm_revoke(ds, np->srv, L4DM_ALL_RIGHTS);
}
