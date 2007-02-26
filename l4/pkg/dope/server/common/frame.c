/*
 * \brief	DOpE Frame widget module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 */


struct private_frame;
#define FRAME struct private_frame
#define WIDGET FRAME
#define WIDGETARG WIDGET

#include "dope-config.h"
#include "memory.h"
#include "script.h"
#include "widget_data.h"
#include "widget.h"
#include "scrollbar.h"
#include "background.h"
#include "clipping.h"
#include "frame.h"
#include "widman.h"
#include "basegfx.h"


static struct memory_services 		*mem;
static struct widman_services 		*widman;
static struct scrollbar_services	*scroll;
static struct clipping_services		*clip;
static struct background_services	*bg;
static struct basegfx_services		*basegfx;
static struct script_services		*script;

#define FRAME_MODE_FREE 0x00	/* frame size is indepentend of its content */
#define FRAME_MODE_FITX 0x01	/* content fits in frame horizontally */
#define FRAME_MODE_FITY 0x02	/* content fits in frame vertically */
#define FRAME_MODE_SCRX 0x04	/* horizontal scrollbars */
#define FRAME_MODE_SCRY 0x08	/* vertical scrollbars */
#define FRAME_MODE_HIDX 0x10	/* auto-hiding of horizontal scrollbar */
#define FRAME_MODE_HIDY 0x20	/* auto-hiding of vertical scrollbar */


#define FRAME_UPDATE_SCRX 			0x01
#define	FRAME_UPDATE_SCRY			0x02
#define FRAME_UPDATE_NEW_CONTENT	0x04
#define FRAME_UPDATE_SCR_CONF		0x08

FRAME {
	/* entry must point to a general widget interface */
	struct widget_methods 	*gen;	/* for public access */
	
	/* entry is for the ones who knows the real widget identity (frame) */
	struct frame_methods 	*frame;	/* for dedicated users */
	
	/* entry contains general widget data */
	struct widget_data		*wd; /* access for frame module and widget manager */
	
	/* here comes the private frame specific data */
	WIDGET		*content;
	u32		 mode;
	u16		 bg_flag;
	s32		 scroll_x,scroll_y;
	SCROLLBAR 	*sb_x;
	SCROLLBAR	*sb_y;
	BACKGROUND	*corner;
	u32		 update_flags;
};

int init_frame(struct dope_services *d);



/******************************/
/*** GENERAL WIDGET METHODS ***/
/******************************/

static void frame_draw(FRAME *f,long x,long y) {
	WIDGET *cw;
	s32 vw,vh;

	if (f) {
		
		vw=f->wd->w;
		if (f->sb_y) vw -= 13;
	
		vh=f->wd->h;
		if (f->sb_x) vh -= 13;

		x += f->wd->x;
		y += f->wd->y;

		/* if content exists, draw it */
		clip->push(x,y,x+vw-1,y+vh-1);
		if (f->bg_flag) {
			basegfx->draw_box(x,y,x+vw-1,y+vh-1,GFX_BOX_WINBG);
		}
		cw=f->content;
		if (cw) cw->gen->draw(cw,x,y);
		clip->pop();
		
		if (f->corner) f->corner->gen->draw((WIDGET *)f->corner,x,y);

		/* draw scrollbars */
		if (f->sb_x) f->sb_x->gen->draw((WIDGET *)f->sb_x,x,y);
		if (f->sb_y) f->sb_y->gen->draw((WIDGET *)f->sb_y,x,y);
	}
}

static WIDGET *frame_find(FRAME *f,long x,long y) {
	WIDGET *result;
	WIDGET *cw;
	
	if (!f) return NULL;

	/* check if position is inside the frame */
	if ((x >= f->wd->x) && (y >= f->wd->y) &&
		(x < f->wd->x+f->wd->w) && (y < f->wd->y+f->wd->h)) {

		/* we are hit - lets check our children */
		if (f->sb_x) {
			result=f->sb_x->gen->find((WIDGET *)f->sb_x,x-f->wd->x,y-f->wd->y);
			if (result) return result;
		}
		if (f->sb_y) {
			result=f->sb_y->gen->find((WIDGET *)f->sb_y,x-f->wd->x, y-f->wd->y);
			if (result) return result;
		}

		cw=f->content;
		if (cw) {
			result=cw->gen->find(cw, x-f->wd->x, y-f->wd->y);
			if (result) return result;
		}
		return f;
	}
	return NULL;	
}


