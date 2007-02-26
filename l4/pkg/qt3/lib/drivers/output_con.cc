/*** L4-SPECIFIC INCLUDES ***/
#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/thread/thread.h>
#include <l4/semaphore/semaphore.h>
#include <l4/input/libinput.h>
#include <l4/l4con/l4con.h>
#include <l4/l4con/l4con-client.h>
#include <l4/l4con/stream-server.h>
#include <l4/dm_mem/dm_mem.h>

/*** LOCAL INCLUDES ***/
#include <l4/qt3/l4qws_shm_client.h>
#include "output.h"

extern void drops_qws_notify(void);

/* ************************************************************************* */

/* hope it will be be enough */
#define SBUF_SIZE 4096

/* ************************************************************************* */

typedef struct fb_info {
  int           w, h;
  l4_threadid_t con_vc;
} fb_info_t;

/* ************************************************************************* */

int                     num_con_events = 0;
stream_io_input_event_t con_event[1024];
l4semaphore_t           con_event_lock = L4SEMAPHORE_UNLOCKED_INITIALIZER;

/* ************************************************************************* */

static void update_thread(void *info);
static void stream_io_thread(void *p);

/* ************************************************************************* */

static int    scr_width, scr_height; /* screen dimensions */
static int    scr_depth;             /* color depth */
static int    scr_linelength;        /* bytes per scanline */
static void  *scr_adr;               /* physical screen address */

/* ************************************************************************* */

long drops_qws_set_screen(long width, long height, long depth) {
  DICE_DECLARE_ENV(env);
  l4_threadid_t      con_ev_handler = L4_NIL_ID_INIT; //L4_NIL_ID;
  l4_threadid_t      con_server;
  l4_threadid_t      con_vc;
  l4thread_t         con_ev_th;
  l4dm_dataspace_t   fb_ds;
  int                dummy;
  int                map_size;
  int                is_server;

  con_ev_th      = l4thread_create(stream_io_thread, NULL, L4THREAD_CREATE_SYNC);
  con_ev_handler = l4thread_l4_id(con_ev_th);

  if (names_waitfor_name(CON_NAMES_STR, &con_server, 5000) == 0) {
    LOG("con not registered at names");
    return 0;
  }
    
  if (con_if_openqry_call(&con_server, SBUF_SIZE, SBUF_SIZE, SBUF_SIZE,
                          (l4_uint8_t)L4THREAD_DEFAULT_PRIO, &con_vc, 0, &env) != 0) {
    LOG("could not open con virtual console");
    return 0;
  }

  if (con_vc_smode_call(&con_vc, CON_INOUT, &con_ev_handler, &env) != 0) {
    LOG("dummy setting of graphics mode failed");
    return 0;
  }

  if (con_vc_graph_gmode_call(&con_vc, (l4_uint8_t *)&dummy, (l4_uint32_t*)&scr_width,
                              (l4_uint32_t*)&scr_height, (l4_uint32_t*)&scr_depth,
			      (l4_uint32_t*)&dummy, (l4_uint32_t*)&scr_linelength,
                              (l4_uint32_t*)&dummy, (l4_uint32_t*)&dummy,
                              (l4_uint32_t*)&dummy, &env) != 0) {
    LOG("could not determine properties of virtual console");
    return 0;
  }

  map_size = scr_height * scr_linelength;

  /* Get a dataspace from QSharedMemory server. We are using '0' 
   * as naming key for the framebuffer.
   *
   * l4qws_shm_create_call() below will fail for clients.
   */
  if (l4qws_shm_create_call(&l4qws_shm_server, 0, map_size, &fb_ds, &env) != 0) {

    l4_threadid_t me = l4_myself();
    /* we are not the server; we need to close the vc */
    is_server = 0;

    if (con_if_close_all_call(&con_server, &me, &env) != 0) {
      LOG("could not close virtual console");
      return 0;
    }

    if (l4qws_shm_get_call(&l4qws_shm_server, 0, &fb_ds, &env) != 0) {
      LOG("could not get shared framebuffer via QSharedMemory server");
      return 0;
    }
  } else {
    /* we are the server */
    is_server = 1;
  }

  if (l4rm_attach(&fb_ds, map_size, 0, L4DM_RW, &scr_adr) != 0) {
    LOG("could not map shared framebuffer");
    return 0;
  }

  if (scr_adr == NULL) {
    LOG("failed to allocate framebuffer");
    return 0;
  }

  LOG("w=%d", scr_width);
  LOG("h=%d", scr_height);
  LOG("d=%d", scr_depth);
  LOG("bytes_per_line=%d", scr_linelength);
  LOG("fb=%p", scr_adr);

  if (is_server) {

    /* Setup framebuffer update thread (this is soooo ugly (and even much
     * slower)). */

    fb_info_t i;

    if (l4dm_share(&fb_ds, con_vc, L4DM_RO) != 0) {
      LOG("failed to share framebuffer");
      return 0;
    }

    if (con_vc_direct_setfb_call(&con_vc, &fb_ds, &env) != 0) {
      LOG("failed to set framebuffer");
      return 0;
    }
    
    i.w      = scr_width;
    i.h      = scr_height;
    i.con_vc = con_vc;
    
    l4thread_create(update_thread, &i, L4THREAD_CREATE_SYNC);
  }

  return 1;
}

