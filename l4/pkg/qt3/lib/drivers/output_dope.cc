/* $Id$ */
/*****************************************************************************/
/**
 * \file   lib/drivers/output_dope.cc
 * \brief  DOpE framebuffer driver for Qt/Embedded.
 *
 * \date   10/24/2004
 * \author Josef Spillner <js177634@inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2004-2005 Technische Universitaet Dresden
 * This file is part of the Qt3 port for L4/DROPS, which is distributed under
 * the terms of the GNU General Public License 2. Please see the COPYING file
 * for details.
 */

#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/l4rm/l4rm.h>
#include <l4/util/macros.h>

#include <l4/log/l4log.h>

extern "C" {
#include <l4/dope/dopelib.h>
#include <l4/dope/vscreen.h>
}

#include "output.h"

extern void drops_qws_notify(void);

static int   scr_width, scr_height; /* screen dimensions */
static int   scr_depth;             /* color depth */
static int   scr_linelength;        /* bytes per scanline */
static void *scr_adr;               /* physical screen adress */

static long  app_id;                /* DOpE application */
static int   qwsserver = 1;

static int eventring[100];
static int eventpos = 0, eventmax = 0;

static void qtdesktop_input(dope_event *e, void *arg)
{
	int x = 0, y = 0;
	int r = 0;

	if(eventmax - eventpos >= 100 - 3) return;

	if ((e->type == EVENT_TYPE_PRESS) || (e->type == EVENT_TYPE_RELEASE))
	{
		if(e->type == EVENT_TYPE_RELEASE) r = 1;
		if(e->press.code > 255)
		{
			eventring[(eventmax++ % 100)] = 0; // mouse press(0)
			eventring[(eventmax++ % 100)] = r; // button press(0) or release(1)
			eventring[(eventmax++ % 100)] = 0; // dummy(0)
		}
		else
		{
			eventring[(eventmax++ % 100)] = 2; // key press(2)
			eventring[(eventmax++ % 100)] = r; // key press(0) or release(1)
			eventring[(eventmax++ % 100)] = e->press.code; // keycode(c)
		}
	}
	else if(e->type == EVENT_TYPE_MOTION)
	{
		x = e->motion.abs_x;
		y = e->motion.abs_y;
		eventring[(eventmax++ % 100)] = 1; // mouse move(1)
		eventring[(eventmax++ % 100)] = x; // mouse horizontal position(x)
		eventring[(eventmax++ % 100)] = y; // mouse vertical position(y)
	}
}

static void dopeinput_init(void) {
	dope_bind(app_id, "qtdesktop", "press", qtdesktop_input, (void*)0x0);
	dope_bind(app_id, "qtdesktop", "release", qtdesktop_input, (void*)0x0);
	dope_bind(app_id, "qtdesktop", "motion", qtdesktop_input, (void*)0x0);
}

int dopeinput_ispending(void)
{
	return (eventmax != eventpos);
}

int dopeinput_get(void)
{
	return eventring[eventpos++ % 100];
}

long drops_qws_set_screen(long width, long height, long depth) {
	dope_init();
	app_id = dope_init_app("Fiasco->DOpE->Qt Desktop");

	dope_cmd(app_id, "qtdesktop=new VScreen()");
	dope_cmdf(app_id, "qtdesktop.setmode(640,480,\"RGB16\")");

	scr_adr        = vscr_get_fb(app_id, "qtdesktop");
	if(scr_adr != (void*)0x00100000) {
		qwsserver = 0;
	}
	if(qwsserver) {
		dope_cmd(app_id, "qtcontext=new Window()");
		dope_cmd(app_id, "qtcontext.set(-x 100 -y 100 -w 640 -h 480 -background off -content qtdesktop)");
		dope_cmd(app_id, "qtcontext.open()");
	} else {
		// FIXME: 
		scr_adr = (void*)0x00100000;
	}
	scr_width      = 640;
	scr_height     = 480;
	scr_depth      = 16;
	scr_linelength = scr_width * scr_depth / 8;

	LOG("DOpE driver: initialized at %p\n", scr_adr);

	if(qwsserver)
		dopeinput_init();

	return 1;
}

void drops_qws_refresh_screen(void) {
	dope_cmd(app_id, "qtdesktop.refresh()");
	dope_process_event(app_id);
	drops_qws_notify();
}

long  drops_qws_get_scr_width  (void) {return scr_width;}
long  drops_qws_get_scr_height (void) {return scr_height;}
long  drops_qws_get_scr_depth  (void) {return scr_depth;}
long  drops_qws_get_scr_line   (void) {return scr_linelength;}
void *drops_qws_get_scr_adr    (void) {return scr_adr;}

