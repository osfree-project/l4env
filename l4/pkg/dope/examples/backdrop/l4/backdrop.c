/*
 * \brief   Backdrop image for DOpE
 * \date    2004-11-05
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

/*** GENERAL INCLUDES ***/
#include <stdlib.h>

/*** L4 INCLUDES ***/
#include <l4/dope/dopelib.h>
#include <l4/dope/vscreen.h>
#include <l4/sys/l4int.h>
#include <l4/log/l4log.h>
#include <l4/libpng/l4png_wrap.h>

char LOG_tag[9] = "backdrop";
l4_ssize_t l4libc_heapsize = 500*1024;


extern char _binary_imagedata_png_start[];
extern int  _binary_imagedata_png_size;


/*** UTILITY: REQUEST LONG ATTRIBUTE FROM DOpE WIDGET ***/
static long dope_req_l(int app_id, char *cmd) {
	char buf[16];
	dope_req(app_id, buf, 16, cmd);
	return atol(buf);
}


/*** DUMMY EVENT HANDLER ***/
static void event_callback(dope_event *e, void *arg) { }


/*** MAIN PROGRAM ***/
int main(int argc,char **argv) {
	long   app_id;         /* DOpE application id  */
	short *scr_adr;        /* VScreen pixel buffer */
	int    scr_w, scr_h;   /* screen dimensions    */
	int    png_w, png_h;   /* bg image dimensions  */
	void  *png_adr  =      &_binary_imagedata_png_start;
	int    png_size = (int)&_binary_imagedata_png_size;

	/* init DOpE library */
	if (dope_init()) return -1;

	/* register DOpE-application */
	app_id = dope_init_app("Backdrop");

	/* determine dimensions of the background image */
	png_w = png_get_width(png_adr,  png_size);
	png_h = png_get_height(png_adr, png_size);

	/* request screen dimensions */
	scr_w = dope_req_l(app_id, "screen.w");
	scr_h = dope_req_l(app_id, "screen.h");

	/* open window */
	dope_cmd (app_id, "vscr = new VScreen()");
	dope_cmdf(app_id, "vscr.setmode(%d,%d,\"RGB16\")", png_w, png_h);
	dope_cmdf(app_id, "w = new Window(-content vscr -workx %d)", 2*scr_w);
	dope_bind(app_id, "vscr", "press", event_callback, NULL);

	/* map vscreen buffer to local address space */
	scr_adr = vscr_get_fb(app_id, "vscr");

	if (scr_adr)
		png_convert_RGB16bit(png_adr, scr_adr, png_size, png_w*png_h*2, png_w);

	/* open background window and refresh vscreen */
	dope_cmd (app_id, "w.open()");
	dope_cmd (app_id, "w.back()");
	dope_cmdf(app_id, "w.set(-workx 0 -worky 0 -workw %d -workh %d)", scr_w, scr_h);
	dope_cmd( app_id, "vscr.refresh()" );

	/* enter mainloop */
	dope_eventloop(app_id);
	return 0;
}