static void frame_update(FRAME *f,u16 redraw_flag) {
	SCROLLBAR *sb;
	s32 vw,vh;	
	WIDGET *c=f->content;
	
	vh=f->wd->h;
	if (f->sb_x) vh -= 13;

	vw=f->wd->w;
	if (f->sb_y) vw -= 13;

	if (f->update_flags & FRAME_UPDATE_NEW_CONTENT) {
		if (c) {
			if (f->mode & FRAME_MODE_FITX) {
				f->wd->min_w = c->gen->get_min_w(c);
				f->wd->max_w = c->gen->get_max_w(c);
			} else {
				f->wd->min_w = 0;
				f->wd->max_w = c->gen->get_w(c);
			}
	
			if (f->mode & FRAME_MODE_FITY) {
				f->wd->min_h = c->gen->get_min_h(c);
				f->wd->max_h = c->gen->get_max_h(c);
			} else {
				f->wd->min_h = 0;
				f->wd->max_h = c->gen->get_h(c);
			}
			
			c->gen->set_x(c,0);
			c->gen->set_y(c,0);
			c->gen->update(c,0);
		}
		f->scroll_x  = 0;
		f->scroll_y  = 0;
		if ((f->mode & FRAME_MODE_FITX) |
			(f->mode & FRAME_MODE_FITY)) {
//			f->wd->update = f->wd->update | WID_UPDATE_SIZE;
		}
	}
	
	if ((f->update_flags & (FRAME_UPDATE_SCRX | FRAME_UPDATE_SCR_CONF)) |
		(f->wd->update & WID_UPDATE_SIZE)) {
	
		if (f->sb_x) {	
			sb=f->sb_x;
			sb->gen->set_x((WIDGET *)sb,0);
			sb->gen->set_y((WIDGET *)sb,vh);
			sb->gen->set_w((WIDGET *)sb,vw);
			sb->gen->set_h((WIDGET *)sb,13);
			sb->scroll->set_type(sb,SCROLLBAR_HOR);
			if (c) sb->scroll->set_real_size(sb,c->gen->get_w(c));
			sb->scroll->set_view_size(sb,vw);
			sb->scroll->set_view_offset(sb,f->scroll_x);
			sb->gen->update((WIDGET *)sb,redraw_flag);
		}
	}
	
	if ((f->update_flags & (FRAME_UPDATE_SCRY | FRAME_UPDATE_SCR_CONF)) |
		(f->wd->update & WID_UPDATE_SIZE)) {

		if (f->sb_y) {
			sb=f->sb_y;
			sb->gen->set_x((WIDGET *)sb,vw);
			sb->gen->set_y((WIDGET *)sb,0);
			sb->gen->set_w((WIDGET *)sb,13);
			sb->gen->set_h((WIDGET *)sb,vh);
			sb->scroll->set_type(sb,SCROLLBAR_VER);
			if (c) sb->scroll->set_real_size(sb,c->gen->get_h(c));
			sb->scroll->set_view_size(sb,vh);
			sb->scroll->set_view_offset(sb,f->scroll_y);
			sb->gen->update((WIDGET *)sb,redraw_flag);
		}
	}

	if ((f->update_flags & FRAME_UPDATE_SCRX) |
		(f->update_flags & FRAME_UPDATE_SCRY) |
		(f->wd->update & WID_UPDATE_SIZE)) {
		
		if (f->corner) {
			f->corner->gen->set_x((WIDGET *)f->corner,vw-1);
			f->corner->gen->set_y((WIDGET *)f->corner,vh-1);
		}
	}

	if ((f->mode & FRAME_MODE_FITX) &&
	   ((f->wd->update & WID_UPDATE_SIZE) || (f->update_flags & FRAME_UPDATE_SCR_CONF))) {
		if (f->content) f->content->gen->set_w(f->content,vw);
	}

	if ((f->mode & FRAME_MODE_FITY) &&
	   ((f->wd->update & WID_UPDATE_SIZE) || (f->update_flags & FRAME_UPDATE_SCR_CONF))) {
		if (f->content) f->content->gen->set_h(f->content,vh);
	}

	if (((f->mode & FRAME_MODE_FITX) ||	(f->mode & FRAME_MODE_FITY)) &&
		((f->wd->update & WID_UPDATE_SIZE) || (f->update_flags & FRAME_UPDATE_SCR_CONF))) {
		if (f->content) f->content->gen->update(f->content,redraw_flag);
	}

	if (f->update_flags & (FRAME_UPDATE_NEW_CONTENT | FRAME_UPDATE_SCR_CONF)) {
		f->gen->force_redraw(f);	
	}
	
	f->update_flags=0;
}


