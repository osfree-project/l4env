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

#include <l4/nitpicker/view.h>
#include <l4/nitpicker/nitpicker-client.h>
#include <l4/names/libnames.h>
#include <stdio.h>
#include <stdlib.h>
#include "private.h"


struct nitpicker_view* nitpicker_view_create(struct nitpicker_buffer *npb) {
	struct nitpicker_view *npv;
	CORBA_Environment env = dice_default_environment;

	if (!nitpicker_buffer_is_valid(npb)) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}

	npv = (struct nitpicker_view *)calloc(1, sizeof(struct nitpicker_view));

	if (!npv) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}

	npv->npb = npb;
	npv->listener = l4_myself();

	npv->id = nitpicker_new_view_call(&npv->npb->np->srv, npb->id,
	                                  &npv->listener,
	                                  &env);

	if (npv->id < 0) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		free(npv);
		return NULL;
	}

	return npv;
}


void nitpicker_view_destroy(struct nitpicker_view *npv) {
	CORBA_Environment env = dice_default_environment;

	if (!nitpicker_view_is_valid(npv)) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return;
	}

	nitpicker_destroy_view_call(&npv->npb->np->srv, npv->id, &env);

	free(npv);
}


int nitpicker_view_is_valid(const struct nitpicker_view *npv) {
	return npv && !(npv->id < 0) && nitpicker_buffer_is_valid(npv->npb);
}


int nitpicker_view_set_view_port(struct nitpicker_view *npv,
                                 int buffer_x, int buffer_y, int x, int y,
                                 unsigned int width, unsigned int height,
                                 int do_redraw) {

	CORBA_Environment env = dice_default_environment;

	if (!nitpicker_view_is_valid(npv)) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return -1;
	}

	return nitpicker_set_view_port_call(&npv->npb->np->srv, npv->id,
	                                    buffer_x, buffer_y, x, y,
	                                    (int)width, (int)height,
	                                    do_redraw, &env);
}


int nitpicker_view_stack(struct nitpicker_view *npv,
                         struct nitpicker_view *neighbor,
                         nitpicker_stack_position_t stack_pos, int do_redraw) {
	CORBA_Environment env = dice_default_environment;

	if (!nitpicker_view_is_valid(npv)) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return -1;
	}

	switch (stack_pos) {
		case TOP:
			return nitpicker_stack_view_call(&npv->npb->np->srv,
			                                 npv->id, -1, 1, do_redraw,
			                                 &env);
		case BOTTOM:
			return nitpicker_stack_view_call(&npv->npb->np->srv,
			                                 npv->id, -1, -1, do_redraw,
			                                 &env);
		case INFRONT:
			if (!nitpicker_view_is_valid(neighbor))
				return -2;

			return nitpicker_stack_view_call(&npv->npb->np->srv,
			                                 npv->id, neighbor->id, 0, do_redraw,
			                                 &env);
		case BEHIND:
			if (!nitpicker_view_is_valid(neighbor))
				return -3;

			return nitpicker_stack_view_call(&npv->npb->np->srv,
			                                 npv->id, neighbor->id, 1, do_redraw,
			                                 &env);
		default:
			printf("Failure at %s:%d\n", __FILE__, __LINE__);
			break;
	}

	return -4;
}


int nitpicker_view_set_title(struct nitpicker_view *npv, const char *title) {
	CORBA_Environment env = dice_default_environment;

	if (!nitpicker_view_is_valid(npv)) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return -1;
	}

	return nitpicker_set_view_title_call(&npv->npb->np->srv, npv->id, title,
	                                     &env);
}


int nitpicker_view_make_background(struct nitpicker_view *npv) {
	CORBA_Environment env = dice_default_environment;

	if (!nitpicker_view_is_valid(npv)) {
		printf("Failure at %s:%d\n", __FILE__, __LINE__);
		return -1;
	}

	return nitpicker_set_background_call(&npv->npb->np->srv, npv->id, &env);
}
