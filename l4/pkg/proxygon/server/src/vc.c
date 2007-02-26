/*
 * \brief   Proxygon virtual console server
 * \date    2004-09-30
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*** GENERAL INCLUDES ***/
#include <stdio.h>
#include <stdlib.h>

/*** L4 INCLUDES ***/
#include <l4/thread/thread.h>
#include <l4/l4con/l4con-server.h>
#include <l4/l4con/stream-client.h>
#include <l4/l4con/l4con.h>
#include <l4/dope/dopelib.h>
#include <l4/dope/vscreen.h>
#include <l4/input/libinput.h>
#include <l4/dm_mem/dm_mem.h>

/*** LOCAL INCLUDES ***/
#include "vc.h"
#include "pslim.h"

#define VC_WIDTH  640  /* width of console in pixels  */
#define VC_HEIGHT 480  /* height of console in pixels */
#define NUM_SBUFS 3    /* number of receive buffers   */

#define u16 unsigned short

struct vc;
struct vc {
	int           id;                    /* id of virtual console        */
	l4_threadid_t tid;                   /* vc server thread id          */
	l4_threadid_t listener;              /* event listener thread        */
	l4_uint32_t  *sbuf[NUM_SBUFS];       /* receive buffers              */
	int           sbuf_size[NUM_SBUFS];  /* sizes of receive buffers     */
	struct pslim *canvas;                /* pSLIM canvas                 */
	void         *pixels;                /* VScreen pixel buffer         */
	void         *ds_adr;                /* imported dataspace address   */
	struct vc    *next;                  /* next virtual console in list */
};

static struct vc *first_vc;   /* head of vc list                    */
static int id_cnt = 1;        /* counter for creating unique vc ids */
extern int app_id;            /* DOpE application id                */
static int tls_key;           /* key for thread local storage       */


/*********************************
 *** INTERNALLY USED FUNCTIONS ***
 *********************************/

/*** UTILITY: DETERMINE VIRTUAL CONSOLE THAT BELONGS TO THE CURRENT VC THREAD ***/
static inline struct vc *get_vc(void) {
	struct vc *vc = l4thread_data_get_current(tls_key);
	if (!vc)
		printf("Error: get_vc: thread local storage contains no valid vc struct.\n");
	return vc;
}


/*** UTILITY: ALLOCATE RECEIVE BUFFER ***/
static int alloc_sbuf(l4_uint32_t **addr, int size, int vc_id) {
	char buf[16];

	/* sanity check */
	if (!size) return 0;
	if (!addr) return -1;

	/* create name for dataspace */
	snprintf(buf, sizeof(buf), "vc%d sbuf", vc_id);

	/* allocate dataspace */
	printf("calling dm_mem_allocate_named(%d, %s)\n", size, buf);
	*addr = l4dm_mem_allocate_named(size, L4RM_MAP, buf);

	/* return 0 on success */
	return (*addr) ? 0 : -1;
}


/*** UTILITY: FREE RECEIVE BUFFER ***/
static void free_sbuf(l4_uint32_t **addr) {
	if (*addr) l4dm_mem_release(*addr);
	*addr = NULL;
}


/*** UTILITY: FREE VIRTUAL CONSOLE ***/
static int destroy_vc(struct vc *vc) {
	struct vc *curr;
	int i;

	if (!vc) return 0;

	/* kill reference to thread local data */
	l4thread_data_set(l4thread_id(vc->tid), tls_key, NULL);

	/* unchain vc struct from connected list */
	if (vc == first_vc) first_vc = first_vc->next;

	/* search in list for vc struct */
	for (curr = first_vc; curr->next && (curr->next != vc); curr = curr->next);

	/* curr is predecessor of vc struct or NULL (not in list), skip vc in list */
	if (curr && (curr->next == vc)) curr->next = curr->next->next;

	/* deallocate receive buffers */
	for (i = 0; i < NUM_SBUFS; i++) free_sbuf(&vc->sbuf[i]);

	/* close DOpE window */
	dope_cmdf(app_id, "win%d.close()", vc->id);

	/* release shared VScreen buffer */
	vscr_free_fb(vc->pixels);

	/* release imported dataspace */
	if (vc->ds_adr) l4rm_detach(vc->ds_adr);

	/* free pSLIM canvas */
	vc->canvas->destroy(vc->canvas);

	/* stop vc thread */
	l4thread_shutdown(l4thread_id(vc->tid));

	return 0;
}


