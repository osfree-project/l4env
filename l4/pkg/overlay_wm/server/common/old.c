#include <stdlib.h>
#include <stdio.h>
#include <dopelib.h>
#include <vscreen.h>
#include "overlay-server.h"
#include "startup.h"
#include "thread.h"
#include "serverloop.h"

static int scr_w, scr_h, scr_depth;
static CORBA_Object input_event_srv;
static CORBA_Object window_event_srv;

struct ovl_win_struct {
	int id;
	int x,y,w,h;
};

static int app_id;
static int num_windows = 0;

/*** EVENT CALLBACK FUNCTIONS ***/

static void vscr_press_callback(dope_event *e, void *arg) {
	printf("vscr press\n");
}

static void winmove_callback(dope_event *e, void *arg) {
	struct ovl_win_struct *window = (struct ovl_win_struct *)arg;
	window->x = atoi(dope_cmdf(app_id,"win%d.x",window->id));
	window->y = atoi(dope_cmdf(app_id,"win%d.y",window->id));
	dope_cmdf(app_id,"frm%d.set(-xview %d -yview %d)",window->id,window->x,window->y);
}

static void winsize_callback(dope_event *e, void *arg) {
	struct ovl_win_struct *window = (struct ovl_win_struct *)arg;
	window->x = atoi(dope_cmdf(app_id,"win%d.x",window->id));
	window->y = atoi(dope_cmdf(app_id,"win%d.y",window->id));
	dope_cmdf(app_id,"frm%d.set(-xview %d -yview %d)",window->id,window->x,window->y);
}

int overlay_init_screen_component(CORBA_Object _dice_corba_obj,
                                 int width,
                                 int height,
                                 int depth,
                                 CORBA_Environment *_dice_corba_env) {
	int i;
	char *pixelformat = NULL;
	
	switch (depth) {
	case 16: 
		pixelformat = "RGB16";
		break;
	default:
		printf("OvlWM(init_screen): color depth %d is not supported.\n", depth);
		return -1;
	}
	
	dope_cmd(app_id, "a = new Window()");
	dope_cmd(app_id, "vscr = new VScreen()");
	dope_cmdf(app_id, "vscr.setmode(%d,%d,\"%s\")", width, height, pixelformat);
	dope_cmd(app_id, "a.set(-x 100 -y 150 -w 320 -h 240 -scrollx yes -scrolly yes -background off -content vscr)" );
	dope_cmd(app_id, "a.open()");
	dope_bind(app_id,"vscr","press",vscr_press_callback,(void *)0);

	scr_w = width;
	scr_h = height;
	scr_depth = depth;
	input_event_srv = NULL;
	return 0;
}


void overlay_deinit_screen_component(CORBA_Object _dice_corba_obj,
                                   int scr_id,
                                   CORBA_Environment *_dice_corba_env) {
	dope_cmd(app_id, "a.close()");
	dope_deinit_app(app_id);
}


int overlay_map_screen_component(CORBA_Object _dice_corba_obj,
                                int scr_id,
                                char* *ds_ident,
                                CORBA_Environment *_dice_corba_env) {
	static char strbuf[128];
	thread2ident(_dice_corba_obj,&strbuf[128]);
	*ds_ident = dope_cmdf(app_id,"vscr.map(-thread \"%s\")",&strbuf[0]);
	return 0;
}


void overlay_refresh_screen_component(CORBA_Object _dice_corba_obj,
                                    int scr_id,
                                    int x,
                                    int y,
                                    int w,
                                    int h,
                                    CORBA_Environment *_dice_corba_env) {
	dope_cmd(app_id,"vscr.refresh()");
}


//void overlay_set_listener_component(CORBA_Object _dice_corba_obj,
//                                  int scr_id,
//                                  const char* listener_ident,
//                                  CORBA_Environment *_dice_corba_env) {
//}


int overlay_create_window_component(CORBA_Object _dice_corba_obj,
                                   int scr_id,
                                   CORBA_Environment *_dice_corba_env) {
	static char strbuf[128];
	int win_id;
	struct ovl_win_struct *window = malloc(sizeof(struct ovl_win_struct));
	
	win_id = num_windows++;
	window->id = win_id;
	
	dope_cmdf(app_id,"win%d = new Window()", win_id);
	dope_cmdf(app_id,"vscr%d = new VScreen()",win_id);
	dope_cmdf(app_id,"vscr%d.share(vscr)",win_id);
	dope_cmdf(app_id,"frm%d = new Frame()", win_id);
	dope_cmdf(app_id,"frm%d.set(-content vscr%d)", win_id, win_id);
	dope_cmdf(app_id,"win%d.set(-content frm%d)",win_id, win_id);
	sprintf(&strbuf[0],"win%d",win_id);
	dope_bind(app_id,&strbuf[0],"moved",  winmove_callback,(void *)window);
	dope_bind(app_id,&strbuf[0],"resized",winsize_callback,(void *)window);
	return win_id;
}


void overlay_open_window_component(CORBA_Object _dice_corba_obj,
                                 int scr_id,
                                 int win_id,
                                 CORBA_Environment *_dice_corba_env) {
	dope_cmdf(app_id,"win%d.open()",win_id);
}


void overlay_close_window_component(CORBA_Object _dice_corba_obj,
                                  int scr_id,
                                  int win_id,
                                  CORBA_Environment *_dice_corba_env) {
	dope_cmdf(app_id,"win%d.close()",win_id);
}


void overlay_place_window_component(CORBA_Object _dice_corba_obj,
                                  int scr_id,
                                  int win_id,
                                  int x,
                                  int y,
                                  int w,
                                  int h,
                                  CORBA_Environment *_dice_corba_env) {
	dope_cmdf(app_id,"win%d.set(-x %d -y %d -w %d -h %d)",(int)x,(int)y,(int)w,(int)h);
}


void overlay_top_window_component(CORBA_Object _dice_corba_obj,
                                int scr_id,
                                int win_id,
                                CORBA_Environment *_dice_corba_env) {
	dope_cmdf(app_id,"win%d.top()",win_id);
}


int main(int argc, char **argv) {
	native_startup(argc,argv);
	dope_init();
	app_id = dope_init_app("Overlay Screen");
	thread_create(enter_overlay_server_loop,0);
	dope_eventloop(app_id);
	return 0;
}
