/*
 * \brief   DOpE VScreen widget module
 * \date    2002-11-13
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

struct vscreen;
#define WIDGET struct vscreen

#include "dopestd.h"
#include "event.h"
#include "appman.h"
#include "thread.h"
#include "rtman.h"
#include "widget_data.h"
#include "gfx.h"
#include "widget.h"
#include "redraw.h"
#include "vscreen.h"
#include "vscr_server.h"
#include "fontman.h"
#include "script.h"
#include "widman.h"
#include "input.h"
#include "userstate.h"
#include "messenger.h"

#define MAX_IDENTS 40

#define VSCR_MOUSEMODE_FREE    0
#define VSCR_MOUSEMODE_GRAB    1
#define VSCR_MOUSEMODE_GRABBED 2

#define VSCR_UPDATE_MOUSE_X   0x1
#define VSCR_UPDATE_MOUSE_Y   0x2

static struct rtman_services       *rtman;
static struct input_services       *input;
static struct widman_services      *widman;
static struct script_services      *script;
static struct redraw_services      *redraw;
static struct appman_services      *appman;
static struct gfx_services         *gfx;
static struct messenger_services   *msg;
static struct userstate_services   *userstate;
static struct thread_services      *thread;
static struct vscr_server_services *vscr_server;
static struct fontman_services     *font;

struct vscreen_data {
	long    update_flags;
	u8     *server_ident;        /* associated vscreen server identifier */
	void   *server_tid;
	MUTEX  *sync_mutex;
	s8      bpp;                 /* bits per pixel */
	s32     xres, yres;          /* virtual screen dimensions */
	s16     fps;                 /* frames per second */
	u8      smb_ident[64];       /* shared memory block identifier */
	void   *pixels;              /* pointer to page aligned pixel buffer */
	GFX_CONTAINER *image;        /* image representation (for drawing) */
	s16     grabmouse;           /* mouse grab mode flag 0=free 1=grab 2=grabbed */
	VSCREEN *share_next;         /* next vscreen widget with shared buffer */
};

static int msg_cnt, msg_fid;    /* fade counter and font id of onsceen message */
static int msg_x, msg_y;        /* position of onscreen message */
static int msg_w, msg_h;        /* size of onscreen message area */
static char *msg_txt;           /* text of the onscreen message */

static int vs_mx, vs_my;        /* mouse position of grabbed mouse */

int init_vscreen(struct dope_services *d);


/*********************
 * UTILITY FUNCTIONS *
 *********************/

/*** EXCLUDE VSCREEN WIDGET FROM RING LIST OF VSCREENS WITH A SHARED BUFFER ***/
static void vscr_share_exclude(VSCREEN *vs) {
	VSCREEN *prev = vs->vd->share_next;
	while ((prev) && (prev->vd->share_next!=vs)) prev = prev->vd->share_next;
	if (prev) prev->vd->share_next = vs->vd->share_next;
	vs->vd->share_next = NULL;
}


/*** INCLUDE NEW VSCREEN IN RING LIST OF VSCREENS WITH A SHARED BUFFER ***
 *
 * \param vs   member of the new desired ring list
 * \param new  new vscreen widget to join the list
 */
static void vscr_share_join(VSCREEN *vs, VSCREEN *new) {
	if (new->vd->share_next) vscr_share_exclude(new);
	new->vd->share_next = vs->vd->share_next;
	vs->vd->share_next = new;
	if (!new->vd->share_next) new->vd->share_next = vs;
}


/**************************
 * GENERAL WIDGET METHODS *
 **************************/

/*** DRAW VSCREEN WIDGET ***/
static void vscr_draw(VSCREEN *vs,struct gfx_ds *ds,long x,long y) {

	x+=vs->wd->x;
	y+=vs->wd->y;
	gfx->push_clipping(ds,x, y, vs->wd->w, vs->wd->h);
	if (vs->vd->image) {
		gfx->draw_img(ds,x,y,vs->wd->w,vs->wd->h,vs->vd->image,255);
	}
	if ((vs->vd->grabmouse == VSCR_MOUSEMODE_GRABBED) && (msg_cnt)) {
		int v=msg_cnt;
		gfx->draw_string(ds,x+msg_x-1, y+msg_y, GFX_RGB(0,0,0), 0, msg_fid, msg_txt);
		gfx->draw_string(ds,x+msg_x+1, y+msg_y, GFX_RGB(0,0,0), 0, msg_fid, msg_txt);
		gfx->draw_string(ds,x+msg_x, y+msg_y-1, GFX_RGB(0,0,0), 0, msg_fid, msg_txt);
		gfx->draw_string(ds,x+msg_x, y+msg_y+1, GFX_RGB(0,0,0), 0, msg_fid, msg_txt);
		gfx->draw_string(ds,x+msg_x, y+msg_y, GFX_RGB(v,v,v), 0, msg_fid, msg_txt);
	}
	gfx->pop_clipping(ds);
}


