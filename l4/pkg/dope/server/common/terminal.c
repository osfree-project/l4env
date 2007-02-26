/*
 * \brief   DOpE Terminal widget module
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This widget type provides the functionality
 * that is needed for text based applications.
 * It implements  a subset  of VT100 and  also
 * supports  the standard escape sequences for
 * using colors.
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

struct terminal;
#define WIDGET struct terminal

#include "dopestd.h"
#include "frame.h"
#include "widget_data.h"
#include "gfx.h"
#include "widget.h"
#include "fontman.h"
#include "scrollbar.h"
#include "terminal.h"
#include "script.h"
#include "widman.h"

static struct gfx_services       *gfx;
static struct widman_services    *widman;
static struct scrollbar_services *scroll;
static struct fontman_services   *fontman;
static struct script_services    *script;
static struct frame_services     *frame;

#define TERM_UPDATE_TEXT_CHANGED 0x01

struct terminal_data {
	s32  update_flags;
	s32  font_id;
	s16  char_w,char_h;
	s16  cursor_x;
	s16  cursor_y;
	s16  y_offset;
	s16  num_lines;
	s16  linelength;
	u8 **text_idx;
	u8 **bgfg_idx;
	u8   curr_style;
};

int init_terminal(struct dope_services *d);


/**********************************/
/*** FUNCTIONS FOR INTERNAL USE ***/
/**********************************/


static void init_textbuffer(TERMINAL *t) {
	u8  *text_buf;
	u8  *bgfg_buf;
	s32  i,j;

	t->td->text_idx = malloc(t->td->num_lines * sizeof(u8 *));
	t->td->bgfg_idx = malloc(t->td->num_lines * sizeof(u8 *));
	text_buf = malloc((t->td->linelength+1) * t->td->num_lines);
	bgfg_buf = malloc((t->td->linelength+1) * t->td->num_lines);

	if (!t->td->text_idx || !t->td->bgfg_idx || !text_buf || !bgfg_buf) {
		ERROR(printf("Terminal(init_textbuffer): out of memory!\n");)
		return;
	}

	/* init text and attribute buffer */
	for (j=0;j<t->td->num_lines;j++) {
		t->td->text_idx[j]=text_buf;
		t->td->bgfg_idx[j]=bgfg_buf;
		for (i=0;i<t->td->linelength;i++) {
			*(text_buf++) = ' ';
			*(bgfg_buf++) = 15*16;   /* black background, white foreground */
		}
		*(text_buf++) = 0;           /* terminate string */
		*(bgfg_buf++) = 0;
	}
}


static void inc_cursor_y(TERMINAL *t) {
	char *curr_text_line;
	int i;
	
	t->td->cursor_y++;
	if (t->td->cursor_y >= t->td->num_lines) {
		t->td->cursor_y = t->td->num_lines - 1;
		t->td->y_offset = (t->td->y_offset + 1) % t->td->num_lines;

		/* clear incoming line */
		curr_text_line = t->td->text_idx[(t->td->cursor_y + t->td->y_offset)%t->td->num_lines];
		for (i=0; i<t->td->linelength; i++) curr_text_line[i] = ' ';
		curr_text_line[i] = 0;
	}
//  printf("y_offset=%lu\n",t->y_offset);
}


static void inc_cursor_x(TERMINAL *t) {
	t->td->cursor_x++;
	if (t->td->cursor_x >= t->td->linelength) {
		inc_cursor_y(t);
		t->td->cursor_x=0;
	}
}


static void set_cursor(TERMINAL *t) {
	u8 *curr_text_line;
	u8 *curr_bgfg_line;
	curr_text_line=t->td->text_idx[(t->td->cursor_y + t->td->y_offset)%t->td->num_lines];
	curr_bgfg_line=t->td->bgfg_idx[(t->td->cursor_y + t->td->y_offset)%t->td->num_lines];
	curr_bgfg_line[t->td->cursor_x]=curr_bgfg_line[t->td->cursor_x]^0xff;
}


static void clear_line(TERMINAL *t,s32 y,s32 from,s32 to) {
	u8 *curr_text_line=t->td->text_idx[(y + t->td->y_offset)%t->td->num_lines];
	u8 *curr_bgfg_line=t->td->bgfg_idx[(y + t->td->y_offset)%t->td->num_lines];
	for (;from<=to;from++) {
		curr_text_line[from]=' ';
		curr_bgfg_line[from]=15*16;
	}
}


static char *handle_control_sequence(TERMINAL *t,char *inp) {
	s32 num1=0;

	/* inp points to the escape char */
	inp++;
	if ((*inp) != '[') return inp;

	/* inp points to the '[' char */
	inp++;

	/* read the following number */
	while ((*inp >= '0') && (*inp <= '9')) num1 = num1*10 + ((*(inp++))&15);

	switch (*inp) {

	/* change attribute */
	case 'm':
		/* change foreground color */
		if ((num1>=30) && (num1<=37)) {
			t->td->curr_style = (t->td->curr_style & 0x0f) + (num1-30+8)*16;
			break;
		}

		/* change background color */
		if ((num1>=40) && (num1<=47)) {
			t->td->curr_style = (t->td->curr_style & 0xf0) + (num1-40);
			break;
		}
	}
	return inp;
}