long  drops_qws_get_scr_width  (void) {return scr_width;}
long  drops_qws_get_scr_height (void) {return scr_height;}
long  drops_qws_get_scr_depth  (void) {return scr_depth;}
void *drops_qws_get_scr_adr    (void) {return scr_adr;}
long  drops_qws_get_scr_line   (void) {return scr_linelength;}

void drops_qws_refresh_screen  (void) {}

/* ************************************************************************* */

void stream_io_push_component(CORBA_Object _dice_corba_obj,
                              const stream_io_input_event_t *event,
                              CORBA_Server_Environment *_dice_corba_env) {

  l4semaphore_down(&con_event_lock);

  if (num_con_events < 16383) {
    
    int merged = 0;
    
    //LOG("(%d,%d,%d) %d", event->type, event->code, event->value, num_con_events);    

    if (event->type == EV_REL && num_con_events > 1) {

      /* Do some magic on mouse events: If there are mouse events at
       * the end of the queue and the new event is also a mouse event,
       * we try to merge the current one with the events that are
       * already queued. */

      int i0, i1;

      i0 = num_con_events - 1;
      i1 = num_con_events - 2;

      if (con_event[i0].type == EV_REL &&
          con_event[i0].code == event->code) {
        con_event[i0].value += event->value;
        merged = 1;

      } else if (con_event[i1].type == EV_REL &&
                 con_event[i1].code == event->code) {
        con_event[i1].value += event->value;
        merged = 1;        
      }
    }
  
    if (merged == 0) {
      /* no magic, just append */
      con_event[num_con_events].type = event->type;
      con_event[num_con_events].code = event->code;
      con_event[num_con_events].value = event->value;
      num_con_events++;
    }
  }

  l4semaphore_up(&con_event_lock);

  drops_qws_notify();
}

/* ************************************************************************* */

static void update_thread(void *i) {

  DICE_DECLARE_ENV(env);
  fb_info_t          *info = (fb_info_t *) i;
  l4_threadid_t      con_vc;
  l4con_pslim_rect_t update_region;

  update_region.x = 0;
  update_region.y = 0;
  update_region.w = info->w;
  update_region.h = info->h;
  con_vc          = info->con_vc;

  l4thread_started(NULL);

  while (1) {
    if (con_vc_direct_update_call(&con_vc, &update_region, &env) != 0) {
      LOG("failed to update framebuffer");
      return;
    }
    //LOG("framebuffer updated");

    l4_sleep(40);
  }  
}


static void stream_io_thread(void *info) {

  l4thread_started(NULL);

  stream_io_server_loop(NULL);
}

