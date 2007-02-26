/*
 * \brief   DOpE VScreen widget module
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

struct private_vscreen;
#define VSCREEN struct private_vscreen
#define WIDGET VSCREEN
#define WIDGETARG WIDGET

#include <stdio.h>	/* !!! should be kicked out !!! */

#include "dope-config.h"
#include "event.h"
#include "appman.h"
#include "thread.h"
#include "memory.h"
#include "rtman.h"
#include "widget_data.h"
#include "clipping.h"
#include "widget.h"
#include "img16data.h"
#include "redraw.h"
#include "vscreen.h"
#include "vscr_server.h"
#include "script.h"
#include "sharedmem.h"
#include "widman.h"

#define MAX_IDENTS 40

static struct rtman_services       *rtman;
static struct memory_services      *mem;
static struct widman_services      *widman;
static struct script_services      *script;
static struct redraw_services      *redraw;
static struct appman_services      *appman;
static struct image16_services     *img16;
static struct clipping_services    *clip;
static struct thread_services      *thread;
static struct sharedmem_services   *sharedmem;
static struct vscr_server_services *vscr_server;

VSCREEN {
	/* entry must point to a general widget interface */
	struct widget_methods  *gen;    /* for public access */
	
	/* entry is for the ones who knows the real widget identity */
	struct vscreen_methods *vscr;   /* for dedicated users */
	
	/* entry contains general widget data */
	struct widget_data     *wd; 
	
	/* here comes the private vscreen specific data */
	long    update_flags;
	u8     *server_ident;       /* associated vscreen server identifier */
	void   *server_tid;
	MUTEX  *sync_mutex;
	s8      bpp;                /* bits per pixel */
	s32     xres, yres;         /* virtual screen dimensions */
	s32     bytes_per_pixel;    /* bytes per pixel */
	s32     bytes_per_line;     /* bytes per line */
	s16     fps;                /* frames per second */
	SHAREDMEM *smb;             /* shared memory block */
	u8      smb_ident[64];      /* shared memory block identifier */
	void   *pixels;             /* pointer to page aligned pixel buffer */
	void   *orig_pixels;        /* pointer to original image pixels */
	void   *image;              /* image representation (for drawing) */
	s16     update_mode;
};

int init_vscreen(struct dope_services *d);



/******************************/
/*** GENERAL WIDGET METHODS ***/
/******************************/

static void vscreen_draw(VSCREEN *vs,long x,long y) {

	if (!vs->image) return;

	x+=vs->wd->x;
	y+=vs->wd->y;

	clip->push(x, y, x+vs->wd->w-1, y+vs->wd->h-1);
	if ((vs->xres == vs->wd->w) && (vs->yres == vs->wd->h)) {
		img16->paint(x,y,vs->image);
	} else {
		img16->paint_scaled(x, y, vs->wd->w, vs->wd->h, vs->image);
	}
	clip->pop();
}


static void (*orig_update) (VSCREEN *vs,u16 redraw_flag);

static void vscreen_update(VSCREEN *vs,u16 redraw_flag) {
	orig_update(vs,redraw_flag);
	vs->update_flags=0;
}



/********************************/
/*** VSCREEN SPECIFIC METHODS ***/
/********************************/


/*** REGISTER VSCREEN WIDGET SERVER ***/
static void vscreen_reg_server(VSCREEN *vs, u8 *new_server_ident) {
	if (!vs || !new_server_ident) return;
	printf("Vscreen(reg_server): register Vscreen server with ident=%s\n",new_server_ident);
	vs->server_ident = new_server_ident;
}


/*** RETURN VSCREEN WIDGET SERVER ***/
static u8 *vscreen_get_server(VSCREEN *vs) {
	if ((!vs) || (!vs->server_ident)) return "<noserver>";
	printf("VScreen(get_server): server_ident = %s\n",vs->server_ident);
	return vs->server_ident;
}


/*** SET UPDATE RATE OF THE VSCREEN WIDGET ***/
static void vscreen_set_framerate(VSCREEN *vs, s32 framerate) {
	if (!vs) return;
	if (framerate == 25) {
		rtman->add(vs,0.0);
		rtman->set_sync_mutex(vs,vs->sync_mutex);
		vs->fps = framerate;
	} else {
		rtman->remove(vs);
	}
}


/*** REQUEST CURRENT UPDATE RATE ***/
static s32 vscreen_get_framerate(VSCREEN *vs) {
	if (!vs) return 0;
	return vs->fps;
}


/*** TEST IF A GIVEN GRAPHICS MODE IS VALID ***/
static s32 vscreen_probe_mode(VSCREEN *vs,s32 width, s32 height, s32 depth) {
	if (!vs) return 0;
	if (depth != 16) return 0;
	if (width*height <= 0) return 0;
	return 1;
}