static void (*orig_update) (VSCREEN *vs,u16 redraw_flag);

/*** UPDATE WIDGET AFTER CHANGES OF ITS ATTRIBUTES ***/
static void vscr_update(VSCREEN *vs,u16 redraw_flag) {
	if (vs->vd->update_flags & (VSCR_UPDATE_MOUSE_X | VSCR_UPDATE_MOUSE_Y)) {
		if (!(vs->vd->update_flags & VSCR_UPDATE_MOUSE_X)) vs_mx = input->get_mx();
		if (!(vs->vd->update_flags & VSCR_UPDATE_MOUSE_Y)) vs_my = input->get_my();
		input->set_pos(vs_mx, vs_my);
	}
	orig_update(vs,redraw_flag);
	vs->vd->update_flags=0;
}


/*** TIMER TICK CALLBACK FOR GRAB USERSTATE ***
 *
 * This callback is used to fade out the mouse-release-message.
 * The variable msg_cnt is set to 255 when the mouse is grabbed and faded
 * down to zero. For each step the part of the widget which displays the
 * message is redrawn.
 */
static void grab_tick_callback(WIDGET *w, int dx, int dy) {
	if (msg_cnt > 70) {
		msg_cnt -= 2;
		if (w->vd->fps == 0) redraw->draw_widgetarea(w, msg_x-1, msg_y-1, msg_x+msg_w+1, msg_y+msg_h+1);
	} else if (msg_cnt > 0) {
		msg_cnt = 0;
		if (w->vd->fps == 0) redraw->draw_widgetarea(w, msg_x-1, msg_y-1, msg_x+msg_w+1, msg_y+msg_h+1);
	} else {
		msg_cnt = 0;
	}
}


static void (*orig_handle_event)(WIDGET *w, EVENT *);

/*** HANDLE EVENTS ***
 *
 * We have to take care about the mouse grab mode of the VScreen widget.
 * In grab-mode the mouse is grabbed by any press event onto the widget.
 * When grabbed, the mouse can be discharged by pressing [pause]. All
 * other events go through the normal way.
 */
static void vscr_handle_event(VSCREEN *vs, EVENT *e) {
	u8 *m;
	s32 app_id;
	if (!vs || !vs->vd) return;
	if (e->type == EVENT_PRESS) {
		/* transition from grabbed to grab */
		if (vs->vd->grabmouse == VSCR_MOUSEMODE_GRABBED) {
			if (e->code == KEY_PAUSE) {
				vs->vd->grabmouse = VSCR_MOUSEMODE_GRAB;
				m = vs->gen->get_bind_msg(vs,"discharge");
				app_id = vs->gen->get_app_id(vs);
				if (m) msg->send_action_event(app_id,"discharge",m);
				msg_cnt = 0;
				redraw->draw_widgetarea(vs, msg_x, msg_y, msg_x + msg_w, msg_y + msg_h);
				userstate->idle();
				return;
			}
		/* transition to grabbed mouse */
		} else if (vs->vd->grabmouse == VSCR_MOUSEMODE_GRAB) {
			vs->vd->grabmouse = VSCR_MOUSEMODE_GRABBED;
			m = vs->gen->get_bind_msg(vs,"catch");
			app_id = vs->gen->get_app_id(vs);
			if (m) msg->send_action_event(app_id,"catch",m);
			msg_x = (vs->wd->w - msg_w)/2;
			msg_y = (vs->wd->h - msg_h)/2;
			userstate->grab(vs, grab_tick_callback);
			msg_cnt = 255;
			return;
		}
	}
	orig_handle_event(vs,e);
}


/*** DESTROY VSCREEN ***
 *
 * Eventually, we have to exclude the vscreen from
 * the ring list of vscreens which share one buffer.
 */
static void (*orig_destroy)(WIDGET *);
static void vscr_destroy(VSCREEN *vs) {
	if (vs->wd->ref_cnt == 0) vscr_share_exclude(vs);
	orig_destroy(vs);
}


/****************************
 * VSCREEN SPECIFIC METHODS *
 ****************************/

/*** REGISTER VSCREEN WIDGET SERVER ***/
static void vscr_reg_server(VSCREEN *vs, u8 *new_server_ident) {
	if (!vs || !vs->vd || !new_server_ident) return;
	printf("Vscreen(reg_server): register Vscreen server with ident=%s\n",new_server_ident);
	vs->vd->server_ident = new_server_ident;
}


