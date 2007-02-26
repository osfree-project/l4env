/*
 * \brief   Nitpicker test program
 * \date    2004-08-23
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*** GENERAL INCLUDES ***/
#include <stdio.h>

/*** L4 INCLUDES ***/
#include <l4/sys/syscalls.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/thread/thread.h>
#include <l4/sys/types.h>
#include <l4/names/libnames.h>
#include <l4/util/util.h>
#include <l4/nitpicker/nitpicker-client.h>
#include <l4/nitpicker/nitevent-server.h>
#include <l4/nitpicker/event.h>

char LOG_tag[9] = "testnit";
l4_ssize_t l4libc_heapsize = 500*1024;

static CORBA_Object_base nit;
static CORBA_Environment env = dice_default_environment;

static int scr_width, scr_height, scr_mode;
static int userstate;
static int curr_win;
static int omx, omy;
static int vid1, vid2, vid3;
static int buf_id;

static struct {
	int x, y, w, h;
} views[10];

#define BUF_W 320
#define BUF_H 240

#define USERSTATE_IDLE 0
#define USERSTATE_MOVE 1



void
nitevent_event_component(CORBA_Object _dice_corba_obj,
                         unsigned long token,
                         int type,
                         int keycode,
                         int rx,
                         int ry,
                         int ax,
                         int ay,
                         CORBA_Server_Environment *_dice_corba_env) {
//	printf("got event: token=%d, type=%d, keycode=%d, rx=%d, ry=%d, ax=%d, ay=%d\n",
//	       (int)token, type, keycode, rx, ry, ax, ay);

	if (type == NITEVENT_TYPE_PRESS) {
		omx = ax;
		omy = ay;
		curr_win = token;
		printf("curr_win=%d\n", curr_win);
//		if (keycode == 272)
			nitpicker_stack_view_call(&nit, token, -1, 1, 1, &env);
//		else if (keycode == 273)
//			nitpicker_stack_view_call(&nit, token, -1, 1, &env);
		
//		nitpicker_stack_view_call(&nit, vid1, vid2, 0, &env);
		userstate = USERSTATE_MOVE;
	}

	if (type == NITEVENT_TYPE_RELEASE) {
		userstate = USERSTATE_IDLE;
	}

	if (type == NITEVENT_TYPE_MOTION) {
		if (userstate == USERSTATE_MOVE) {
			views[curr_win].x += ax - omx;
			views[curr_win].y += ay - omy;
			omx = ax;
			omy = ay;
			
			nitpicker_set_view_port_call(&nit, curr_win, 0, 0,
			                             views[curr_win].x, views[curr_win].y,
			                             views[curr_win].w, views[curr_win].h, 1, &env);
		}
	}
}

int main(int argc, char **argv) {
	CORBA_Object_base myself = l4_myself();
	int ret;
	l4dm_dataspace_t ds;
	short *addr;
	int i = 0, j;

	names_waitfor_name("DOpE", &nit, 10000000);

	if (names_waitfor_name("Nitpicker", &nit, 100000) == 0) {
		printf("Nitpicker is not registered at names!\n");
		return 1;
	}

	l4_sleep(5);

	/* donate memory */
	addr = l4dm_mem_ds_allocate(1000*100,
	                            L4DM_CONTIGUOUS | L4RM_LOG2_ALIGNED | L4RM_MAP,
	                            &ds);
	printf("l4dm_mem_ds_allocate returned %p\n", addr);

	ret = l4dm_share(&ds, nit, L4DM_RW);
	printf("l4dm_share returned %d\n", ret);

	ret = nitpicker_donate_memory_call(&nit, &ds, 10, 10, &env);
	printf("nitpicker_donate_memory_call returned %d\n", ret);

	nitpicker_get_screen_info_call(&nit, &scr_width, &scr_height, &scr_mode, &env);
	printf("scr_width=%d, scr_height=%d, scr_mode=%d\n", scr_width, scr_height, scr_mode);

	addr = l4dm_mem_ds_allocate(BUF_W*BUF_H*scr_mode/8,
	                            L4DM_CONTIGUOUS | L4RM_LOG2_ALIGNED | L4RM_MAP,
	                            &ds);
	printf("l4dm_mem_ds_allocate returned %p\n", addr);

	ret = l4dm_share(&ds, nit, L4DM_RW);
	printf("l4dm_share returned %d\n", ret);

	buf_id = nitpicker_import_buffer_call(&nit, &ds, BUF_W, BUF_H, &env);
	printf("nitpicker_import_buffer_call returned buf_id=%d\n", buf_id);

	vid1 = nitpicker_new_view_call(&nit, buf_id, &myself, &env);
	printf("nitpicker_new_view_call returned vid1=%d\n", vid1);

	views[vid1].x = 240; views[vid1].y = 100; views[vid1].w = 300; views[vid1].h = 140;
	ret = nitpicker_set_view_port_call(&nit, vid1, 0, 0, views[vid1].x, views[vid1].y,
	                                   views[vid1].w, views[vid1].h, 1, &env);

	nitpicker_set_view_title_call(&nit, vid1, "Trusted Colour Haze 1", &env);

	vid2 = nitpicker_new_view_call(&nit, buf_id, &myself, &env);
	printf("nitpicker_new_view_call returned vid2=%d\n", vid2);

	views[vid2].x = 350; views[vid2].y = 200; views[vid2].w = 250; views[vid2].h = 180;
	ret = nitpicker_set_view_port_call(&nit, vid2, 0, 0, views[vid2].x, views[vid2].y,
	                                   views[vid2].w, views[vid2].h, 1, &env);

	nitpicker_set_view_title_call(&nit, vid2, "Trusted Colour Haze 2", &env);

//	vid3 = nitpicker_new_view_call(&nit, buf_id, &myself, &env);
//	printf("nitpicker_new_view_call returned vid3=%d\n", vid3);
//
//	views[vid3].x = 450; views[vid3].y = 250; views[vid3].w = 250; views[vid3].h = 180;
//	ret = nitpicker_set_view_port_call(&nit, vid3, 0, 0, views[vid3].x, views[vid3].y,
//	                                   views[vid3].w, views[vid3].h, 1, &env);
//	nitpicker_set_view_title_call(&nit, vid3, "Trusted Colour Haze 3", &env);

//	nitpicker_set_background_call(&nit, vid1, &env);

	for (j = 0; j < BUF_W*BUF_H; j++) addr[j] = j + i;
	i++;
	nitpicker_refresh_call(&nit, buf_id, 0, 0, 320, 240, &env);

	nitevent_server_loop(NULL);

	return 0;
}
