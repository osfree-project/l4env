/*
 * \brief	DOpE Terminal widget module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This widget type provides the functionality
 * that is needed for text based applications.
 * It implements  a subset  of VT100 and  also
 * supports  the standard escape sequences for
 * using colors.
 */


struct private_terminal;
#define TERMINAL struct private_terminal
#define WIDGET TERMINAL
#define WIDGETARG WIDGET

#include "dope-config.h"
#include "memory.h"
#include "basegfx.h"
#include "frame.h"
#include "widget_data.h"
#include "clipping.h"
#include "widget.h"
#include "fontman.h"
#include "scrollbar.h"
#include "terminal.h"
#include "script.h"
#include "widman.h"


static struct memory_services *mem;
static struct basegfx_services *gfx;
static struct widman_services *widman;
static struct clipping_services *clip;
static struct scrollbar_services *scroll;
static struct fontman_services	*fontman;
static struct script_services  *script;
static struct frame_services *frame;

#define TERM_UPDATE_TEXT_CHANGED 0x01

TERMINAL {
	/* entry must point to a general widget interface */
	struct widget_methods 	*gen;	/* for public access */
	
	/* entry is for the ones who knows the real widget identity */
	struct terminal_methods *term;	/* for dedicated users */
	
	/* entry contains general widget data */
	struct widget_data		*wd; 
	
	/* here comes the private terminal specific data */
	long		update_flags;
	s32		font_id;
	s16		char_w,char_h;
	s16		cursor_x;
	s16		cursor_y;
	s16		y_offset;
	s16		num_lines;
	s16		linelength;
	u8		**text_idx;
	u8		**bgfg_idx;
	u8		curr_style;
};

int init_terminal(struct dope_services *d);


/**********************************/
/*** FUNCTIONS FOR INTERNAL USE ***/
/**********************************/


static void init_textbuffer(TERMINAL *t) {
	u8 	*text_buf;
	u8 	*bgfg_buf;
	s32	i,j;

	t->text_idx = mem->alloc(t->num_lines * sizeof(u8 *));
	t->bgfg_idx = mem->alloc(t->num_lines * sizeof(u8 *));
	text_buf = mem->alloc((t->linelength+1) * t->num_lines);
	bgfg_buf = mem->alloc((t->linelength+1) * t->num_lines);
	
	if (!t->text_idx || !t->bgfg_idx || !text_buf || !bgfg_buf) {
		ERROR(printf("Terminal(init_textbuffer): out of memory!\n");)
		return;
	}
	
	/* init text and attribute buffer */
	for (j=0;j<t->num_lines;j++) {
		t->text_idx[j]=text_buf;
		t->bgfg_idx[j]=bgfg_buf;
		for (i=0;i<t->linelength;i++) {
			*(text_buf++) = ' ';
			*(bgfg_buf++) = 15*16;	/* black background, white foreground */
		}
		*(text_buf++) = 0;	/* terminate string */
		*(bgfg_buf++) = 0;
	}	
}


static void inc_cursor_y(TERMINAL *t) {
	t->cursor_y++;
	if (t->cursor_y >= t->num_lines) {
		t->cursor_y = t->num_lines - 1;
		t->y_offset = (t->y_offset + 1) % t->num_lines;
	}
//	printf("y_offset=%lu\n",t->y_offset);
}


static void inc_cursor_x(TERMINAL *t) {
	t->cursor_x++;
	if (t->cursor_x >= t->linelength) {
		t->cursor_x=0;
		inc_cursor_y(t);
	}
}


static void set_cursor(TERMINAL *t) {
	u8 *curr_text_line;
	u8 *curr_bgfg_line;
	curr_text_line=t->text_idx[(t->cursor_y + t->y_offset)%t->num_lines];
	curr_bgfg_line=t->bgfg_idx[(t->cursor_y + t->y_offset)%t->num_lines];
	curr_bgfg_line[t->cursor_x]=curr_bgfg_line[t->cursor_x]^0xff;
}


static void clear_line(TERMINAL *t,s32 y,s32 from,s32 to) {
	u8 *curr_text_line=t->text_idx[(y + t->y_offset)%t->num_lines];
	u8 *curr_bgfg_line=t->bgfg_idx[(y + t->y_offset)%t->num_lines];
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
	while ((*inp >= '0') && (*inp <= '9')) 	num1 = num1*10 + ((*(inp++))&15);
	
	switch (*inp) {
	
	/* change attribute */
	case 'm':
		/* change foreground color */
		if ((num1>=30) && (num1<=37)) {
			t->curr_style = (t->curr_style & 0x0f) + (num1-30+8)*16;
			break;
		}

		/* change background color */
		if ((num1>=40) && (num1<=47)) {
			t->curr_style = (t->curr_style & 0xf0) + (num1-40);
			break;
		}
	}
	return inp;
}