/*** HANDLE CHILD-DEPENDENT LAYOUT CHANGE ***/
static u16 frame_do_layout(FRAME *f,WIDGET *c,u16 redraw_flag) {
	SCROLLBAR *sb;
	if (!f) return 0;
	if (f->content != c) return 0;
//	f->update_flags = f->update_flags | FRAME_UPDATE_NEW_CONTENT;

	if (f->mode & FRAME_MODE_FITX) {
		f->wd->min_w = c->gen->get_min_w(c);
		f->wd->max_w = c->gen->get_max_w(c);
	} else {
		f->wd->min_w = 0;
		f->wd->max_w = c->gen->get_w(c);
	}
	
	if (f->mode & FRAME_MODE_FITY) {
		f->wd->min_h = c->gen->get_min_h(c);
		f->wd->max_h = c->gen->get_max_h(c);
	} else {
		f->wd->min_h = 0;
		f->wd->max_h = c->gen->get_h(c);
	}

	if (f->sb_x) {	
		sb=f->sb_x;
		sb->scroll->set_real_size(sb,c->gen->get_w(c));
		sb->gen->update((WIDGET *)sb,0);
		f->wd->min_h+=sb->scroll->get_arrow_size(sb);
		f->wd->max_h+=sb->scroll->get_arrow_size(sb);
	}

	if (f->sb_y) {
		sb=f->sb_y;
		sb->scroll->set_real_size(sb,c->gen->get_h(c));
		sb->gen->update((WIDGET *)sb,0);
		f->wd->min_w+=sb->scroll->get_arrow_size(sb);
		f->wd->max_w+=sb->scroll->get_arrow_size(sb);
	}

//	frame_update(f,redraw_flag);
	if (redraw_flag) f->gen->force_redraw(f);
	
	/* we did the redrawing job already... a zero tells the calling */
	/* child that it doesnt need to redraw anything */
	return 0;
}


static char *frame_get_type(FRAME *f) {
	return "Frame";
}



/******************************/
/*** FRAME SPECIFIC METHODS ***/
/******************************/

static void frame_set_content(FRAME *f,WIDGET *new_content) {
	if (!f) return;
	if (f->content) {
		f->content->gen->set_parent(f->content,NULL);
		f->content->gen->dec_ref(f->content);
	}
	f->content=new_content;
	if (new_content) {
		new_content->gen->set_parent(new_content,f);
		new_content->gen->inc_ref(new_content);
		f->update_flags = f->update_flags | FRAME_UPDATE_NEW_CONTENT;
	}
}


static WIDGET *frame_get_content(FRAME *f) {
	if (f) return f->content;
	else return NULL;
}


static void frame_set_background(FRAME *f,u32 bg_flag) {
	if (!f) return;
	f->bg_flag = bg_flag;
}


static u32 frame_get_background(FRAME *f) {
	if (f) return f->bg_flag;
	else return 0;
}



static void frame_xscroll_update(FRAME *f,u16 redraw_flag) {
	WIDGET *cw;
	s32 new_scroll_x=0;
	if (!f) return;
	if (f->sb_x) {
		new_scroll_x = f->sb_x->scroll->get_view_offset(f->sb_x);
		if ((cw=f->content)) cw->gen->set_x(cw,cw->gen->get_x(cw) - new_scroll_x + f->scroll_x);
		if (f->sb_x) f->scroll_x = new_scroll_x;
	}
	if (redraw_flag) f->gen->force_redraw(f);
}


static void frame_yscroll_update(FRAME *f,u16 redraw_flag) {
	WIDGET *cw;
	s32 new_scroll_y=0;
	if (!f) return;
	if (f->sb_y) {
		new_scroll_y = f->sb_y->scroll->get_view_offset(f->sb_y);
		if ((cw=f->content)) cw->gen->set_y(cw,cw->gen->get_y(cw) - new_scroll_y + f->scroll_y);
		if (f->sb_y) f->scroll_y = new_scroll_y;
	}
	if (redraw_flag) f->gen->force_redraw(f);
}