/**************************
 *** CALLBACK FUNCTIONS ***
 **************************/

/*** CALLBACK: CALLED FOR EACH INCOMING KEYBOARD EVENT ***
 *
 * We just forward all keyboard events to the stream server thread
 * of the corresponding virtual console application.
 */
static void keyevent_callback(dope_event *e, void *arg) {
	CORBA_Environment env = dice_default_environment;
	struct stream_io_input_event ev;
	struct vc *vc = arg;

	ev.type = EV_KEY;
	ev.code = e->press.code;
	ev.value = (e->type == EVENT_TYPE_PRESS);

	/* send event to listener thread */
	stream_io_push_call(&vc->listener, &ev, &env);
}


/*** CALLBACK: CALLED FOR EACH MOUSE MOTION EVENT ***/
static void absmotion_callback(dope_event *e, void *arg) {
	CORBA_Environment env = dice_default_environment;
	struct stream_io_input_event ev;
	struct vc *vc = arg;

	/* send horizontal motion event to listener thread */
	ev.type  = EV_ABS;
	ev.code  = 0;
	ev.value = e->motion.abs_x;
	stream_io_push_call(&vc->listener, &ev, &env);

	/* send vertical motion event to listener thread */
	ev.type  = EV_ABS;
	ev.code  = 1;
	ev.value = e->motion.abs_y;
	stream_io_push_call(&vc->listener, &ev, &env);
}


/*** CALLBACK: UPDATE REGION OF VSCREEN WIDGET ***
 *
 * This function is called from the pSLIM canvas for each
 * area that must be redrawn on screen.
 */
static void canvas_update(int x, int y, int w, int h, void *arg) {
	struct vc *vc = arg;
	dope_cmdf(app_id, "vscr%d.refresh(-x %d -y %d -w %d -h %d)",
	                   vc->id, x, y, w, h);
//	printf("canvas_update called id=%d, x=%d, y=%d, w=%d, h=%d, arg=%p\n", vc->id, x, y, w, h, arg);
}


/*************************************
 *** VC SERVER COMPONENT FUNCTIONS ***
 *************************************/

/*** INIT RECEIVE BUFFERS ***
 *
 * Before the virtual console serverloop is entered, this function
 * is called for each receive buffer that is needed by the serverloop.
 * The virtual console uses three receive buffers that were allocated
 * when the vc server thread was started. We need to return the
 * information about these buffers here.
 */
void vc_init_rcvstring(int nb, l4_umword_t *addr, l4_umword_t *size,
                           CORBA_Server_Environment *env) {
	struct vc *vc = get_vc();
	if (!vc || !addr || !size) return;

	if (nb >= NUM_SBUFS) {
		printf("Error: init_rcvstring: number of receive buffer is out of range.\n");
		return;
	}

	/* return information about receive buffers */
	*addr = (l4_umword_t)vc->sbuf[nb];
	*size = (l4_umword_t)vc->sbuf_size[nb];
}


/*** CLOSE VIRTUAL CONSOLE ***/
long
con_vc_close_component(CORBA_Object _dice_corba_obj,
                       short *_dice_reply,
                       CORBA_Server_Environment *_dice_corba_env) {
	struct vc *vc = l4thread_data_get_current(tls_key);
	return destroy_vc(vc);
}


