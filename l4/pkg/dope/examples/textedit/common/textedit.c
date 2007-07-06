/*
 * \brief   Simple text editor for DOpE
 * \date    2004-08-16
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2004-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*** GENERIC INCLUDES ***/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*** DOpE SPECIFIC INCLUDES ***/
#include "dopelib.h"
#include "vscreen.h"
#include "keycodes.h"

struct line {
	char *buf;        /* text line buffer                 */
	int   buf_len;    /* length of text line buffer       */
	int   num_chars;  /* number of characters on the line */
};

static int app_id;              /* DOpE application id                */
static char *vts_adr;           /* VTextScreen buffer base address    */
static int vts_w, vts_h;        /* width and height of VTextScreen    */
static int view_w, view_h;      /* width and height of viewport       */
static int vts_cw, vts_ch;      /* character width and height         */
static struct line *lineidx;    /* index of text lines                */
static int lineidx_size;        /* size of text line index array      */
static unsigned int num_lines;  /* number of text lines               */
static int tx_pos, ty_pos;      /* current viewport position          */
static int curs_line;           /* text line where the cursor resides */
static int curs_pos;            /* cursor position on text line       */

unsigned long l4libc_heapsize = 500*1024;


/*** UTILITY: REQUEST LONG ATTRIBUTE FROM DOpE WIDGET ***/
static long dope_req_l(int app_id, char *cmd) {
	char buf[16];
	dope_req(app_id, buf, 16, cmd);
	return atol(buf);
}


/*** UPDATE VTEXTSCREEN CURSOR ACCORDING TO TEXT CURSOR POSITION ***/
static void update_cursor(void) {
	int cx = curs_pos;
	if (cx > lineidx[curs_line].num_chars) cx = lineidx[curs_line].num_chars;
	dope_cmdf(app_id, "vt.set(-cursorx %d -cursory %d)", cx - tx_pos, curs_line - ty_pos);
}


/*** CHANGE VIEW PORT SUCH THAT THE KEYBOARD CURSOR IS VISIBLE ***/
static void show_cursor(void) {
	int update_sb = 0;

	if (ty_pos < curs_line - view_h + 1) {
		ty_pos = curs_line - view_h + 1;
		update_sb = 1;
	}

	if (ty_pos > curs_line) {
		ty_pos = curs_line;
		update_sb = 1;
	}

	if (update_sb) dope_cmdf(app_id, "vsb.set(-offset %d)", ty_pos * vts_ch);

	update_sb = 0;

	if (tx_pos < curs_pos - vts_w + 1) {
		tx_pos = curs_pos - vts_w + 1;
		update_sb = 1;
	}

	if (tx_pos > curs_pos) {
		tx_pos = curs_pos;
		update_sb = 1;
	}

	if (update_sb) dope_cmdf(app_id, "hsb.set(-offset %d)", tx_pos * vts_cw);
}


/*** PLACE CURSOR ON A VALID INSERT POSITION ***
 *
 * When moving the cursor via the cursor keys the cursor position
 * might be beyond the end of characters. Before inserting or
 * removing new characters, this function must be called.
 */
static void fix_curs_pos(void) {
	if (curs_pos > lineidx[curs_line].num_chars)
		curs_pos = lineidx[curs_line].num_chars;
}


/*** EXPAND SIZE OF AN ARRAY ***
 *
 * This function reallocates an array with the double size of the
 * original array, copies the old content into the new and frees
 * the original array. It is used to dynamically expand the memory
 * that is used to store the text index buffer and the text line
 * buffers.
 */
static int expand_buffer(void **ptr, int elem_size, int *num_elem) {
	long old_size = *num_elem * elem_size;
	void *new;
	
	if (!(new = malloc(old_size * 2))) {
		printf("Editor(expand_buffer): Error: out of memory\n");
		return -1;
	}

	/* keep old buffer content */
	memcpy(new, *ptr, old_size);

	/* clear expanded area */
	memset(((char *)new) + old_size, 0, old_size);

	/* replace old buffer by the new one */
	free(*ptr);
	*ptr = new;
	*num_elem *= 2;

	return 0;
}


/*** INSERT NEW TEXT LINE ***
 *
 * \param at_line  line index at which the new line should be inserted
 * \return         0 on success
 */
static int insert_line(unsigned int at_line) {
	if (at_line > num_lines) at_line = num_lines;

	if (lineidx_size <= num_lines + 1)
		if (expand_buffer((void **)&lineidx, sizeof(struct line), &lineidx_size))
			return -1;

	/* create gap in index for new line */
	memmove(&lineidx[at_line + 1], &lineidx[at_line],
	        (num_lines - at_line) * sizeof(struct line));
	memset(&lineidx[at_line], 0, sizeof(struct line));
	num_lines++;

	/* allocate new text line when needed */
	lineidx[at_line].buf_len = 1;
	if (!(lineidx[at_line].buf = malloc(lineidx[at_line].buf_len))) {
		printf("Editor(insert_line): Error: could not allocate line buffer\n");
		return -1;
	}
	lineidx[at_line].buf[0] = 0;
	lineidx[at_line].num_chars = 0;

	dope_cmdf(app_id, "vsb.set(-realsize %d)", num_lines * vts_ch);
	return 0;
}


