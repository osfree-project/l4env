/*
 * \brief	DOpE Button widget module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 */


struct private_button;
#define BUTTON struct private_button
#define WIDGET BUTTON
#define WIDGETARG WIDGET

#include "dope-config.h"
#include "memory.h"
#include "basegfx.h"
#include "widget_data.h"
#include "clipping.h"
#include "event.h"
#include "widget.h"
#include "button.h"
#include "fontman.h"
#include "img8idata.h"
#include "script.h"
#include "widman.h"

static struct memory_services *mem;
static struct basegfx_services *gfx;
static struct widman_services *widman;
static struct clipping_services *clip;
static struct image8i_services *img8i;
static struct fontman_services *font;
static struct script_services  *script;

#define BUTTON_UPDATE_TEXT	0x01

BUTTON {
	/* entry must point to a general widget interface */
	struct widget_methods 	*gen;	/* for public access */
	
	/* entry is for the ones who knows the real widget identity (button) */
	struct button_methods 	*but;	/* for dedicated users */
	
	/* entry contains general widget data */
	struct widget_data		*wd; /* access for button module and widget manager */
	
	/* here comes the private button specific data */
	long	update_flags;
	char	*text;
	s16	style;
	s16	font_id;				/* if of used font */
	s16	tx,ty;					/* text position inside the button */
	void	(*click)	(WIDGET *);
	void	(*release) 	(WIDGET *);
};


static struct image8i *normal_bg_img;
static struct image8i *focus_bg_img;
static struct image8i *actwin_bg_img;

int init_button(struct dope_services *d);



/**********************************/
/*** FUNCTIONS FOR INTERNAL USE ***/
/**********************************/

static s32 strlength(u8 *s) {
	s32 result=0;
	while (*(s++)) result++;
	return result;
}

static u8 *strdup(u8 *s) {
	u8 *d;
	u8 *result;
	s32 strl;
	if (!s) return NULL;
	strl = strlength(s);
	if (strl>=0) {
		result = mem->alloc(strl+2);
		if (!result) return NULL;
		d=result;
		while (*s) *(d++)=*(s++);
		*d=0;
		return result;
	}
	return NULL;
}


static void update_text_pos(BUTTON *b) {
	if (!b->text) return;
	b->tx = (b->wd->w - font->calc_str_width (b->font_id,b->text))>>1;
	b->ty = (b->wd->h - font->calc_str_height(b->font_id,b->text))>>1;
}



/******************************/
/*** GENERAL WIDGET METHODS ***/
/******************************/

static void but_draw(BUTTON *b,long x,long y) {
	long tx=b->tx,ty=b->ty;

	x+=b->wd->x;
	y+=b->wd->y;

	clip->push(x,y,x+b->wd->w-1,y+b->wd->h-1);
	
	if (b->wd->flags & WID_FLAGS_FOCUS) {
		img8i->paint_scaled(x,y,b->wd->w,b->wd->h,focus_bg_img);

		if (!(b->wd->flags & WID_FLAGS_STATE)) {
			gfx->draw_frame(x,y,x+b->wd->w-1,y+b->wd->h-1,GFX_FRAME_FOCUS);
		}
	} else {
		if (b->style == 2) {
			img8i->paint_scaled(x,y,b->wd->w,b->wd->h,actwin_bg_img);
		} else {
			img8i->paint_scaled(x,y,b->wd->w,b->wd->h,normal_bg_img);		
		}

	}

	if (b->wd->flags & WID_FLAGS_STATE) {
		gfx->draw_frame(x,y,x+b->wd->w-1,y+b->wd->h-1,GFX_FRAME_PRESSED);	
	} else {
		if (!(b->wd->flags & WID_FLAGS_FOCUS)) {
			gfx->draw_frame(x,y,x+b->wd->w-1,y+b->wd->h-1,GFX_FRAME_RAISED);		
		}
	}	
	
	tx+=x;ty+=y;
	if (b->wd->flags & WID_FLAGS_FOCUS) {
		tx+=1;ty+=1;
	}
	if (b->wd->flags & WID_FLAGS_STATE) {
		tx+=2;ty+=2;
	}
	clip->push(x+2,y+2,x+b->wd->w-1-2,y+b->wd->h-1-2);

	if (b->text) gfx->draw_string(tx,ty,b->font_id,b->style,b->text);
	
	clip->pop();
	clip->pop();
}


void (*orig_handle_event) (BUTTON *b,EVENT *e);

static void but_handle_event(BUTTON *b,EVENT *e) {
	int propagate=1;
	switch (e->type) {
	case EVENT_PRESS:
		if (b->click) {
			b->click(b);
			propagate=0;
		}
		break;
	case EVENT_RELEASE:
		if (b->release) {
			b->release(b);
			propagate=0;
		}
		break;
	}

	if (propagate) orig_handle_event(b,e);
}


static void (*orig_update) (BUTTON *b,u16 redraw_flag);