/*** DRAW FILLED RECTANGLE ***/
long
con_vc_pslim_fill_component(CORBA_Object _dice_corba_obj,
                            const l4con_pslim_rect_t *rect,
                            l4con_pslim_color_t color,
                            CORBA_Server_Environment *_dice_corba_env) {
	struct vc *vc = l4thread_data_get_current(tls_key);
	return vc ? vc->canvas->fill(vc->canvas, rect, color) : -1;
}


/*** DRAW BITMAP ***/
long
con_vc_pslim_bmap_component(CORBA_Object _dice_corba_obj,
                            const l4con_pslim_rect_t *rect,
                            l4con_pslim_color_t fg,
                            l4con_pslim_color_t bg,
                            const l4_uint8_t *bmap,
                            long bmap_size,
                            unsigned char bmap_type,
                            CORBA_Server_Environment *_dice_corba_env) {
	struct vc *vc = l4thread_data_get_current(tls_key);
	return vc ? vc->canvas->bmap(vc->canvas, rect, fg, bg, bmap, bmap_type) : 0;
}


/*** DRAW COLOR IMAGE ***/
long
con_vc_pslim_set_component(CORBA_Object _dice_corba_obj,
                           const l4con_pslim_rect_t *rect,
                           const unsigned char *pmap,
                           long pmap_size,
                           CORBA_Server_Environment *_dice_corba_env) {
	struct vc *vc = l4thread_data_get_current(tls_key);
	return vc ? vc->canvas->set(vc->canvas, rect, pmap) : 0;
}


/*** DRAW STRING ***/
long
con_vc_puts_component(CORBA_Object _dice_corba_obj,
                      const char *s,
                      int len,
                      short x,
                      short y,
                      l4con_pslim_color_t fg,
                      l4con_pslim_color_t bg,
                      CORBA_Server_Environment *_dice_corba_env) {
	struct vc *vc = l4thread_data_get_current(tls_key);
	return vc ? vc->canvas->puts(vc->canvas, s, len, x, y, fg, bg) : 0;
}


/*** DRAW ANSI STRING ***/
long
con_vc_puts_attr_component(CORBA_Object _dice_corba_obj,
                           const short *s,
                           int len,
                           short x,
                           short y,
                           CORBA_Server_Environment *_dice_corba_env) {
	struct vc *vc = l4thread_data_get_current(tls_key);
	return vc ? vc->canvas->puts_attr(vc->canvas, (char *)s, len, x, y) : 0;

}


/*** SET EVENT LISTENER THREAD ***/
long
con_vc_smode_component(CORBA_Object _dice_corba_obj,
                       unsigned char mode,
                       const l4_threadid_t *ev_handler,
                       CORBA_Server_Environment *_dice_corba_env) {
	struct vc *vc = l4thread_data_get_current(tls_key);
	if (!vc) return -1;
	vc->listener = *ev_handler;
	return 0;
}


/*** REQUEST INFORMATION ABOUT CURRENT VIDEO MODE ***/
long
con_vc_graph_gmode_component(CORBA_Object _dice_corba_obj,
                             unsigned char *g_mode,
                             l4_uint32_t *xres,
                             l4_uint32_t *yres,
                             l4_uint32_t *bits_per_pixel,
                             l4_uint32_t *bytes_per_pixel,
                             l4_uint32_t *bytes_per_line,
                             l4_uint32_t *flags,
                             l4_uint32_t *xtxt,
                             l4_uint32_t *ytxt,
                             CORBA_Server_Environment *_dice_corba_env) {

	*g_mode          = GRAPH_BPP_16;
	*xres            = VC_WIDTH;
	*yres            = VC_HEIGHT;
	*bits_per_pixel  = 16;
	*bytes_per_pixel = 2;
	*bytes_per_line  = VC_WIDTH * *bytes_per_pixel;
	*xtxt            = pSLIM_FONT_CHAR_W;
	*ytxt            = pSLIM_FONT_CHAR_H;
	*flags           = 0;

	return 0;
}