/******************************/
/*** GENERAL WIDGET METHODS ***/
/******************************/

static void term_draw(TERMINAL *t,struct gfx_ds *ds,long x,long y) {
	s32 j,line;

	x+=t->wd->x;
	y+=t->wd->y;

	gfx->push_clipping(ds,x,y,t->wd->w,t->wd->h);
	for (j=0;j<t->td->num_lines;j++) {
		line = (j + t->td->y_offset)%t->td->num_lines;
		gfx->draw_ansi(ds,x,y + j*t->td->char_h,t->td->font_id,t->td->text_idx[line],t->td->bgfg_idx[line]);
	}
	gfx->pop_clipping(ds);
}


static void (*orig_update) (TERMINAL *t,u16 redraw_flag);

static void term_update(TERMINAL *t,u16 redraw_flag) {
	orig_update(t,redraw_flag);
	t->td->update_flags=0;
}

/*********************************/
/*** TERMINAL SPECIFIC METHODS ***/
/*********************************/

static void term_print(TERMINAL *t,char *new) {
	u8 *curr_text_line;
	u8 *curr_bgfg_line;
	if (!new || !t) return;

	/* go through the input string.... */
	while (*new) {
		curr_text_line=t->td->text_idx[(t->td->cursor_y + t->td->y_offset)%t->td->num_lines];
		curr_bgfg_line=t->td->bgfg_idx[(t->td->cursor_y + t->td->y_offset)%t->td->num_lines];

		/* valid character -> just insert it */
		if (*new >= 0x20) {
			curr_text_line[t->td->cursor_x]=*new;
			curr_bgfg_line[t->td->cursor_x]=t->td->curr_style;
			inc_cursor_x(t);
			set_cursor(t);
			new++;
			continue;
		}

		switch (*new) {

		/* linefeed */
		case 10: set_cursor(t);
				 inc_cursor_y(t);t->td->cursor_x=0;
				 clear_line(t,t->td->cursor_y,0,t->td->linelength-1);
				 set_cursor(t);
				 break;

		/* backspace */
		case 8: if (t->td->cursor_x) {
					set_cursor(t);
					t->td->cursor_x--;
					curr_text_line[t->td->cursor_x]=' ';
					curr_bgfg_line[t->td->cursor_x]=t->td->curr_style;
					set_cursor(t);
				}
				break;

		/* escape */
		case 27:new = handle_control_sequence(t,new);
				break;
		}
		new++;
	}
	t->gen->force_redraw(t);
}

static struct widget_methods gen_methods;
static struct terminal_methods term_methods={
	term_print,
};


/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

char dummy[2]={0,0};

static TERMINAL *create(void) {
	struct font *fnt;

	/* allocate memory for new widget */
	TERMINAL *new = (TERMINAL *)malloc(sizeof(struct terminal)
	                                 + sizeof(struct widget_data)
	                                 + sizeof(struct terminal_data));
	if (!new) {
		ERROR(printf("Terminal(create): out of memory\n"));
		return NULL;
	}
	new->gen = &gen_methods;  /* pointer to general widget methods */
	new->term= &term_methods; /* pointer to terminal specific methods */

	/* set general widget attributes */
	new->wd = (struct widget_data *)((long)new + sizeof(struct terminal));
	new->td = (struct terminal_data *)((long)new->wd + sizeof(struct widget_data));
	widman->default_widget_data(new->wd);

	/* set terminal specific attributes */
	fnt = fontman->get_by_id(1);
	if (!fnt) {
		WARNING(printf("Terminal(create): font with id 1 does not exist\n");)
		return new;
	}
	new->td->font_id    = fnt->font_id;
	new->td->char_w     = fnt->width_table[50];
	new->td->char_h     = fnt->img_h;
	new->td->num_lines  = 25;
	new->td->linelength = 80;
	new->td->y_offset   = 0;
	new->td->cursor_x   = 0;
	new->td->cursor_y   = 0;
	new->td->curr_style = 15*16 + 0; /* white foreground, black background */
	new->wd->w     = new->td->char_w * new->td->linelength;
	new->wd->h     = new->td->char_h * new->td->num_lines;
	new->wd->min_w = new->wd->w;
	new->wd->max_w = new->wd->w;
	new->wd->min_h = new->wd->h;
	new->wd->max_h = new->wd->h;
	init_textbuffer(new);
	set_cursor(new);

	return new;
}


/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct terminal_services services = {
	create
};


/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/


static void build_script_lang(void) {
	void *widtype = script->reg_widget_type("Terminal",(void *(*)(void))create);

	script->reg_widget_method(widtype,"void print(string s)",term_print);
	widman->build_script_lang(widtype,&gen_methods);
}


int init_terminal(struct dope_services *d) {

	widman  = d->get_module("WidgetManager 1.0");
	fontman = d->get_module("FontManager 1.0");
	gfx     = d->get_module("Gfx 1.0");
	script  = d->get_module("Script 1.0");
	scroll  = d->get_module("Scrollbar 1.0");
	frame   = d->get_module("Frame 1.0");

	/* define widget functions */
	widman->default_widget_methods(&gen_methods);
	orig_update=gen_methods.update;
	gen_methods.draw=term_draw;
	gen_methods.update=term_update;

	build_script_lang();
	d->register_module("Terminal 1.0",&services);
	return 1;
}