static void check_corner(FRAME *f) {
	if (f->sb_x && f->sb_y) {
		if (!f->corner) f->corner=bg->create();
		f->corner->gen->set_w((WIDGET *)f->corner,15);
		f->corner->gen->set_h((WIDGET *)f->corner,15);
	} else {
		if (f->corner) {
			f->corner->gen->dec_ref((WIDGET *)f->corner);
			f->corner=NULL;
		}			
	}
}


static void frame_set_scrollx(FRAME *f,u32 flag) {
	if (flag) {
		f->mode = f->mode | FRAME_MODE_SCRX;
		if (!f->sb_x) {
			f->sb_x = scroll->create();
			f->sb_x->gen->set_parent((WIDGET *)f->sb_x,f);
			f->sb_x->scroll->reg_scroll_update(f->sb_x,(void (*)(void *,u16))frame_xscroll_update,f);
			f->update_flags = f->update_flags | FRAME_UPDATE_SCR_CONF;
		}
	} else {
		f->mode = f->mode & (FRAME_MODE_SCRX^0xffffffff);
		if (f->sb_x) {
			f->sb_x->gen->dec_ref((WIDGET *)f->sb_x);
			f->sb_x=NULL;
			f->update_flags = f->update_flags | FRAME_UPDATE_SCR_CONF;
		}	
	}
	check_corner(f);
}


static s32 frame_get_scrollx(FRAME *f) {
	if (!f) return -1;
	if (f->mode & FRAME_MODE_SCRX) return 1;
	else return 0;
}


static void frame_set_scrolly(FRAME *f,u32 flag) {
	if (flag) {
		f->mode = f->mode | FRAME_MODE_SCRY;
		if (!f->sb_y) {
			f->sb_y = scroll->create();
			f->sb_y->gen->set_parent((WIDGET *)f->sb_y,f);
			f->sb_y->scroll->reg_scroll_update(f->sb_y,(void (*)(void *,u16))frame_yscroll_update,f);
			f->update_flags = f->update_flags | FRAME_UPDATE_SCR_CONF;
		}
	} else {
		f->mode = f->mode & (FRAME_MODE_SCRY^0xffffffff);
		if (f->sb_y) {
			f->sb_y->gen->dec_ref((WIDGET *)f->sb_y);
			f->sb_y=NULL;
			f->update_flags = f->update_flags | FRAME_UPDATE_SCR_CONF;
		}	
	}
	check_corner(f);
}


static s32 frame_get_scrolly(FRAME *f) {
	if (!f) return -1;
	if (f->mode & FRAME_MODE_SCRY) return 1;
	else return 0;
}


static void frame_set_fitx(FRAME *f,u32 flag) {
	if (flag) {
		f->mode = f->mode | FRAME_MODE_FITX;
		f->update_flags = f->update_flags | FRAME_UPDATE_SCR_CONF;
	} else {
		f->mode = f->mode & (FRAME_MODE_FITX^0xffffffff);
		f->update_flags = f->update_flags | FRAME_UPDATE_SCR_CONF;
	}
}


static s32 frame_get_fitx(FRAME *f) {
	if (!f) return -1;
	if (f->mode & FRAME_MODE_FITX) return 1;
	else return 0;
}


static void frame_set_fity(FRAME *f,u32 flag) {
	if (flag) {
		f->mode = f->mode | FRAME_MODE_FITY;
		f->update_flags = f->update_flags | FRAME_UPDATE_SCR_CONF;
	} else {
		f->mode = f->mode & (FRAME_MODE_FITY^0xffffffff);
		f->update_flags = f->update_flags | FRAME_UPDATE_SCR_CONF;
	}
}


static s32 frame_get_fity(FRAME *f) {
	if (!f) return -1;
	if (f->mode & FRAME_MODE_FITY) return 1;
	else return 0;
}


static void frame_set_xview(FRAME *f,s32 new_scroll_x) {
	WIDGET *cw;
	if (!f) return;
	if ((cw=f->content)) cw->gen->set_x(cw,cw->gen->get_x(cw) - new_scroll_x + f->scroll_x);
	f->scroll_x=new_scroll_x;
	f->update_flags = f->update_flags | FRAME_UPDATE_SCRX;
}