/*** RETURN VSCREEN WIDGET SERVER ***/
static u8 *vscr_get_server(VSCREEN *vs) {
	if (!vs || !vs->vd || !vs->vd->server_ident) return "<noserver>";
	printf("VScreen(get_server): server_ident = %s\n",vs->vd->server_ident);
	return vs->vd->server_ident;
}


/*** SET UPDATE RATE OF THE VSCREEN WIDGET ***/
static void vscr_set_framerate(VSCREEN *vs, s32 framerate) {
	if (!vs || !vs->vd) return;
	if (framerate == 25) {
		rtman->add(vs,0.0);
		rtman->set_sync_mutex(vs,vs->vd->sync_mutex);
		vs->vd->fps = framerate;
	} else {
		rtman->remove(vs);
	}
}


/*** REQUEST CURRENT UPDATE RATE ***/
static s32 vscr_get_framerate(VSCREEN *vs) {
	if (!vs || !vs->vd) return 0;
	return vs->vd->fps;
}


/*** SET VSCREEN MOUSE X POSITION ***/
static void vscr_set_mx(VSCREEN *vs, s32 new_mx) {
	if (!vs || !vs->vd || (vs->vd->grabmouse != VSCR_MOUSEMODE_GRABBED)) return;
	vs_mx = new_mx + vs->gen->get_abs_x(vs);
	vs->vd->update_flags |= VSCR_UPDATE_MOUSE_X;
}


/*** REQUEST VSCREEN MOUSE X POSITION ***/
static s32 vscr_get_mx(VSCREEN *vs) {
	if (!vs || !vs->vd || (vs->vd->grabmouse != VSCR_MOUSEMODE_GRABBED)) return 0;
	return vs_mx - vs->gen->get_abs_x(vs);
}


/*** SET VSCREEN MOUSE Y POSITION ***/
static void vscr_set_my(VSCREEN *vs, s32 new_my) {
	if (!vs || !vs->vd || (vs->vd->grabmouse != VSCR_MOUSEMODE_GRABBED)) return;
	vs_my = new_my + vs->gen->get_abs_y(vs);
	vs->vd->update_flags |= VSCR_UPDATE_MOUSE_Y;
}


/*** REQUEST VSCREEN MOUSE Y POSITION ***/
static s32 vscr_get_my(VSCREEN *vs) {
	if (!vs || !vs->vd || (vs->vd->grabmouse != VSCR_MOUSEMODE_GRABBED)) return 0;
	return vs_my - vs->gen->get_abs_y(vs);
}


/*** SET MOUSE GRAB MODE OF THE VSCREEN WIDGET ***/
static void vscr_set_grabmouse(VSCREEN *vs, s32 grab_flag) {
	if (!vs || !vs->vd) return;
	if (grab_flag) {
		vs->vd->grabmouse = VSCR_MOUSEMODE_GRAB;
	} else {
		vs->vd->grabmouse = VSCR_MOUSEMODE_FREE;
	}
}


/*** REQUEST MOUSE GRAB MODE ***/
static s32 vscr_get_grabmouse(VSCREEN *vs) {
	if (!vs || !vs->vd) return 0;
	if (vs->vd->grabmouse == VSCR_MOUSEMODE_FREE) return 0;
	return 1;
}


/*** TEST IF A GIVEN GRAPHICS MODE IS VALID ***/
static s32 vscr_probe_mode(VSCREEN *vs,s32 width, s32 height, char *mode) {
	if (!vs) return 0;
	if ((!dope_streq(mode,"RGB16",6)) && (!dope_streq(mode,"YUV420",7))) return 0;
	if (width*height <= 0) return 0;
	return 1;
}


/*** SET GRAPHICS MODE ***/
static s32 vscr_set_mode(VSCREEN *vs,s32 width, s32 height, char *mode) {
	int type = 0;

	if (!vs || !vs->vd) return 0;
	if (!vscr_probe_mode(vs,width,height,mode)) return 0;

	/* destroy old image buffer and reset values */
	switch (vs->vd->bpp) {
	case 16: if (vs->vd->image) gfx->destroy(vs->vd->image);
	}

	vs->vd->bpp    = 0;
	vs->vd->xres   = 0;
	vs->vd->yres   = 0;
	vs->vd->pixels = NULL;
	vs->vd->image  = NULL;

	/* create new frame buffer image */
	if (dope_streq("RGB16", mode,6)) type = GFX_IMG_TYPE_RGB16;
	if (dope_streq("YUV420",mode,7)) type = GFX_IMG_TYPE_YUV420;

	if (!type) {
		ERROR(printf("VScreen(set_mode): mode %s not supported!\n", mode);)
		return 0;
	}

	if ((vs->vd->image = gfx->alloc_img(width, height, type))) {
		vs->vd->xres   = width;
		vs->vd->yres   = height;
		vs->vd->pixels = gfx->map(vs->vd->image);
		gfx->get_ident(vs->vd->image, &vs->vd->smb_ident[0]);
		INFO(printf("pixels = %lu\n",(adr)vs->vd->pixels));
	} else {
		ERROR(printf("VScreen(set_mode): out of memory!\n");)
		return 0;
	}

	vs->gen->set_w(vs,width);
	vs->gen->set_h(vs,height);
	vs->gen->update(vs,1);
	return 1;
}


