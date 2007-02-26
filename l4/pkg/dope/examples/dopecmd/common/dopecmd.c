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

/*** GENERAL INCLUDES ***/
#include <stdio.h>
#include <string.h>

/*** DOpE INCLUDES ***/
#include <dopelib.h>
#include <vscreen.h>
#include <keycodes.h>

/*** LOCAL INCLUDES ***/
#include "startup.h"

#define TERM_W 80
#define TERM_H 25

static long  app_id;
static int   curs_x, curs_y;   /* cursor position              */
static char *cbuf_adr;         /* VTextScreen character buffer */
static char *abuf_adr;         /* VTextScreen attribute buffer */


/*** UTILITY: SET VTEXTSCREEN CURSOR TO NEW POSITION ***/
static void set_cursor(int x, int y) {

	/* take over new cursor position */
	curs_x = x; curs_y = y;
	dope_cmdf(app_id, "vts.set(-cursorx %d -cursory %d)", curs_x, curs_y);
}


/*** UTILITY: SCROLL VTEXTSCREEN CONTENT ONE LINE UP ***/
static void scroll(void) {

	/* move visible lines up */
	memmove(cbuf_adr, cbuf_adr + TERM_W, TERM_W*(TERM_H - 1));
	memmove(abuf_adr, abuf_adr + TERM_W, TERM_W*(TERM_H - 1));

	/* clear new line */
	memset(cbuf_adr + TERM_W*(TERM_H - 1), 0, 80);
	memset(abuf_adr + TERM_W*(TERM_H - 1), 0, 80);

	/* refresh vtextscreen */
	dope_cmd(app_id, "vts.refresh()");
}


/*** UTILITY: INSERT CHARACTER AT CURSOR POSITION ***/
static void insert_char(char c) {
	char *cadr = cbuf_adr + curs_y*TERM_W + curs_x;
	char *aadr = abuf_adr + curs_y*TERM_W + curs_x;
	int len = strlen(cadr);

	if (curs_x >= TERM_W - 1) return;

	/* move all characters behind the cursor to the right */
	memmove(cadr + 1, cadr, len);
	memmove(aadr + 1, aadr, len);

	/* insert character into vtextscreen buffer */
	cbuf_adr[curs_y*TERM_W + curs_x] = c;
	abuf_adr[curs_y*TERM_W + curs_x] = (7<<3) + (3<<6);

	/* refresh vtextscreen */
	dope_cmdf(app_id, "vts.refresh(-x %d -y %d -w %d -h 1)", curs_x, curs_y, len + 1);

	/* move cursor to right */
	set_cursor(curs_x + 1, curs_y);
}


/*** UTILITY: DELETE CHARACTER AT CURSOR POSITION ***/
static void delete_char(void) {
	char *cadr = cbuf_adr + curs_y*TERM_W + curs_x;
	char *aadr = abuf_adr + curs_y*TERM_W + curs_x;
	int len = strlen(cadr) + 1;

	if (curs_x <= 5) return;

	/* move cursor to the left */
	set_cursor(curs_x - 1, curs_y);

	/* move all characters behind the cursor to left */
	memmove(cadr - 1, cadr, len);
	memmove(aadr - 1, aadr, len);
	dope_cmdf(app_id, "vts.refresh(-x %d -y %d -w %d -h 1)", curs_x, curs_y, len);
}


/*** UTILITY: PRINT DOPE COMMAND PROMPT ***/
static void print_prompt(void) {
	sprintf(cbuf_adr + curs_y*TERM_W, "DOpE>");
	memset(abuf_adr + curs_y*TERM_W, (7<<3) + (0<<6) + 1, 5);
	set_cursor(5, curs_y);
	dope_cmdf(app_id, "vts.refresh(-x %d -y %d -w 5 -h 1)", curs_x, curs_y);
}


/*** UTILITY: INSERT NEW LINE ***/
static void newline(void) {
	if (curs_y < TERM_H - 1) {
		set_cursor(0, curs_y + 1);
	} else {
		scroll();
		set_cursor(0, curs_y);
	}
}


/*** UTILITY: PRINT RESULT STRING ***/
static void print_result(char *res) {
	int len = strlen(res);

	/* cut too large strings to fit on the current line */
	if (curs_x + len > TERM_W) len = TERM_W - curs_x;

	/* printf string to character buffer at current cursor position */
	snprintf(cbuf_adr + curs_y*TERM_W + curs_x, len + 1, "%s", res);

	/* use yellow text on black background */
	memset(abuf_adr + curs_y*TERM_W, (3<<3) + (3<<6), len);

	/* set cursor behind the string */
	set_cursor(curs_x + len, curs_y);
}


/*** CALLBACK: CALLED FOR EACH KEY STROKE ***/
static void press_callback(dope_event *e,void *arg) {
	static char res[256];
	char nc;

	if ((e->type != EVENT_TYPE_PRESS)
	 && (e->type != EVENT_TYPE_KEYREPEAT)) return;

	nc = dope_get_ascii(app_id, e->press.code);

	/* move cursor to left but prevent overwriting the prompt */
	if (e->press.code == DOPE_KEY_LEFT) {
		if (curs_x > 5) set_cursor(curs_x - 1, curs_y);
		return;
	}

	/* move cursor to right but only in string range */
	if (e->press.code == DOPE_KEY_RIGHT) {
		if (cbuf_adr[curs_y*TERM_W + curs_x]) set_cursor(curs_x + 1, curs_y);
		return;
	}

	/* only handle valid ascii characters */
	if (!nc) return;

	switch (nc) {

		/* backspace */
		case 8:
			delete_char();
			break;

		/* return */
		case 10:

			/* if there are some characters on the line, execute them as command */
			if (cbuf_adr[curs_y*TERM_W + 5]) {
				dope_reqf(app_id, res, sizeof(res), "%s", cbuf_adr + curs_y*TERM_W + 5);
				newline();
				print_result(res);
			}
			newline();
			print_prompt();
			break;

		/* other printable character */
		default:
			insert_char(nc);
			break;
	}
}


/*** MAIN PROGRAM ***/
int main(int argc,char **argv) {

	native_startup(argc,argv);
	dope_init();
	app_id = dope_init_app("DOpEcmd");

	#include "simpleterm.dpi"

	printf("dopecmd: attack returned %d\n", dope_cmd(1, "a = new Window()"));

	/* set mode of vtextscreen */
	dope_cmdf(app_id, "vts.setmode(%d, %d, C8A8PLN)", TERM_W, TERM_H);

	/* map vtextscreen buffer to local address space */
	cbuf_adr = vscr_get_fb(app_id, "vts");
	abuf_adr = cbuf_adr + TERM_W*TERM_H;

	if (!cbuf_adr) {
		printf("Error: could not map vtextscreen buffer\n");
		return 1;
	}

	print_prompt();

	dope_bind(app_id, "vts", "press",     press_callback, (void *)0xaaa);
	dope_bind(app_id, "vts", "keyrepeat", press_callback, (void *)0xaaa);

	dope_eventloop(app_id);
	return 0;
}