/*** IMPORT DATASPACE TO BE USED AS VIRTUAL FRAMEBUFFER ***/
long
con_vc_direct_setfb_component(CORBA_Object _dice_corba_obj,
                              const l4dm_dataspace_t *data_ds,
                              CORBA_Server_Environment *_dice_corba_env) {
	int error;
	l4_size_t size;
	struct vc *vc = l4thread_data_get_current(tls_key);
	if (!vc) return -1;

	/* if there already exists an imported dataspace, free it */
	if (vc->ds_adr) {
		l4rm_detach(vc->ds_adr);
		vc->ds_adr = NULL;
	}

	/* request size of dataspace */
	if ((error = l4dm_mem_size((l4dm_dataspace_t*)data_ds, &size))) {
		printf("Error: direct_setfb: l4dm_mem_size returned %d", error);
		return -1;
	}

	/* check if size of imported dataspace is sufficient */
	if (size < VC_WIDTH*VC_HEIGHT*2) {
		printf("Error: direct_setfb: imported dataspace is too small\n");
		return -1;
	}

	/* map dataspace into local address space */
	error = l4rm_attach((l4dm_dataspace_t*)data_ds, size, 0, L4DM_RO, &vc->ds_adr);
	if (error) {
		printf("Error: direct_setfb: l4rm_attach returned %d", error);
		return -1;
	}
	return 0;
}


/*** UPDATE SCREEN AREA OF AN IMPORTED DATASPACE ***/
long
con_vc_direct_update_component(CORBA_Object _dice_corba_obj,
                               const l4con_pslim_rect_t *rect,
                               CORBA_Server_Environment *_dice_corba_env) {

	u16 *src, *dst;
	struct vc *vc = l4thread_data_get_current(tls_key);
	int x, y, w, h, i;

	if (!vc) return -1;
	if (!vc->ds_adr || !vc->pixels) return -2;

	/* clip rectangle to buffer dimensions */
	x = rect->x;
	y = rect->y;
	w = rect->w;
	h = rect->h;

	if (x < 0) { w += x; x  = 0; }
	if (y < 0) { h += y; y  = 0; }
	if (x + w > VC_WIDTH)  w = VC_WIDTH  - x;
	if (y + h > VC_HEIGHT) h = VC_HEIGHT - y;

	/* check if an area is left */
	if (w <= 0 || h <= 0) return 0;

	/* copy area from dataspace to vscreen buffer */
	src = (u16 *)vc->ds_adr + y*VC_WIDTH + x;
	dst = (u16 *)vc->pixels + y*VC_WIDTH + x;

	for (i = 0; i < h; i++) {
		memcpy(dst, src, w*2);
		src += VC_WIDTH;
		dst += VC_WIDTH;
	}

	/* update corresponding vscreen area */
	canvas_update(x, y, w, h, vc);
	return 0;
}


/*** REQUEST PIXEL LAYOUT ***/
long
con_vc_graph_get_rgb_component(CORBA_Object _dice_corba_obj,
                               l4_uint32_t *red_offs,
                               l4_uint32_t *red_len,
                               l4_uint32_t *green_offs,
                               l4_uint32_t *green_len,
                               l4_uint32_t *blue_offs,
                               l4_uint32_t *blue_len,
                               CORBA_Server_Environment *_dice_corba_env) {

	/* return values for RGB565 */
	*red_len    = 5;
	*red_offs   = 11;
	*green_len  = 6;
	*green_offs = 5;
	*blue_len   = 5;
	*blue_offs  = 0;
	return 0;
}


/*********************************************
 *** PROXYGON INTERNAL INTERFACE FUNCTIONS ***
 *********************************************/