/*** WAIT FOR THE NEXT SYNC ***/
static void vscr_waitsync(VSCREEN *vs) {
	thread->mutex_down(vs->vd->sync_mutex);
}


/*** UPDATE A SPECIFIED AREA OF THE WIDGET ***/
static void vscr_refresh(VSCREEN *vs, s32 x, s32 y, s32 w, s32 h) {
	VSCREEN *last = vs;
	int sx1, sy1, sx2, sy2;
	float mx,my;

	int cnt = 1;
	if (vs->vd->share_next) {
		while ((vs = vs->vd->share_next) != last) cnt++;
	}
	
	if (w == -1) w = vs->vd->xres;
	if (h == -1) h = vs->vd->yres;
	if ((w<1) || (h<1)) return;
	
//	mx = (float)vs->wd->w / (float)vs->vd->xres;
//	my = (float)vs->wd->h / (float)vs->vd->yres;
//	sx1 = x*mx;
//	sy1 = y*my;
//	sx2 = (int)((x+w)*mx);
//	sy2 = (int)((y+h)*my);
//	
//	redraw->draw_widgetarea(vs, sx1, sy1, sx2, sy2);
//	vs = vs->vd->share_next;
//	
	/* refresh all vscreens which share the same pixel buffer */
//	while (vs && (vs != last)) {
	while (cnt--) {
		
		mx = (float)vs->wd->w / (float)vs->vd->xres;
		my = (float)vs->wd->h / (float)vs->vd->yres;
		sx1 = x*mx;
		sy1 = y*my;
		sx2 = (int)((x+w)*mx);
		sy2 = (int)((y+h)*my);
		redraw->draw_widgetarea(vs, sx1, sy1, sx2, sy2);
		vs = vs->vd->share_next;
	}
}


/*** MAP VSCREEN BUFFER TO ANOTHER THREAD'S ADDRESS SPACE ***/
static u8 *vscr_map(VSCREEN *vs,u8 *dst_thread_ident) {
	s32 app_id;
	char dst_th_buf[16];
	THREAD *dst_th = (THREAD *)dst_th_buf;
	
	if (!vs || !vs->vd) return "Error: Widget does not exist.";
	if (!vs->vd->image) return "Error: VScreen mode not initialized.";

	/* if no valid thread identifier was suppied we map to the app's thread */
	if (thread->ident2thread(dst_thread_ident, dst_th)) {
		app_id = vs->gen->get_app_id(vs);
		dst_th = appman->get_app_thread(app_id);
	}
	gfx->share(vs->vd->image, dst_th);
	INFO(printf("VScreen(map): return vs->vd->smb_ident = %s\n", &vs->vd->smb_ident[0]));
	return &vs->vd->smb_ident[0];
}


/*** SHARE IMAGE BUFFER WITH OTHER SPECIFIED VSCREEN ***/
static void vscr_share(VSCREEN *vs,VSCREEN *from) {
	int img_type = 0;

	printf("share 0x%x\n",(int)vs);
	/* exclude widget from its previous ring list of buffer-shared vscreen */
	if (vs->vd->share_next) vscr_share_exclude(vs);
	
	if (vs->vd->image) gfx->destroy(vs->vd->image);
	vs->vd->image = from->vscr->get_image(from);
	if (!vs->vd->image) return;

	vs->vd->xres = gfx->get_width(vs->vd->image);
	vs->vd->yres = gfx->get_height(vs->vd->image);
	img_type = gfx->get_type(vs->vd->image);
	switch (img_type) {
	case GFX_IMG_TYPE_RGB16:
		vs->vd->bpp = 16;
		break;
	case GFX_IMG_TYPE_YUV420:
		vs->vd->bpp = 12;
		break;
	}
	vs->vd->pixels = gfx->map(vs->vd->image);

	vs->gen->set_w(vs,vs->vd->xres);
	vs->gen->set_h(vs,vs->vd->yres);
	vs->gen->update(vs,1);

	/* join the new ring list of buffer sharing vscreens */
	vscr_share_join(from, vs);
}


