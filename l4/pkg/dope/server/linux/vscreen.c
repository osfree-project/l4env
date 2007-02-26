/*
 * \brief	DOpE VScreen widget module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 */

struct private_vscreen;
#define VSCREEN struct private_vscreen
#define WIDGET VSCREEN
#define WIDGETARG WIDGET

#include <stdio.h>	/* !!! should be kicked out !!! */

/*** DOpE RELATED ***/
#include "dope-config.h"
#include "event.h"
#include "memory.h"
#include "rtman.h"
#include "thread.h"
#include "widget_data.h"
#include "clipping.h"
#include "widget.h"
#include "img16data.h"
#include "redraw.h"
#include "vscreen.h"
#include "script.h"
#include "widman.h"

/*** REQUIRED FOR MMAP ***/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

#include <vscr-server.h>

#define MAX_IDENTS 40

static struct rtman_services		*rtman;
static struct memory_services 		*mem;
static struct widman_services 		*widman;
static struct script_services		*script;
static struct redraw_services		*redraw;
static struct thread_services		*thread;
static struct image16_services		*img16;
static struct clipping_services 	*clip;

static s32 max_server_id = 0;
static s16 thread_started=0;

//static int dummy_argc=1;
//static char *dummy_arg1="dummyname";
//static char **dummy_args=&dummy_arg1;

//static CORBA_ORB			orb;
//static CORBA_Environment	*ev;

VSCREEN {
	/* entry must point to a general widget interface */
	struct widget_methods 	*gen;	/* for public access */
	
	/* entry is for the ones who knows the real widget identity */
	struct vscreen_methods 	*vscr;	/* for dedicated users */
	
	/* entry contains general widget data */
	struct widget_data		*wd; 
	
	/* here comes the private pslim specific data */
	long	update_flags;
	s32		server_id;
	CORBA_Environment env;
	int		fh;					/* file handle for shared memory */
	MUTEX  *sync_mutex;
	s8		bpp;				/* bits per pixel */
	s32  	xres, yres;			/* virtual screen dimensions */
	s32 	bytes_per_pixel;	/* bytes per pixel */
	s32 	bytes_per_line;		/* bytes per line */
	void   *pixels;				/* pointer to page aligned pixel buffer */
	void   *orig_pixels;		/* pointer to original image pixels */
	void   *image;				/* image representation (for drawing) */
	s16	update_mode;
};

int init_vscreen(struct dope_services *d);



/*****************************/
/*** VSCREEN WIDGET SERVER ***/
/*****************************/


CORBA_long dope_vscr_map_component(CORBA_Object *_dice_corba_obj,
                                   CORBA_char_ptr *ident,
                                   CORBA_Environment *_dice_corba_env) {
	static u8 ident_buf[32];
	VSCREEN *vs = (VSCREEN *)_dice_corba_env->user_data;
	sprintf(ident_buf,"/tmp/dopevscr%lu.map",vs->server_id);
	return ident_buf;
}

CORBA_void dope_vscr_update_component(CORBA_Object *_dice_corba_obj,
                                      CORBA_long x,
                                      CORBA_long y,
                                      CORBA_long w,
                                      CORBA_long h,
                                      CORBA_Environment *_dice_corba_env) {
}

CORBA_void dope_vscr_waitsync_component(CORBA_Object *_dice_corba_obj,
                                        CORBA_Environment *_dice_corba_env) {
}


