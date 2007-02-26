/*
 * \brief   DOpE command terminal 
 * \date    2002-11-21
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This is a small example client for DOpE. It provides
 * a terminal to interact with the DOpE server based on
 * the DOpE command language.
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <stdio.h>
#include <dopelib.h>
#include "startup.h"

static long app_id;

static char esc_red_bg[6]   = {27,'[','4','1','m',0};
static char esc_black_bg[6] = {27,'[','4','0','m',0};

static char esc_white_fg[6] = {27,'[','3','7','m',0};
static char esc_yellow_fg[6]= {27,'[','3','3','m',0};

static int  curs_x=0;
static char command[256];

static void print_dope_prompt(void) {   
	dope_cmdf(app_id, "t.print(\"%sDOpE>%s\")", esc_red_bg, esc_black_bg);
}

static void print_answer(char *s) {
	dope_cmdf(app_id, "t.print(\"%s%s%s\n\")", esc_yellow_fg, s, esc_white_fg);
}


static void termpress_callback(dope_event *e,void *arg) {
	char c[3] = {0,0,0};
	char nc = 0;
	int  execute = 0;

	if (e->type == EVENT_TYPE_PRESS) {
		nc = dope_get_ascii(app_id, e->press.code);
		c[0] = nc;
		if (c[0] == '"') { c[0]='\\'; c[1]='"'; c[2]=0; }
		if (c[0] == 10 ) { c[0]='\\'; c[1]='n'; c[2]=0; execute = 1; nc=0; }
		if (c[0] == 8) {
			if (curs_x > 0) {
				curs_x--;
			} else {
				c[0] = 0;
			}
			nc = 0;
		}
		if (nc && (curs_x < 80)) command[curs_x++] = nc;
		dope_cmdf(app_id, "t.print(\"%s\")", c);
	}
	if (execute) {
		char retbuf[256];
		command[curs_x] = 0;
		dope_req(app_id, retbuf, 256, command);
		print_answer(retbuf);
		print_dope_prompt();
		curs_x = 0;
	}
}


int main(int argc,char **argv) {

	native_startup(argc,argv);
	dope_init();
	app_id = dope_init_app("DOpEcmd");

	printf("DOpEcmd(main): init_app finished.\n");
	
	#include "simpleterm.i"

	print_dope_prompt();
	dope_bind(app_id,"t","press",termpress_callback,(void *)0xaaa);
	dope_eventloop(app_id);
	return 0;
}