/*** REMOVE TEXT LINE ***
 *
 * \param at_line  index of text line to remove
 */
static void remove_line(unsigned int at_line) {
	if (num_lines < 1) return;
	if (at_line >= num_lines) at_line = num_lines - 1;

	free(lineidx[at_line].buf);
	memmove(&lineidx[at_line], &lineidx[at_line + 1],
	        sizeof(struct line) * (num_lines - at_line + 1));
	num_lines--;
}


/*** INSERT NEW CHARACTER ***
 *
 * \param at_line  index of destination line
 * \param at_pos   index of destination positon at text line
 * \param new      new character to insert
 * \return         0 on success
 */
static int insert_char(unsigned int at_line, unsigned int at_pos, char new) {
	struct line *line;

	if (at_line >= num_lines) return -1;

	/* expand text line buffer if needed */
	line = &lineidx[at_line];
	if (line->buf_len <= line->num_chars + 2)
		if (expand_buffer((void **)&line->buf, sizeof(char), &line->buf_len))
			return -1;

	if (at_pos > line->num_chars)
		at_pos = line->num_chars;

	/* create gap for new character */
	memmove(&line->buf[at_pos + 1], &line->buf[at_pos],
	        line->num_chars - at_pos + 1);
	line->num_chars++;

	/* insert new character */
	line->buf[at_pos] = new;
	return 0;
}


/*** REMOVE CHARACTER AT SPECIFIED TEXT LINE AND POSITION ***/
static void remove_char(unsigned int at_line, unsigned int at_pos) {
	struct line *line;

	if (num_lines < 1) return;
	if (at_line >= num_lines) at_line = num_lines - 1;

	line = &lineidx[at_line];

	if (line->num_chars < 1) return;
	if (at_pos >= line->num_chars) at_pos = line->num_chars - 1;

	memmove(&line->buf[at_pos], &line->buf[at_pos + 1],
	        line->num_chars - at_pos + 1);
	line->num_chars--;
}


/*** SPLIT A TEXT LINE AT SPECIFIED POSITION ***
 *
 * \at_line  text line to split into two
 * \at_pos   position where to cut
 * \return   0 on success
 */
static int split_line(unsigned int at_line, unsigned int at_pos) {
	if (insert_line(at_line + 1)) return -1;
	while (at_pos < lineidx[at_line].num_chars) {
		if (insert_char(at_line + 1, -1, lineidx[at_line].buf[at_pos]))
			return -1;
		remove_char(at_line, at_pos);
	}
	return 0;
}


/*** JOIN TWO TEXT LINES ***
 *
 * \return   0 on success
 */
static int join_lines(unsigned int dst, unsigned int src) {

	/* sanity check */
	if ((src >= num_lines) || (dst >= num_lines) || (dst == src)) return -1;

	/* move characters from src to destination */
	while (lineidx[src].num_chars > 0) {
		if (insert_char(dst, -1, lineidx[src].buf[0]))
			return -1;
		remove_char(src, 0);
	}

	/* get rid of src line */
	remove_line(src);
	return 0;
}


/*** UPDATE VTEXTSCREEN REPRESENTATION OF THE TEXT ***/
static void update_vts(void) {
	char *dst = vts_adr;
	int  line = ty_pos;

	/* draw visible text lines */
	for (line=ty_pos; (line < num_lines) && (line - ty_pos <= view_h); line++) {
		int cnt = 0;

		/* copy visible part of text line */
		if (lineidx[line].num_chars > tx_pos) {
			char *s = &lineidx[line].buf[tx_pos];
			while ((cnt < vts_w) && (*s)) dst[cnt++] = *(s++);
		}

		/* clear characters after end of line */
		if (cnt < vts_w)
			memset(&dst[cnt], 0, vts_w - cnt);

		dst += vts_w;
	}

	/* clear trailing lines */
	for (; line - ty_pos < view_h; line++) memset(&dst[0], 0, vts_w);

	update_cursor();
	dope_cmd(app_id, "vt.refresh()");
}


/*** CALLBACK: CALLED WHEN THE HORIZONTAL VIEWPORT POSITION CHANGES ***/
static void hscroll_callback(dope_event *e, void *arg) {
	ty_pos = dope_req_l(app_id, "hsb.offset");
	dope_cmdf(app_id, "f.set(-xview %d)", tx_pos % vts_cw);
	tx_pos /= vts_cw;
	update_vts();
}


/*** CALLBACK: CALLED WHEN THE VERTICAL VIEWPORT POSITION CHANGES ***/
static void vscroll_callback(dope_event *e, void *arg) {
	ty_pos = dope_req_l(app_id, "vsb.offset");
	dope_cmdf(app_id, "f.set(-yview %d)", ty_pos % vts_ch);
	ty_pos /= vts_ch;
	update_vts();
}