static void but_update(BUTTON *b,u16 redraw_flag) {
	s16 redraw_needed=0;
	if (b->wd->update & (WID_UPDATE_FOCUS|WID_UPDATE_STATE)) redraw_needed = 1;
	if (b->update_flags & BUTTON_UPDATE_TEXT) {
		update_text_pos(b);
		redraw_needed = 1;
	}
	if (b->wd->update & (WID_UPDATE_POS|WID_UPDATE_SIZE)) {
		update_text_pos(b);
		redraw_needed = 0;
	}

	if (redraw_flag && redraw_needed) b->gen->force_redraw(b);
	orig_update(b,redraw_flag);
	b->update_flags=0;
}



/*******************************/
/*** BUTTON SPECIFIC METHODS ***/
/*******************************/

static void but_set_text(BUTTON *b,char *new_txt) { 
	if (b->text) mem->free(b->text);
	b->text=strdup(new_txt);
	b->update_flags = b->update_flags | BUTTON_UPDATE_TEXT;
}


static char *but_get_text(BUTTON *b) {
	if (!b) return NULL;
	return b->text;
}


static void but_set_font(BUTTON *b,s32 font_id) { 
	b->font_id=font_id;
	b->update_flags = b->update_flags | BUTTON_UPDATE_TEXT;
}


static s32 but_get_font(BUTTON *b) {
	if (!b) return 0;
	return b->font_id;
}


static void but_set_style(BUTTON *b,s32 style) { 
	b->style=style;
	b->update_flags = b->update_flags | BUTTON_UPDATE_TEXT;
}


static s32 but_get_style(BUTTON *b) {
	if (!b) return 0;
	return b->font_id;
}


static void but_set_click(BUTTON *b,void (*callback)(BUTTON *b)) {
	b->click=callback;
}


static void but_set_release(BUTTON *b,void (*callback)(BUTTON *b)) {
return;
	b->release=callback;
}


static struct widget_methods gen_methods;
static struct button_methods but_methods={
	but_set_text,
	but_get_text,
	but_set_font,
	but_get_font,
	but_set_style,
	but_get_style,
	but_set_click,
	but_set_release
};



/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

static BUTTON *create(void) {

	/* allocate memory for new widget */
	BUTTON *new = (BUTTON *)mem->alloc(sizeof(BUTTON)+sizeof(struct widget_data));
	if (!new) {
		DOPEDEBUG(printf("Button(create): out of memory\n"));
		return NULL;
	}
	new->gen = &gen_methods;	/* pointer to general widget methods */
	new->but = &but_methods;	/* pointer to button specific methods */

	/* set general widget attributes */
	new->wd = (struct widget_data *)((long)new + sizeof(BUTTON));
	widman->default_widget_data(new->wd);
		
	/* set button specific attributes */
	new->text = NULL;
	new->font_id = 0;
	new->click=NULL;
	new->release=NULL;
	new->style=1;
	new->tx=0;new->ty=0;
	update_text_pos(new);
	
	return new;
}



/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct button_services services = {
	create
};



/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

static void build_script_lang(void) {
	void *widtype;

	widtype = script->reg_widget_type("Button",(void *(*)(void))create);

	script->reg_widget_attrib(widtype,"string text",but_get_text,but_set_text,gen_methods.update);
	script->reg_widget_attrib(widtype,"long font",but_get_font,but_set_font,gen_methods.update);
	
	widman->build_script_lang(widtype,&gen_methods);
}


int init_button(struct dope_services *d) {
	s32 i,j;

	mem		= d->get_module("Memory 1.0");
	gfx		= d->get_module("Basegfx 1.0");
	widman	= d->get_module("WidgetManager 1.0");
	clip	= d->get_module("Clipping 1.0");
	img8i	= d->get_module("Image8iData 1.0");
	font	= d->get_module("FontManager 1.0");
	script	= d->get_module("Script 1.0");
	
	normal_bg_img = img8i->create(100,100);
	actwin_bg_img = img8i->create(100,100);
	focus_bg_img  = img8i->create(100,100);
	
	for (i=0;i<256;i++) {
		*(normal_bg_img->palette + i) = ((i/2+90)<<24) + ((i/2+90)<<16) + ((i/2+110)<<8) + i/2+150;
		*(focus_bg_img->palette + i) = ((i/2+100)<<24) + ((i/2+100)<<16) + ((i/2+140)<<8) + i/2+150;
		*(actwin_bg_img->palette + i) = ((i/2+110)<<24) + ((i/2+100)<<16) + ((i/3+90)<<8) + i/3+90;
	}
	for (j=0;j<100;j++) {
		for (i=0;i<100;i++) {
			*(normal_bg_img->pixels + j*normal_bg_img->w + i) = i+j;
			*(focus_bg_img->pixels + j*focus_bg_img->w + i) = i+j;
			*(actwin_bg_img->pixels + j*actwin_bg_img->w + i) = i+j;
		}
	}
	img8i->refresh(normal_bg_img);
	img8i->refresh(focus_bg_img);
	img8i->refresh(actwin_bg_img);
	
	/* define general widget functions */
	widman->default_widget_methods(&gen_methods);

	orig_update=gen_methods.update;
	orig_handle_event=gen_methods.handle_event;

	gen_methods.draw=but_draw;
	gen_methods.update=but_update;
	gen_methods.handle_event=but_handle_event;


	build_script_lang();

	
	d->register_module("Button 1.0",&services);
	return 1;
}
