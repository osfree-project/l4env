/*
 * \brief   DOpE nitpicker view manager
 * \date    2004-09-03
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*** L4 INCLUDES ***/
#include <l4/nitpicker/nitpicker-client.h>

/*** LOCAL INLCUDES ***/
#include "dopestd.h"
#include "viewman.h"

extern int nit_buf_id;                     /* from nitscrdrv.c */
extern CORBA_Object_base nitevent_thread;  /* from nitinput.c  */
extern CORBA_Object_base nit;              /* from nitscrdrv.c */

int init_viewman(struct dope_services *d);

static CORBA_Environment env = dice_default_environment;


/*************************
 *** SERVICE FUNCTIONS ***
 *************************/

/*** CREATE A NEW VIEW ***
 *
 * \return  view id of the new view or a negative error code
 */
static struct view *view_create(void) {
	int view_id = nitpicker_new_view_call(&nit, nit_buf_id, &nitevent_thread, &env);
	nitpicker_set_view_title_call(&nit, view_id, "DOpE", &env);
	return (struct view *)view_id;
}


/*** DESTROY VIEW ***/
static void view_destroy(struct view *v) {
	nitpicker_destroy_view_call(&nit, (int)v, &env);
}


/*** POSITION VIEW ***
 *
 * \return  0 on success or a negative error code
 */
static int view_place(struct view *v, int x, int y, int w, int h) {
	return nitpicker_set_view_port_call(&nit, (int)v, x, y, x, y, w, h, 0, &env);
}


/*** BRING VIEW ON TOP ***
 *
 * \return  0 on success or a negative error code
 */
static int view_top(struct view *v) {
	return nitpicker_stack_view_call(&nit, (int)v, -1, 1, &env);
}


/*** BRING VIEW TO BACK ***
 *
 * \return  0 on success or a negative error code
 */
static int view_back(struct view *v) {
	return nitpicker_stack_view_call(&nit, (int)v, -1, 0, &env);
}


/*** SET TITLE OF A VIEW ***
 *
 * \return  0 on success or a negative error code
 */
static int view_set_title(struct view *v, const char *title) {
	return nitpicker_set_view_title_call(&nit, (int)v, title, &env);
}


/*** DEFINE BACKGROUND VIEW ***
 *
 * \return  0 on success or a negative error code
 */
static int view_set_bg(struct view *v) {
	return nitpicker_set_background_call(&nit, (int)v, &env);
}


/****************************************
 *** SERVICE STRUCTURE OF THIS MODULE ***
 ****************************************/

static struct viewman_services services = {
	view_create,
	view_destroy,
	view_place,
	view_top,
	view_back,
	view_set_title,
	view_set_bg,
};


/**************************
 *** MODULE ENTRY POINT ***
 **************************/

int init_viewman(struct dope_services *d) {
	d->register_module("ViewManager 1.0", &services);
	return 1;
}
