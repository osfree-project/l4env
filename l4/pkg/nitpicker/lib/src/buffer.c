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


#include <l4/nitpicker/buffer.h>
#include <l4/nitpicker/nitpicker-client.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/names/libnames.h>
#include <stdio.h>
#include <stdlib.h>
#include "private.h"


struct nitpicker_buffer *nitpicker_buffer_create(struct nitpicker *np,
                                                 unsigned int width,
                                                 unsigned int height) {
	struct nitpicker_buffer *npb;
	nitpicker_pixel_format_t format;
	CORBA_Environment env = dice_default_environment;

	if (!nitpicker_is_valid(np) || !width || !height) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}

	npb = (struct nitpicker_buffer *)calloc(1, sizeof(struct nitpicker_buffer));

	if (!npb) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}

	npb->np = np;
	npb->width = width;
	npb->height = height;

	if (nitpicker_get_screen_info(np, NULL, NULL, &format) < 0) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		free(npb);
		return NULL;
	}

	switch (format) {
		case RGB16:
			npb->addr = l4dm_mem_ds_allocate(width*height*2,
			                                 L4DM_CONTIGUOUS |
			                                 L4RM_LOG2_ALIGNED |
			                                 L4RM_MAP, &npb->ds);
			break;

		case RGBA32:
			npb->addr = l4dm_mem_ds_allocate(width*height*4,
			                                 L4DM_CONTIGUOUS |
			                                 L4RM_LOG2_ALIGNED |
			                                 L4RM_MAP, &npb->ds);
			break;

		default:
			npb->addr = NULL;
			break;
	}

	if (!npb->addr) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		free(npb);
		return NULL;
	}

	if (l4dm_share(&npb->ds, npb->np->srv, L4DM_RW)) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return 0;
	}

	npb->id = nitpicker_import_buffer_call(&npb->np->srv,
	                                       &npb->ds, width, height, &env);

	if (npb->id < 0) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		nitpicker_remove_dataspace(np, &npb->ds);
		l4dm_close(&npb->ds);
		free(npb);
		return NULL;
	}

	return npb;
}


void nitpicker_buffer_destroy(struct nitpicker_buffer *npb) {
	CORBA_Environment env = dice_default_environment;

	if (!nitpicker_buffer_is_valid(npb)) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return;
	}

	nitpicker_remove_buffer_call(&npb->np->srv, npb->id, &env);

	nitpicker_remove_dataspace(npb->np, &npb->ds);
}


int nitpicker_buffer_is_valid(const struct nitpicker_buffer *npb) {
	return npb && !(npb->id < 0) && nitpicker_is_valid(npb->np);
}


int nitpicker_buffer_refresh(struct nitpicker_buffer *npb) {
	unsigned int width, height;

	if (!nitpicker_buffer_is_valid(npb)) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return -1;
	}

	if (nitpicker_buffer_get_size(npb, &width, &height) < 0) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return -2;
	}

	return nitpicker_buffer_refresh_at(npb, 0, 0, width, height);
}


int nitpicker_buffer_refresh_at(struct nitpicker_buffer *npb,
                                unsigned int x,     unsigned int y,
                                unsigned int width, unsigned int height) {
	CORBA_Environment env = dice_default_environment;

	if (!nitpicker_buffer_is_valid(npb)) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return -1;
	}

	nitpicker_refresh_call(&npb->np->srv, npb->id, x, y, width, height, &env);

	return 0;
}


int nitpicker_buffer_get_size(struct nitpicker_buffer *npb,
                              unsigned int *width, unsigned int *height) {
	if (!nitpicker_buffer_is_valid(npb)) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return -1;
	}

	if (width)
		*width = npb->width;

	if (height)
		*height = npb->height;

	return 0;
}


int nitpicker_buffer_get_format(struct nitpicker_buffer  *npb,
                                nitpicker_pixel_format_t *format) {
	if (!nitpicker_buffer_is_valid(npb)) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return -1;
	}

	if (nitpicker_get_screen_info(npb->np, NULL, NULL, format) < 0) {
	    printf("Failure at %s:%d\n", __FILE__, __LINE__);
	    return -2;
	}

	return 0;
}


void *nitpicker_buffer_get_address(struct nitpicker_buffer *npb) {
	if (!nitpicker_buffer_is_valid(npb)) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}

	return npb->addr;
}