///*** App-specific servant structures ***/
//
//typedef struct
//{
//   POA_dope_vscr servant;
//   PortableServer_POA poa;
//
//} impl_POA_dope_vscr;
//
///*** Implementation stub prototypes ***/
///*
//static void impl_dope_vscr__destroy(impl_POA_dope_vscr * servant,
//				       CORBA_Environment * ev);
//*/					   
//static CORBA_long
//impl_dope_vscr_map(impl_POA_dope_vscr * servant,
//		      CORBA_char ** ident, CORBA_Environment * ev);
//
//static void
//impl_dope_vscr_update(impl_POA_dope_vscr * servant,
//			 CORBA_long x,
//			 CORBA_long y,
//			 CORBA_long w, CORBA_long h, CORBA_Environment * ev);
//
//static void
//impl_dope_vscr_waitsync(impl_POA_dope_vscr * servant,
//			   CORBA_Environment * ev);
//
///*** epv structures ***/
//
//static PortableServer_ServantBase__epv impl_dope_vscr_base_epv = {
//   NULL,			/* _private data */
//   NULL,			/* finalize routine */
//   NULL,			/* default_POA routine */
//};
//static POA_dope_vscr__epv impl_dope_vscr_epv = {
//   NULL,			/* _private */
//   (gpointer) & impl_dope_vscr_map,
//
//   (gpointer) & impl_dope_vscr_update,
//
//   (gpointer) & impl_dope_vscr_waitsync,
//
//};
//
///*** vepv structures ***/
//
//static POA_dope_vscr__vepv impl_dope_vscr_vepv = {
//   &impl_dope_vscr_base_epv,
//   &impl_dope_vscr_epv,
//};
//
///*** Stub implementations ***/
//
//static dope_vscr
//impl_dope_vscr__create(PortableServer_POA poa, CORBA_Environment * ev)
//{
//   dope_vscr retval;
//   impl_POA_dope_vscr *newservant;
//   PortableServer_ObjectId *objid;
//
//   newservant = g_new0(impl_POA_dope_vscr, 1);
//   newservant->servant.vepv = &impl_dope_vscr_vepv;
//   newservant->poa = poa;
//   POA_dope_vscr__init((PortableServer_Servant) newservant, ev);
//   objid = PortableServer_POA_activate_object(poa, newservant, ev);
//   CORBA_free(objid);
//   retval = PortableServer_POA_servant_to_reference(poa, newservant, ev);
//
//   return retval;
//}
//
///*
//static void
//impl_dope_vscr__destroy(impl_POA_dope_vscr * servant,
//			   CORBA_Environment * ev)
//{
//   PortableServer_ObjectId *objid;
//
//   objid = PortableServer_POA_servant_to_id(servant->poa, servant, ev);
//   PortableServer_POA_deactivate_object(servant->poa, objid, ev);
//   CORBA_free(objid);
//
//   POA_dope_vscr__fini((PortableServer_Servant) servant, ev);
//   g_free(servant);
//}
//*/
//
//
//static CORBA_long
//impl_dope_vscr_map(impl_POA_dope_vscr * servant,
//		      CORBA_char ** ident, CORBA_Environment * ev) {
//	CORBA_long retval;
//	retval = 0;
//	return retval;
//}
//
//static void
//impl_dope_vscr_update(impl_POA_dope_vscr * servant,
//			 CORBA_long x,
//			 CORBA_long y,
//			 CORBA_long w, CORBA_long h, CORBA_Environment * ev)
//{
//}
//
//static void
//impl_dope_vscr_waitsync(impl_POA_dope_vscr * servant,
//			   CORBA_Environment * ev)
//{
//}




//
//
//
//
//
//sdword_t dope_vscreen_server_map(sm_request_t *request, vscr_dataspace_t *ds, sm_exc_t *_ev) {
//	VSCREEN *vs = (VSCREEN *) flick_server_get_local(request);
//////	ds->id = vs->ds.id;
//////	ds->manager = vs->ds.manager;
//////	*ds = (vscr_dataspace_t)vs->ds;
////
////	mem->move(ds,&vs->ds,sizeof(vscr_dataspace_t));
////
////	DOPEDEBUG(printf("VScreen(map): check_rights = %d\n",
////		l4dm_check_rights(&vs->ds,L4DM_RW)
////	);)
////
////	l4dm_share(&vs->ds,request->client_tid,L4DM_RW);
////
//////	printf("VScreen(map): map function called!\n");
//////	printf("VScreen(map): ds.id = %lu\n",(long)(vs->ds.id));
//	return 42;
//}
//
//void dope_vscreen_server_update(sm_request_t *request, sdword_t x, sdword_t y, 
//								sdword_t w, sdword_t h, sm_exc_t *_ev) {
//
//}
//
//void dope_vscreen_server_waitsync(sm_request_t *request, sm_exc_t *_ev) {
//	VSCREEN *vs = (VSCREEN *) flick_server_get_local(request);
//	thread->mutex_down(vs->sync_mutex);
//}