static GFX_CONTAINER *vscr_get_image(VSCREEN *vs) {
	if (!vs || !vs->vd) return NULL;
	return vs->vd->image;
}


static struct widget_methods gen_methods;
static struct vscreen_methods vscreen_methods = {
	vscr_reg_server,
	vscr_get_server,
	vscr_set_framerate,
	vscr_get_framerate,
	vscr_set_grabmouse,
	vscr_get_grabmouse,
	vscr_probe_mode,
	vscr_set_mode,
	vscr_waitsync,
	vscr_map,
	vscr_refresh,
	vscr_get_image,
};



/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

static VSCREEN *create(void) {

	/* allocate memory for new widget */
	VSCREEN *new = (VSCREEN *)malloc(sizeof(struct vscreen)
	                               + sizeof(struct widget_data)
	                               + sizeof(struct vscreen_data));
	if (!new) {
		ERROR(printf("VScreen(create): out of memory\n"));
		return NULL;
	}
	new->gen  = &gen_methods;       /* pointer to general widget methods */
	new->vscr = &vscreen_methods;      /* pointer to vscreen specific methods */
	new->wd   = (struct widget_data *)((long)new + sizeof(struct vscreen));
	new->vd   = (struct vscreen_data *)((long)new->wd + sizeof(struct widget_data));
	
	/* set general widget attributes */
	widman->default_widget_data(new->wd);

	/* set widget type specific data */
	memset(new->vd, 0, sizeof(struct vscreen_data));
	new->vd->sync_mutex = thread->create_mutex(1);  /* locked */

	/* start widget server */
	if (vscr_server) vscr_server->start(new);

	return new;
}



/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct vscreen_services services = {
	create
};



/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

static void build_script_lang(void) {
	void *widtype;

	widtype = script->reg_widget_type("VScreen",(void *(*)(void))create);

	script->reg_widget_method(widtype,"long probemode(long width,long height,string mode)",vscr_probe_mode);
	script->reg_widget_method(widtype,"long setmode(long width,long height,string mode)",vscr_set_mode);
	script->reg_widget_method(widtype,"string getserver()",vscr_get_server);
	script->reg_widget_method(widtype,"string map(string thread=\"caller\")",vscr_map);
	script->reg_widget_method(widtype,"void refresh(long x=0, long y=0, long w=-1, long h=-1)",vscr_refresh);
	script->reg_widget_method(widtype,"void share(Widget from)",vscr_share);

	script->reg_widget_attrib(widtype,"long framerate",vscr_get_framerate,vscr_set_framerate,gen_methods.update);
	script->reg_widget_attrib(widtype,"long mx",vscr_get_mx,vscr_set_mx,gen_methods.update);
	script->reg_widget_attrib(widtype,"long my",vscr_get_my,vscr_set_my,gen_methods.update);
	script->reg_widget_attrib(widtype,"boolean grabmouse",vscr_get_grabmouse,vscr_set_grabmouse,gen_methods.update);
	widman->build_script_lang(widtype,&gen_methods);
}


int init_vscreen(struct dope_services *d) {

	gfx         = d->get_module("Gfx 1.0");
	script      = d->get_module("Script 1.0");
	widman      = d->get_module("WidgetManager 1.0");
	redraw      = d->get_module("RedrawManager 1.0");
	appman      = d->get_module("ApplicationManager 1.0");
	thread      = d->get_module("Thread 1.0");
	rtman       = d->get_module("RTManager 1.0");
	vscr_server = d->get_module("VScreenServer 1.0");
	msg         = d->get_module("Messenger 1.0");
	input       = d->get_module("Input 1.0");
	userstate   = d->get_module("UserState 1.0");
	font        = d->get_module("FontManager 1.0");

	/* define general widget functions */
	widman->default_widget_methods(&gen_methods);

	orig_update         = gen_methods.update;
	orig_destroy        = gen_methods.destroy;
	orig_handle_event   = gen_methods.handle_event;
	
	gen_methods.draw    = vscr_draw;
	gen_methods.update  = vscr_update;
	gen_methods.destroy = vscr_destroy;
	gen_methods.handle_event = vscr_handle_event;

	build_script_lang();

	msg_fid = 0;
	msg_txt = "press [pause] to release mouse";
	msg_w = font->calc_str_width(msg_fid, msg_txt);
	msg_h = font->calc_str_height(msg_fid, msg_txt);

	d->register_module("VScreen 1.0",&services);
	return 1;
}