/*** CALLBACK: CALLED WHEN THE USER PRESSES KEYS ***/
static void type_callback(dope_event *e, void *arg) {
	char c = dope_get_ascii(app_id, e->press.code);
	switch (c) {
		case 0:
			break;

		case 8:  /* backspace */
			fix_curs_pos();
			if (curs_pos > 0) {
				remove_char(curs_line, curs_pos - 1);
				curs_pos--;
			} else {
				if (curs_line > 0) {
					curs_pos = lineidx[curs_line - 1].num_chars;
					join_lines(curs_line - 1, curs_line);
					curs_line--;
				}
			}
			update_cursor();
			break;

		case 10:  /* carriage return */
			fix_curs_pos();
			split_line(curs_line++, curs_pos);
			curs_pos = 0;
			update_cursor();
			break;

		case 11: /* tab - insert spaces - later we could implement real tabs */
			fix_curs_pos();
			{
				int num_spaces = 4 - (curs_pos % 4);
				for (;num_spaces-- > 0;) insert_char(curs_line, curs_pos++, ' ');
			}
			update_cursor();
			break;

		default:  /* valid ascii value to insert */
			fix_curs_pos();
			insert_char(curs_line, curs_pos++, c);
			update_cursor();
			break;
	}

	if (!c) {
		switch (e->press.code) {

			case DOPE_KEY_LEFT:
				fix_curs_pos();
				if (curs_pos > 0) curs_pos--;
				else {
					if (curs_line > 0) {
						curs_line--;
						curs_pos = lineidx[curs_line].num_chars;
					}
				}
				update_cursor();
				break;

			case DOPE_KEY_RIGHT:
				fix_curs_pos();
				if (curs_pos < lineidx[curs_line].num_chars) {
					curs_pos++;
				} else {
					if (curs_line < num_lines - 1) {
						curs_line++;
						curs_pos = 0;
					}
				}
				update_cursor();
				break;

			case DOPE_KEY_UP:
				if (curs_line > 0) {
					curs_line--;
					update_cursor();
				}
				break;

			case DOPE_KEY_DOWN:
				if (curs_line < num_lines - 1) {
					curs_line++;
					update_cursor();
				}
				break;
		}
	}
	show_cursor();
	update_vts();
}


/*** SET TEXT MODE ***
 *
 * This function allocates a few more text lines and rows
 * than the visisble are (view_w and view_h). This makes the
 * max size of the VTextScreen is a bit larger than the
 * actual viewport and enables the user to enlarge the
 * window within this range. If the size hits its maximum,
 * the resize callback function allocates a larger buffer.
 */
static int set_textmode(int new_w, int new_h) {


	/* free old vtextscreen buffer */
	if (vts_adr) vscr_free_fb(vts_adr);

	view_w = new_w;
	view_h = new_h;
	vts_w  = new_w + 20;
	vts_h  = new_h + 20;

	/* set new mode and request new vtextscreen buffer */
	dope_cmdf(app_id, "vt.setmode(%d, %d, C8A8PLN)", vts_w, vts_h);
	dope_cmd (app_id, "f.set(-content vt)");
	printf("set text mode to %dx%d\n", vts_w, vts_h);
	vts_adr = vscr_get_fb(app_id, "vt");
	if (!vts_adr) {
		printf("Editor(set_textmode): Error: cannot map VTextScreen buffer\n");
		return -1;
	}
	return 0;
}


/*** CALLBACK: CALLED WHEN TEXT FRAME GETS RESIZED ***/
static void resize_callback(dope_event *e, void *arg) {

	view_w = dope_req_l(app_id, "f.workw") / vts_cw;
	view_h = dope_req_l(app_id, "f.workh") / vts_ch;

	if ((view_w >= vts_w - 10) || (view_h >= vts_h - 10)) {
		set_textmode(view_w, view_h);
		update_vts();
	}
}


int main(int argc, char **argv) {

	if (dope_init()) return -1;

	app_id = dope_init_app("Editor");

	#include "textedit.dpi"

	set_textmode(40, 5);

	/* request character width and height */
	vts_cw = dope_req_l(app_id, "vt.charw");
	vts_ch = dope_req_l(app_id, "vt.charh");

	dope_cmdf(app_id, "w.set(-content g -w 9999 -h 9999)");
	dope_cmdf(app_id, "w.set(-workw 400 -workh 300)");
	dope_cmd (app_id, "w.open()");

	dope_bind(app_id, "vsb", "change", vscroll_callback, (void *)0);
	dope_bind(app_id, "hsb", "change", hscroll_callback, (void *)0);
	dope_bind(app_id, "vt",  "press",     type_callback, (void *)0);
	dope_bind(app_id, "vt",  "keyrepeat", type_callback, (void *)0);
	dope_bind(app_id, "f",   "resize",  resize_callback, (void *)0);

	lineidx = malloc(sizeof(struct line));
	lineidx_size = 1;

	insert_line(0);
	update_cursor();

	dope_eventloop(app_id);
	return 0;
}