static void vscreen_server_thread(void *arg) {
//	dope_vscr					vscreen;
//	PortableServer_POA			root_poa;
//	PortableServer_POAManager	pm;
	VSCREEN	   *vs = (VSCREEN *)arg;
	
//
//	ev = g_new0(CORBA_Environment, 1);
//
//	CORBA_exception_init(ev);
//
//	orb = CORBA_ORB_init(&dummy_argc, dummy_args, "orbit-local-orb", ev);
//	root_poa = (PortableServer_POA)
//    CORBA_ORB_resolve_initial_references(orb, "RootPOA", ev);
//	vscreen = impl_dope_vscr__create(root_poa, ev);
////	objref = CORBA_ORB_object_to_string(orb, vscreen, ev);
//	
//	pm = PortableServer_POA__get_the_POAManager(root_poa, ev);
//	PortableServer_POAManager_activate(pm, ev);
//
//	DOPEDEBUG(printf("Server(server_thead): entering corba mainloop\n");)
//	CORBA_ORB_run(orb, ev);

	vs->env.srv_port = 0;
	vs->env.user_data = vs;
	dope_vscr_server_loop(&vs->env);
}


///// OOOOLD /////
//static void vscreen_server_thread(void *arg) {
//	int i,ret;
//	l4_msgdope_t result;
//	sm_request_t request;
//  	l4_ipc_buffer_t ipc_buf;
//	char ident_buf[10];
//	VSCREEN *vs = (VSCREEN *)arg;
//
//	printf("VScreen(server_thread): entered server thread\n");
//
//	vs->server_tid = l4thread_l4_id(l4thread_myself());
//	l4thread_set_prio(l4thread_myself(),l4thread_get_prio(l4thread_myself())-5);
//
//	printf("VScreen(server_thread): tid = %lu.%lu\n",
//		(long)(vs->server_tid.id.task),
//		(long)(vs->server_tid.id.lthread));
//
//	flick_init_request(&request, &ipc_buf);
//	flick_server_set_timeout(&request, WAIT_TIMEOUT);
////	flick_server_set_rcvstring(&request, 0, STRBUF_SIZE, strbuf);
//	flick_set_number_rcvstring(&request, 1);
//
//	printf("VScreen(server_thread): set_local\n");
//
//	/* pass this_vc reference as implicit arg */
//	flick_server_set_local(&request, vs);
//
//	printf("VScreen(server_thread): find identifier\n");
//
//	/* find free identifier for this vscreen server */
//	for (i=0;(i<MAX_IDENTS) && (ident_tab[i]);i++);
//	if (i<MAX_IDENTS-1) {
//		sprintf(ident_buf,"Dvs%d",i);
//		ident_tab[i]=1;
//	} else {
//		/* if there are not enough identifiers, exit the server thread */
//		thread_started=1;
//		printf("VScreen(server_thread): no free identifiers\n");
//		return;
//	}
//
//	printf("VScreen(server_thread): ident_buf=%s\n",ident_buf);
//	if (!names_register(ident_buf)) return;
//	
//	vs->server_ident = ident_buf;
//	thread_started=1;
//	
//	/* IDL server loop */
//	while (1) {
//		result = flick_server_wait(&request);
//
//		while (!L4_IPC_IS_ERROR(result)) {
//			/* dispatch request */
//			ret = dope_vscreen_server(&request);
//
//			switch(ret) {
//			case DISPATCH_ACK_SEND:
//				/* reply and wait for next request */
//				result = flick_server_reply_and_wait(&request);
//				break;
//				
//			default:
//				/* wait for next request */
//				result = flick_server_wait(&request);
//				break;
//			}
//		} /* !L4_IPC_IS_ERROR(result) */
//	}
//}


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

