/*
 * \brief   DOpE nitpicker screen driver module
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
#include <l4/sys/syscalls.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/thread/thread.h>
#include <l4/sys/types.h>
#include <l4/names/libnames.h>
#include <l4/nitpicker/nitpicker-client.h>
#include <l4/nitpicker/nitevent-server.h>
#include <l4/nitpicker/event.h>

/*** LOCAL INCLUDES ***/
#include "dopestd.h"
#include "scrdrv.h"


static int    scr_width, scr_height;    /* screen dimensions                         */
static int    scr_depth;                /* color depth                               */
static int    scr_linelength;           /* bytes per scanline                        */
static void  *buf_adr;                  /* adress of screen buffer (doublebuffering) */

CORBA_Object_base nit;                  /* nitpicker server        */
static l4dm_dataspace_t ds;             /* screen buffer dataspace */

static CORBA_Environment env = dice_default_environment;
int    nit_buf_id;                      /* nitpicker buffer id     */

extern CORBA_Object_base nitevent_thread;  /* from nitinput.c */
extern int config_adapt_redraw;

int init_scrdrv(struct dope_services *d);


/*************************
 *** SERVICE FUNCTIONS ***
 *************************/

/*** SET UP SCREEN ***/
static long set_screen(long width, long height, long depth) {
	int ret;

	buf_adr = l4dm_mem_ds_allocate(100*1024,
	                               L4DM_CONTIGUOUS | L4RM_LOG2_ALIGNED | L4RM_MAP,
	                               &ds);
	printf("l4dm_mem_ds_allocate returned %p\n", buf_adr);

	ret = l4dm_share(&ds, nit, L4DM_RW);
	printf("l4dm_share returned %d\n", ret);

	ret = nitpicker_donate_memory_call(&nit, &ds, 100, 1, &env);
	printf("nitpicker_donate_memory_call returned %d\n", ret);

	nitpicker_get_screen_info_call(&nit, &scr_width, &scr_height, &scr_depth, &env);
	printf("scr_width=%d, scr_height=%d, scr_mode=%d\n", scr_width, scr_height, scr_depth);
	scr_linelength = scr_width;

	buf_adr = l4dm_mem_ds_allocate(scr_width*scr_height*scr_depth/8,
	                               L4DM_CONTIGUOUS | L4RM_LOG2_ALIGNED | L4RM_MAP,
	                               &ds);
	printf("l4dm_mem_ds_allocate returned %p\n", buf_adr);

	ret = l4dm_share(&ds, nit, L4DM_RW);
	printf("l4dm_share returned %d\n", ret);

	nit_buf_id = nitpicker_import_buffer_call(&nit, &ds, scr_width, scr_height, &env);
	printf("nitpicker_import_buffer_call returned nit_buf_id=%d\n", nit_buf_id);

	return 1;
}


/*** DEINITIALISATION ***/
static void restore_screen(void) { }


/*** PROVIDE INFORMATION ABOUT THE SCREEN ***/
static long  get_scr_width  (void) {return scr_width;}
static long  get_scr_height (void) {return scr_height;}
static long  get_scr_depth  (void) {return scr_depth;}
static void *get_scr_adr    (void) {return buf_adr;}
static void *get_buf_adr    (void) {return buf_adr;}


/*** PROPAGATE BUFFER UPDATE TO NITPICKER ***/
static void update_area(long x1,long y1,long x2,long y2) {
	if ((x1 > x2) || (y1 > y2)) return;
	nitpicker_refresh_call(&nit, nit_buf_id, x1, y1, x2 - x1 + 1, y2 - y1 + 1, &env);
}


/*** SET NEW MOUSE SHAPE ***/
static void set_mouse_shape(void *new_shape) { }


/*** SET MOUSE POSITION ***/
static void set_mouse_pos(long mx, long my) { }


/****************************************
 *** SERVICE STRUCTURE OF THIS MODULE ***
 ****************************************/

static struct scrdrv_services services = {
	set_screen:         set_screen,
	restore_screen:     restore_screen,
	get_scr_width:      get_scr_width,
	get_scr_height:     get_scr_height,
	get_scr_depth:      get_scr_depth,
	get_scr_adr:        get_scr_adr,
	get_buf_adr:        get_buf_adr,
	update_area:        update_area,
	set_mouse_pos:      set_mouse_pos,
	set_mouse_shape:    set_mouse_shape,
};


/**************************
 *** MODULE ENTRY POINT ***
 **************************/

int init_scrdrv(struct dope_services *d) {

	config_adapt_redraw = 0;

	if (names_waitfor_name("Nitpicker", &nit, 10000) == 0) {
		printf("Nitpicker is not registered at names!\n");
		return 0;
	}

	d->register_module("ScreenDriver 1.0",&services);
	return 1;
}