/*** SET GRAPHICS MODE ***/
static s32 vscreen_set_mode(VSCREEN *vs,s32 width, s32 height, s32 depth) {
	if (!vs) return 0;
	if (!vscreen_probe_mode(vs,width,height,depth)) return 0;

	/* destroy old image buffer and reset values */
	switch (vs->bpp) {
	case 16: if (vs->image) img16->destroy(vs->image);
	}

	vs->bpp    = 0;
	vs->xres   = 0;
	vs->yres   = 0;
	vs->pixels = NULL;
	vs->image  = NULL;

	/* create new frame buffer image */
	switch (depth) {
	case 16:
		if ((vs->image	= img16->create(11,11))) {
			vs->orig_pixels = ((struct image16 *)vs->image)->pixels;
			vs->bpp		= depth;
			vs->xres	= width;
			vs->yres 	= height;
			
			/* allocate data space */
			if (vs->smb) sharedmem->free(vs->smb);
			vs->smb     = sharedmem->alloc(width*height*2);
			vs->pixels  = sharedmem->get_address(vs->smb);
			sharedmem->get_ident(vs->smb, &vs->smb_ident[0]);
					
			INFO(printf("pixels = %lu\n",(adr)vs->pixels));
			
			((struct image16 *)vs->image)->pixels = vs->pixels;
			((struct image16 *)vs->image)->w = width;
			((struct image16 *)vs->image)->h = height;
		} else {
			ERROR(printf("VScreen(set_mode): out of memory!\n");)
			return 0;
		}
	}
	
	vs->gen->set_w(vs,width);
	vs->gen->set_h(vs,height);
	vs->gen->update(vs,1); 
	return 1;
}


/*** WAIT FOR THE NEXT SYNC ***/
static void vscreen_waitsync(VSCREEN *vs) {
	thread->mutex_down(vs->sync_mutex);
}


/*** UPDATE A SPECIFIED AREA OF THE WIDGET ***/
static void vscreen_refresh(VSCREEN *vs, s32 x, s32 y, s32 w, s32 h) {
	if (w == -1) w = vs->wd->w;
	if (h == -1) h = vs->wd->h;
	if ((w<1) || (h<1)) return;
	redraw->draw_widgetarea(vs, x, y, x+w-1, y+h-1);
}


/*** MAP VSCREEN BUFFER TO ANOTHER THREAD'S ADDRESS SPACE ***/
static u8 *vscreen_map(VSCREEN *vs,u8 *dst_thread_ident) {
	s32 app_id;
	THREAD *dst_th = thread->ident2thread(dst_thread_ident);

	/* if no valid thread identifier was suppied we map to the app's thread */
	if (!dst_th) {
		app_id = vs->gen->get_app_id(vs);
		dst_th = appman->get_app_thread(app_id);
	}
	sharedmem->share(vs->smb, dst_th);
	INFO(printf("VScreen(map): return vs->smb_ident = %s\n", &vs->smb_ident[0]));
	return &vs->smb_ident[0];
}


static struct widget_methods gen_methods;
static struct vscreen_methods  vscr_methods={
	vscreen_reg_server,
	vscreen_get_server,
	vscreen_set_framerate,
	vscreen_get_framerate,
	vscreen_probe_mode,
	vscreen_set_mode,
	vscreen_waitsync,
	vscreen_map,
	vscreen_refresh,
};



/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

static VSCREEN *create(void) {
	
	/* allocate memory for new widget */
	VSCREEN *new = (VSCREEN *)mem->alloc(sizeof(VSCREEN) +
	                                     sizeof(struct widget_data));
	if (!new) {
		ERROR(printf("VSCREEN(create): out of memory\n"));
		return NULL;
	}
	new->gen  = &gen_methods;       /* pointer to general widget methods */
	new->vscr = &vscr_methods;      /* pointer to vscreen specific methods */

	/* set general widget attributes */
	new->wd = (struct widget_data *)((long)new + sizeof(VSCREEN));
	widman->default_widget_data(new->wd);
		
	new->update_flags = 0;
	new->server_ident = NULL;
	new->sync_mutex = thread->create_mutex(1);  /* locked */ 
	new->bpp    = 0;
	new->xres   = 0;
	new->yres   = 0;
	new->pixels = NULL;
	new->image  = NULL;
	new->smb    = NULL;
	new->fps    = 0;

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

	script->reg_widget_method(widtype,"long probemode(long width,long height,long depth)",vscreen_probe_mode);
	script->reg_widget_method(widtype,"long setmode(long width,long height,long depth)",vscreen_set_mode);
	script->reg_widget_method(widtype,"string getserver()",vscreen_get_server);
	script->reg_widget_method(widtype,"string map(string thread=\"caller\")",vscreen_map);
	script->reg_widget_method(widtype,"void refresh(long x=0, long y=0, long w=-1, long h=-1)",vscreen_refresh);
	
	script->reg_widget_attrib(widtype,"long framerate",vscreen_get_framerate,vscreen_set_framerate,gen_methods.update);
	widman->build_script_lang(widtype,&gen_methods);
}


int init_vscreen(struct dope_services *d) {

	mem         = d->get_module("Memory 1.0");
	clip        = d->get_module("Clipping 1.0");
	img16       = d->get_module("Image16Data 1.0");
	script      = d->get_module("Script 1.0");
	widman      = d->get_module("WidgetManager 1.0");
	redraw      = d->get_module("RedrawManager 1.0");
	appman      = d->get_module("ApplicationManager 1.0");
	thread      = d->get_module("Thread 1.0");
	rtman       = d->get_module("RTManager 1.0");
	sharedmem   = d->get_module("SharedMemory 1.0");
	vscr_server = d->get_module("VScreenServer 1.0");

	/* define general widget functions */
	widman->default_widget_methods(&gen_methods);

	orig_update=gen_methods.update;
	gen_methods.draw   = vscreen_draw;
	gen_methods.update = vscreen_update;

	build_script_lang();

	d->register_module("VScreen 1.0",&services);
	return 1;
}