static s32 frame_get_xview(FRAME *f) {
	if (!f) return 0;
	return f->scroll_x;
}


static void frame_set_yview(FRAME *f,s32 new_scroll_y) {
	WIDGET *cw;
	if (!f) return;
	if ((cw=f->content)) cw->gen->set_y(cw,cw->gen->get_y(cw) - new_scroll_y + f->scroll_y);
	f->scroll_y=new_scroll_y;
	f->update_flags = f->update_flags | FRAME_UPDATE_SCRY;
}


static s32 frame_get_yview(FRAME *f) {
	if (!f) return 0;
	return f->scroll_y;
}



static struct widget_methods 	gen_methods;
static struct frame_methods 	frame_methods={
	frame_set_content,
	frame_get_content,
	frame_set_scrollx,
	frame_get_scrollx,
	frame_set_scrolly,
	frame_get_scrolly,
	frame_set_fitx,
	frame_get_fitx,
	frame_set_fity,
	frame_get_fity,	
	frame_set_xview,
	frame_get_xview,
	frame_set_yview,
	frame_get_yview,
	frame_set_background,
	frame_get_background,
	frame_xscroll_update,
	frame_yscroll_update
};



/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

static FRAME *create(void) {

	/* allocate memory for new widget */
	FRAME *new = (FRAME *)mem->alloc(sizeof(FRAME)+sizeof(struct widget_data));
	if (!new) {
		DOPEDEBUG(printf("Frame(create): out of memory\n"));
		return NULL;
	}
	new->gen  = &gen_methods;	/* pointer to general widget methods */
	new->frame= &frame_methods;	/* pointer to frame specific methods */

	/* set general widget attributes */
	new->wd = (struct widget_data *)((long)new + sizeof(FRAME));
	widman->default_widget_data(new->wd);
	new->wd->min_w=0;
	new->wd->min_h=0;
	new->wd->max_w=640;
	new->wd->max_h=480;
		
	/* set frame specific default attributes */
	new->content=NULL;
	new->mode=0;
	new->bg_flag=0;
	new->scroll_x=0;
	new->scroll_y=0;
	new->sb_x = NULL;
	new->sb_y = NULL;
	new->corner=NULL;
	new->update_flags=0;
	
	return new;
}



/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct frame_services services = {
	create
};



/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

static void build_script_lang(void) {
	void *widtype;

	widtype = script->reg_widget_type("Frame",(void *(*)(void))create);

	script->reg_widget_attrib(widtype,"Widget content",frame_get_content,frame_set_content,frame_update);
	script->reg_widget_attrib(widtype,"boolean background",frame_get_background,frame_set_background,frame_update);
	script->reg_widget_attrib(widtype,"boolean scrollx",frame_get_scrollx,frame_set_scrollx,frame_update);
	script->reg_widget_attrib(widtype,"boolean scrolly",frame_get_scrolly,frame_set_scrolly,frame_update);
	script->reg_widget_attrib(widtype,"boolean fitx",frame_get_fitx,frame_set_fitx,frame_update);
	script->reg_widget_attrib(widtype,"boolean fity",frame_get_fity,frame_set_fity,frame_update);
	script->reg_widget_attrib(widtype,"long xview",frame_get_xview,frame_set_xview,frame_update);
	script->reg_widget_attrib(widtype,"long yview",frame_get_yview,frame_set_yview,frame_update);

	widman->build_script_lang(widtype,&gen_methods);
}


int init_frame(struct dope_services *d) {

	mem		= d->get_module("Memory 1.0");
	widman	= d->get_module("WidgetManager 1.0");
	scroll	= d->get_module("Scrollbar 1.0");
	bg		= d->get_module("Background 1.0");
	clip	= d->get_module("Clipping 1.0");
	basegfx	= d->get_module("Basegfx 1.0");
	script	= d->get_module("Script 1.0");
	
	/* define general widget functions */
	widman->default_widget_methods(&gen_methods);
	gen_methods.get_type	= frame_get_type;
	gen_methods.draw		= frame_draw;
	gen_methods.find		= frame_find;
	gen_methods.update		= frame_update;
	gen_methods.do_layout	= frame_do_layout;
	
	build_script_lang();
	
	d->register_module("Frame 1.0",&services);
	return 1;
}