/*** RETURN VSCREEN WIDGET SERVER ***/
static char *vscreen_get_server(VSCREEN *vs) {
	static char ident_buf[20];
	if (!vs) return "<noserver>";
	sprintf(ident_buf,"%d",(int)vs->env.srv_port);
	return ident_buf;
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
	static u8 ident_buf[32];

	if (!vs) return 0;
	if (!vscreen_probe_mode(vs,width,height,depth)) return 0;

	/* destroy old image buffer and reset values */
	switch (vs->bpp) {
	case 16: if (vs->image) img16->destroy(vs->image);
	}

	vs->bpp 		= 0;
	vs->xres 		= 0;
	vs->yres 		= 0;
	vs->pixels 		= NULL;
	vs->image  		= NULL;

	/* create new frame buffer image */
	switch (depth) {
	case 16:
		if ((vs->image	= img16->create(11,11))) {
			vs->orig_pixels = ((struct image16 *)vs->image)->pixels;
			vs->bpp		= depth;
			vs->xres	= width;
			vs->yres 	= height;
		
			sprintf(ident_buf,"/tmp/dopevscr%lu.map",vs->server_id);
//			printf("VScreen(set_mode): identifier=%s\n",&vs->server_ident[0]);

       		vs->fh = open(ident_buf,O_CREAT|O_RDWR);
        	ftruncate(vs->fh,vs->xres * vs->yres * 2);
        	vs->pixels = mmap(NULL,vs->xres * vs->yres * 2,PROT_READ|PROT_WRITE,MAP_SHARED,vs->fh,0);
									
			printf("pixels = %lu\n",(adr)vs->pixels);
			
			((struct image16 *)vs->image)->pixels = vs->pixels;
			((struct image16 *)vs->image)->w = width;
			((struct image16 *)vs->image)->h = height;
		} else {
			DOPEDEBUG(printf("VScreen(set_mode): out of memory!\n");)
			return 0;
		}
	}
	
	vs->gen->set_w(vs,width);
	vs->gen->set_h(vs,height);
	vs->gen->update(vs,1); 
	return 1;
}


static struct widget_methods gen_methods;
static struct vscreen_methods  vscr_methods={
	vscreen_get_server,
	vscreen_probe_mode,
	vscreen_set_mode,
};



/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

static VSCREEN *create(void) {
		
	/* allocate memory for new widget */
	VSCREEN *new = (VSCREEN *)mem->alloc(sizeof(VSCREEN)+
										 sizeof(struct widget_data));
	if (!new) {
		ERROR(printf("VSCREEN(create): out of memory\n"));
		return NULL;
	}
	new->gen 	= &gen_methods;		/* pointer to general widget methods */
	new->vscr	= &vscr_methods;	/* pointer to vscreen specific methods */

	/* set general widget attributes */
	new->wd = (struct widget_data *)((long)new + sizeof(VSCREEN));
	widman->default_widget_data(new->wd);
		
	new->update_flags = 0;
	new->sync_mutex = thread->create_mutex(1);	/* locked */ 
	new->bpp 		= 0;
	new->xres 		= 0;
	new->yres 		= 0;
	new->pixels 	= NULL;
	new->image  	= NULL;
	new->server_id	= max_server_id;

	max_server_id++;

	/* start widget server */
	thread_started=0;
	DOPEDEBUG(printf("VScreen(create): creating server thread\n");)
//	thread->create_thread(&vscreen_server_thread,new);
//	while (!thread_started) l4_sleep(1);
	DOPEDEBUG(printf("VScreen(create): server thread started\n");)
	
	DOPEDEBUG(printf("VScreen(create): register new widget at rtman\n");)
	rtman->add(new,0.0);
	rtman->set_sync_mutex(new,new->sync_mutex);
	
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

	widman->build_script_lang(widtype,&gen_methods);
}


int init_vscreen(struct dope_services *d) {

	mem			= d->get_module("Memory 1.0");
	clip		= d->get_module("Clipping 1.0");
	img16		= d->get_module("Image16Data 1.0");
	script		= d->get_module("Script 1.0");
	widman		= d->get_module("WidgetManager 1.0");
	redraw		= d->get_module("RedrawManager 1.0");
	thread		= d->get_module("Thread 1.0");
	rtman		= d->get_module("RTManager 1.0");

	/* define general widget functions */
	widman->default_widget_methods(&gen_methods);

	orig_update=gen_methods.update;
	gen_methods.draw   = vscreen_draw;
	gen_methods.update = vscreen_update;

	build_script_lang();

	d->register_module("VScreen 1.0",&services);
	return 1;
}