/*** START VC SERVER THREAD AND CREATE A NEW VIRTUAL CONSOLE WINDOW ***/
int start_vc_server(l4_uint32_t sbuf1_size,
                    l4_uint32_t sbuf2_size,
                    l4_uint32_t sbuf3_size,
                    l4_uint8_t priority,
                    l4_threadid_t *vc_tid,
                    l4_int16_t vfbmode) {
	int i, ret = 0;
	l4thread_t id;
	char buf[16];
	struct vc *new = malloc(sizeof(struct vc));

	if (!new) {
		printf("Error: start_vc_server: unable to allocate vc struct\n");
		return -1;
	}
	memset(new, 0, sizeof(struct vc));

	/* remember unique virtual console id and dimensions of receive buffers */
	new->id = id_cnt++;
	new->sbuf_size[0] = sbuf1_size;
	new->sbuf_size[1] = sbuf2_size;
	new->sbuf_size[2] = sbuf3_size;

	/* allocate receive buffers */
	for (i = 0; i < NUM_SBUFS; i++)
		ret |= alloc_sbuf(&new->sbuf[i], new->sbuf_size[i], new->id);

	/* check if something went wrong with the allocation */
	if (ret) {
		printf("Error: start_vc_server: unable to allocate receive buffers\n");

		/* free already allocated buffers */
		for (i = 0; i < NUM_SBUFS; i++) free_sbuf(&new->sbuf[i]);
		return -1;
	}

	/* allocate key for thread local storage */
	if (!tls_key) tls_key = l4thread_data_allocate_key();

	/* create service thread for the innocent virtual console */
	snprintf(buf, sizeof(buf), "proxygon vc%d", new->id);
	id = l4thread_create_named(con_vc_server_loop, buf, NULL, L4THREAD_CREATE_ASYNC);
	if (id <= 0) {
		printf("Error: start_vc_server: unable to create vc server thread\n");
		return -1;
	}

	/* remember id of newly awakened vc server thread */
	new->tid = l4thread_l4_id(id);
	new->listener = L4_NIL_ID;

	/* set thread local pointer to vc struct */
	l4thread_data_set(id, tls_key, new);

	/* set priority of vc server thread */
	l4thread_set_prio(id, priority);

	/* create and open DOpE window */
	dope_cmdf(app_id, "vscr%d = new VScreen(-grabfocus yes)", new->id);
	dope_cmdf(app_id, "vscr%d.setmode(%d, %d, RGB16)",
	                   new->id, VC_WIDTH, VC_HEIGHT);
	dope_cmdf(app_id, "win%d = new Window(-content vscr%d -workw %d -workh %d)",
	                   new->id, new->id, VC_WIDTH, VC_HEIGHT);
	dope_cmdf(app_id, "win%d.open()", new->id);

	/* install event handler for events referring the VScreen widget */
	dope_bindf(app_id, "vscr%d", "press",   keyevent_callback,  new, new->id);
	dope_bindf(app_id, "vscr%d", "release", keyevent_callback,  new, new->id);
	dope_bindf(app_id, "vscr%d", "motion",  absmotion_callback, new, new->id);

	/* map VScreen buffer */
	snprintf(buf, sizeof(buf), "vscr%d", new->id);
	new->pixels = vscr_get_fb(app_id, buf);
	if (!new->pixels) {
		printf("Error: start_vc_server: could not map VScreen buffer\n");
		return -1;
	}

	/* create pSLIM canvas */
	new->canvas = create_pslim_canvas(canvas_update, new);
	new->canvas->set_mode(new->canvas, VC_WIDTH, VC_HEIGHT, 16, new->pixels);

	/* chain new vc struct into vc list */
	new->next = first_vc;
	first_vc  = new;

	/* everything went fine */
	if (vc_tid) *vc_tid = new->tid;
	return 0;
}


/*** CLOSE ALL VIRTUAL CONSOLES OF THE SPECIFIED CLIENT TASK ***/
void piss_off_client(l4_threadid_t tid) {
	struct vc *vc = first_vc;

	printf("piss_off_client called\n");
	/* close all virtual consoles that belong to the specified task */
	while (vc) {

		if (vc->listener.id.task == tid.id.task) {
			printf("found virtual console to destroy\n");
			destroy_vc(vc);
		}
		vc = vc->next;
	}
	printf("piss_off_client finished\n");
}