/******************************/
/*** GENERAL WIDGET METHODS ***/
/******************************/

static void term_draw(TERMINAL *t,long x,long y) {
	s32 j,line;

	x+=t->wd->x;
	y+=t->wd->y;

	clip->push(x,y,x+t->wd->w-1,y+t->wd->h-1);
	for (j=0;j<t->num_lines;j++) {
		line = (j + t->y_offset)%t->num_lines;
		gfx->draw_ansi(x,y + j*t->char_h,t->font_id,t->text_idx[line],t->bgfg_idx[line]);
	}
	clip->pop();
}


static void (*orig_update) (TERMINAL *t,u16 redraw_flag);

static void term_update(TERMINAL *t,u16 redraw_flag) {


/*	if ((t->wd->update & (WID_UPDATE_FOCUS|WID_UPDATE_STATE)) && 
	   !(t->wd->update & (WID_UPDATE_POS|WID_UPDATE_SIZE)) &&
	    redraw_flag) {
		t->gen->force_redraw(t);
	}*/
	orig_update(t,redraw_flag);
	t->update_flags=0;
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
		curr_text_line=t->text_idx[(t->cursor_y + t->y_offset)%t->num_lines];
		curr_bgfg_line=t->bgfg_idx[(t->cursor_y + t->y_offset)%t->num_lines];
		
		/* valid character -> just insert it */
		if (*new >= 0x20) {
			curr_text_line[t->cursor_x]=*new;
			curr_bgfg_line[t->cursor_x]=t->curr_style;
			inc_cursor_x(t);
			set_cursor(t);
			new++;
			continue;
		}
		
		switch (*new) {
		
		/* linefeed */
		case 10: set_cursor(t);
				 inc_cursor_y(t);t->cursor_x=0;
				 clear_line(t,t->cursor_y,0,t->linelength-1);
				 set_cursor(t);
				 break;

		/* backspace */
		case 8: if (t->cursor_x) {
				 	set_cursor(t);
				 	t->cursor_x--;
					curr_text_line[t->cursor_x]=' ';
					curr_bgfg_line[t->cursor_x]=t->curr_style;
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
	struct font_struct *fnt;
	
	/* allocate memory for new widget */
	TERMINAL *new = (TERMINAL *)mem->alloc(sizeof(TERMINAL)+sizeof(struct widget_data));
	if (!new) {
		ERROR(printf("Terminal(create): out of memory\n"));
		return NULL;
	}
	new->gen = &gen_methods;	/* pointer to general widget methods */
	new->term= &term_methods;	/* pointer to terminal specific methods */

	/* set general widget attributes */
	new->wd = (struct widget_data *)((long)new + sizeof(TERMINAL));
	widman->default_widget_data(new->wd);
		
	/* set terminal specific attributes */
	fnt = fontman->get_by_id(1);
	if (!fnt) {
		WARNING(printf("Terminal(create): font with id 1 does not exist\n");)
		return new;
	}
	new->font_id  	= fnt->font_id;
	new->char_w		= fnt->width_table[50];
	new->char_h		= fnt->img_h;
	new->num_lines	= 25;
	new->linelength	= 80;
	new->y_offset	= 0;
	new->cursor_x	= 0;
	new->cursor_y	= 0;
	new->curr_style	= 15*16+0; /* white foreground, black background */
	new->wd->w		= new->char_w*new->linelength;
	new->wd->h		= new->char_h*new->num_lines;
	new->wd->min_w	= new->wd->w;
	new->wd->max_w	= new->wd->w;
	new->wd->min_h	= new->wd->h;
	new->wd->max_h	= new->wd->h;
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
	void *widtype;

	widtype = script->reg_widget_type("Terminal",(void *(*)(void))create);

//	script->reg_widget_attrib(widtype,"string text",NULL,but_set_text,gen_methods.update);
	script->reg_widget_method(widtype,"void print(string s)",term_print);
	
	widman->build_script_lang(widtype,&gen_methods);
}


int init_terminal(struct dope_services *d) {

	mem		= d->get_module("Memory 1.0");
	gfx		= d->get_module("Basegfx 1.0");
	widman	= d->get_module("WidgetManager 1.0");
	fontman	= d->get_module("FontManager 1.0");
	clip	= d->get_module("Clipping 1.0");
	script	= d->get_module("Script 1.0");
	scroll  = d->get_module("Scrollbar 1.0");
	frame	= d->get_module("Frame 1.0");
	
	/* define general widget functions */
	widman->default_widget_methods(&gen_methods);

	orig_update=gen_methods.update;

	gen_methods.draw=term_draw;
	gen_methods.update=term_update;

	build_script_lang();

	
	d->register_module("Terminal 1.0",&services);
	return 1;
}
