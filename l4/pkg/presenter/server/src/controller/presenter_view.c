/**
 * \brief   presenter view controller module
 * \date    2003-04-11
 * \author  Jens Syckor <js712688@inf.tu-dresden.de>
 *
 * This file implements the controller which interacts with DOpE.
 */

struct private_view;
#define PRESENTER_VIEW struct private_view

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <l4/dm_mem/dm_mem.h>
#include <l4/env/errno.h>

#include <l4/dope/vscreen.h>        /* DOpE VScreen lib */
#include <l4/dope/keycodes.h>

#include "properties/view.properties"
#include "util/arraylist.h"
#include "util/presenter_conf.h"
#include "model/presenter.h"
#include "util/module_names.h"
#include "util/dataprovider.h"
#include "model/slide.h"
#include "model/presentation.h"
#include "model/presmanager.h"
#include "controller/presenter_view.h"
#include "view/display.h"

#define WIND_H 400
#define WIND_W 540

static int fullscr_h;
static int fullscr_w;

#define PREV_W 128
#define PREV_H 128

#define PRES_HELP_W 40
#define PRES_HELP_H 11

#define PRES_SLIDE_PREFIX "sld"
#define SLIDE_NAME_SIZE 16

#ifdef DEBUG
static int _DEBUG = 1;
#else
static int _DEBUG = 0;
#endif

PRESENTER_VIEW {

    struct presenter_view_methods *pvm;

    /* all general methods */
    struct presenter_methods *gen;

    /* all general data */
    struct presenter_data *pdat;

    int curr_pres;
    int max_size_load;
    int error_index;
    u8 help_is_open;
    u16 *scradr;
    long app_id;                 /* DOpE application id */
    ARRAYLIST *slide_list;

};

struct prev_slide_info {
    PRESENTER_VIEW *capsuled_pv;
    int current_index;
};

extern PRESMANAGER *pm;

static struct arraylist_services *arraylist;
static struct pres_display_services *display;
static struct dataprovider_services *dataprovider;
static struct presenter_general_services *presenter;

static struct presenter_methods gen_methods;

int init_presenter_view(struct presenter_services *);
void create_help_window(PRESENTER_VIEW *pv);
char * convert2DOpEString(char *text);
void showSlide(PRESENTER_VIEW *pv, int slide_nr);

static void button_fullscreen_callback(dope_event *e,void *arg) {
    PRESENTER_VIEW *pv = (PRESENTER_VIEW *) arg;
    if (pv->curr_pres < 1) return;

    dope_cmdf(pv->app_id, "fullw.set(-workx 0 -worky 0 -workw %d -workh %d)",
              fullscr_w, fullscr_h);
    dope_cmd(pv->app_id, "fullw.open()");
    dope_cmd(pv->app_id, "fullw.top()");
}

static void full_vscr_press_callback(dope_event *e,void *arg)
{
  PRESENTER_VIEW *pv = (PRESENTER_VIEW *) arg;

  if (pv->curr_pres < 1)
    return;

  if (e->type == EVENT_TYPE_PRESS) {

        switch (e->press.code) {

        /* escape key */
        case DOPE_KEY_ESC:
        case DOPE_KEY_Q:

        dope_cmd(pv->app_id,"fullw.close()");
        dope_cmd(pv->app_id,"appw.top()");
        break;

        /* left arrow key or right mouse button */
        case DOPE_KEY_LEFT:
        case DOPE_KEY_PAGEUP:
        case DOPE_BTN_RIGHT:

        showSlide(pv, arraylist->get_current_index(pv->slide_list)-1);
        break;

        /* right arrow or space or left mouse button */
        case DOPE_KEY_RIGHT:
        case DOPE_KEY_SPACE:
        case DOPE_KEY_PAGEDOWN:
        case DOPE_BTN_LEFT:

        showSlide(pv, arraylist->get_current_index(pv->slide_list)+1);
        break;

        default:
            break;
        }

    }
}

static void go_entry_commit_callback(dope_event *e,void *arg) {
    char req_buf[16];
    int slide_nr;
    PRESENTER_VIEW *pv = (PRESENTER_VIEW *) arg;
    if (pv->curr_pres < 1) return;

    dope_req(pv->app_id,req_buf,sizeof(req_buf),"go_entry.text");

    slide_nr = atoi(req_buf) - 1;

    /* reset input field and close go window */
    dope_cmd(pv->app_id,"go_entry.set(-text \"\")");
    dope_cmd(pv->app_id,"go_window.close()");

    showSlide(pv, slide_nr);
}

static void appw_press_callback(dope_event *e,void *arg) {
    PRESENTER_VIEW *pv = (PRESENTER_VIEW *) arg;
    if (pv->curr_pres < 1) return;

        if (e->type == EVENT_TYPE_PRESS) {

            switch (e->press.code) {

            /* left arrow */
            case DOPE_KEY_LEFT:

            showSlide(pv, arraylist->get_current_index(pv->slide_list)-1);
            break;

            /* right arrow or space key */
            case DOPE_KEY_RIGHT:
            case DOPE_KEY_SPACE:

            showSlide(pv, arraylist->get_current_index(pv->slide_list)+1);
            break;

            /* pressed f key */
            case DOPE_KEY_F:

	    button_fullscreen_callback(NULL, pv);
            break;

            default:
            break;
            }
    }

}

static void prev_vscreen_callback(dope_event *e,void *arg) {
    struct prev_slide_info *caps = (struct prev_slide_info *) arg;
    PRESENTER_VIEW *pv = (PRESENTER_VIEW *) caps->capsuled_pv;

    if (e->type == EVENT_TYPE_PRESS) {

        switch (e->press.code) {

            case DOPE_KEY_F:
            button_fullscreen_callback(NULL, pv);
            return;

            case DOPE_BTN_LEFT:
            showSlide(pv, caps->current_index);
            return;
        }
    }

    /*
     * If the event was not specific for the preview,
     * let us handle it as window shortcut.
     */
    appw_press_callback(e, pv);
}

static void button_reload_callback(dope_event *e,void *arg) {
    int pres_key, key, res;
    char *path;
    PRESENTATION *p;
    struct prev_slide_info *caps = (struct prev_slide_info *) arg;
    PRESENTER_VIEW *pv = (PRESENTER_VIEW *) caps->capsuled_pv;
    key = caps->current_index;

    p =  pm->pmm->get_presentation(pm,key);

    if (! p)
    {
        return;
    }

    path = strdup(p->presm->get_path(p));

    /* delete old presentation */
    pm->pmm->del_presentation(pm,key);

    /* check and possibly wait for file provider */
    pv->pvm->check_fprov(pv);

    pres_key = pm->pmm->build_presentation(pm,path);

    if (pres_key <= 0) {
        switch (pres_key) {
            case -L4_ENOMEM:
                pv->pvm->present_log(pv, PRESENTATION_NO_MEM);
                break;

            default:
                pv->pvm->present_log(pv, PRES_INTERNAL_ERROR);
                break;
        }
        return;
    }

    /* check if presentation is corrupted */
    res = pv->pvm->check_slides(pv,pres_key);

    if (res == DAMAGED_PRESENTATION)
    {
        p = pm->pmm->get_presentation(pm,pres_key);
        pm->pmm->del_presentation(pm,key);
        pv->pvm->reinit_presenter_window(pv);
        pv->pvm->update_open_pres(pv);
        return;
    }

    if (pv->curr_pres == key) {
        pv->pvm->show_presentation(pv,pres_key);
    }

   return;
}


static void button_goto_callback(dope_event *e,void *arg) {
    PRESENTER_VIEW *pv = (PRESENTER_VIEW *) arg;
    if (pv->curr_pres < 1) return;

    dope_cmd(pv->app_id,"go_window.open()");
    dope_cmd(pv->app_id,"go_window.top()");
}

static void button_next_callback(dope_event *e,void *arg) {
    PRESENTER_VIEW *pv = (PRESENTER_VIEW *) arg;
    if (pv->curr_pres < 1) return;

    showSlide(pv, arraylist->get_current_index(pv->slide_list)+1);
}

static void button_prev_callback(dope_event *e,void *arg) {
    PRESENTER_VIEW *pv = (PRESENTER_VIEW *) arg;
    if (pv->curr_pres < 1) return;

    showSlide(pv, arraylist->get_current_index(pv->slide_list)-1);
}

static void button_first_callback(dope_event *e,void *arg) {
    PRESENTER_VIEW *pv = (PRESENTER_VIEW *) arg;
    if (pv->curr_pres < 1) return;

    showSlide(pv, 0);
}

static void button_last_callback(dope_event *e,void *arg) {
    PRESENTER_VIEW *pv = (PRESENTER_VIEW *) arg;
    if (pv->curr_pres < 1) return;

    showSlide(pv, arraylist->size(pv->slide_list)-1);
}

static void button_open_presentation(dope_event *e,void *arg) {
    int key;
    struct prev_slide_info *caps = (struct prev_slide_info *) arg;
    PRESENTER_VIEW *pv = (PRESENTER_VIEW *) caps->capsuled_pv;
    key = caps->current_index;

    if (pv->curr_pres != key) {
        pv->pvm->show_presentation(pv,key);
    }
}

static void button_close_presentation(dope_event *e,void *arg) {
    int key;
    struct prev_slide_info *caps = (struct prev_slide_info *) arg;
    PRESENTER_VIEW *pv = (PRESENTER_VIEW *) caps->capsuled_pv;
    key = caps->current_index;

    if (pv->curr_pres == key) {
        pv->pvm->reinit_presenter_window(pv);
    }

    LOGd(_DEBUG,"delete presentation"); 

    pm->pmm->del_presentation(pm,key);

    LOGd(_DEBUG,"update open pres window");

    pv->pvm->update_open_pres(pv);

    LOGd(_DEBUG,"finished button_close event");
}

static void button_help_callback(dope_event *e,void *arg) { 
    PRESENTER_VIEW *pv = (PRESENTER_VIEW *) arg;

    if (!pv->help_is_open) {
        dope_cmd(pv->app_id,"helpw.open()");
        dope_cmd(pv->app_id,"helpw.top()");
        pv->help_is_open=1;
    }
    else {
        dope_cmd(pv->app_id,"helpw.close()");
        pv->help_is_open=0;
    }
}

static void window_pvlog_callback(dope_event *e,void *arg) {
    PRESENTER_VIEW *pv = (PRESENTER_VIEW *) arg;

    dope_cmd(pv->app_id,"pvlogw.close()");
}

static void show_presentation(PRESENTER_VIEW *pv, int curr_key) {
    int i, curr_index;
    ARRAYLIST *al;
    PRESENTATION *p;
    SLIDE *sl;
    u16 *prev_scradr;
    char *pres_name,slide_name[SLIDE_NAME_SIZE];
    struct prev_slide_info *caps;
    al=NULL;

    p = pm->pmm->get_presentation(pm,curr_key);

    if (!p) return;

    pv->curr_pres = curr_key;

    pres_name = p->gen->get_name((PRESENTER *)p);
    LOGd(_DEBUG, "presentation name: %s", pres_name);
    dope_cmdf(pv->app_id, "appw.set(-title %c%s%c)",'"',
              convert2DOpEString(pres_name),'"');

    pv->slide_list = p->presm->get_slides_of_pres(p);

    /* save current index of presentation to set current slide in main vscreen */
    if (arraylist->get_current_index(pv->slide_list) <
        arraylist->size(pv->slide_list)) {
        curr_index = arraylist->get_current_index(pv->slide_list);
    }
    else {
        curr_index=0;
    }

    /* build that nice loadbar window */
    pv->pvm->init_load_display(pv,arraylist->size(pv->slide_list)+1,
                               PRES_CONVERT_SLIDES);

    /* create and set frame preview grid */
    dope_cmd(pv->app_id, "fgr=new Grid()");

    arraylist->set_iterator(pv->slide_list);

    /* fill left frame with small slide vscreens */
    for(i=0;i<arraylist->size(pv->slide_list);i++) {

        sl = (SLIDE *) arraylist->get_next(pv->slide_list);

        sprintf(slide_name,"%s%d",PRES_SLIDE_PREFIX,i+1);

        LOGd(_DEBUG,"slide prev name: %s",slide_name);

        dope_cmdf(pv->app_id, "%s=new VScreen()",slide_name);

        dope_cmdf(pv->app_id, "%s.setmode(%d,%d,\"RGB16\")",slide_name,
                  PREV_W, PREV_H);

        dope_cmdf(pv->app_id, "%s.set(-fixw %d -fixh %d)", slide_name,
                  PREV_W, PREV_H);

        dope_cmdf(pv->app_id, "fgr.place(%s,-column 1 -row %d -pady 1)",
                  slide_name,i+1);

        prev_scradr = vscr_get_fb(pv->app_id, slide_name);
        if (!prev_scradr) {
            LOG_Error("Could not map prev_screen!");
            return;
        }

        display->show_slide((char *)sl->sm->get_content(sl),
                            sl->sm->get_content_size(sl),
                            prev_scradr,PREV_W,PREV_H);

        caps = (struct prev_slide_info *) malloc(sizeof(struct prev_slide_info));
        if (caps == NULL) {
            LOG_Error("not enough memory for malloc caps object");
            break;
        }

        caps->capsuled_pv = pv;
        caps->current_index = arraylist->get_current_index(pv->slide_list);

        dope_bind(pv->app_id,slide_name,"press", &prev_vscreen_callback, (void *)caps);

        dope_cmdf(pv->app_id, "%s.refresh()",slide_name);

        pv->pvm->update_load_display(pv,i+1);
    }

    /* add dummy row to avoid shrinking of main window if we have only a small amount
     * of slides */
    dope_cmdf(pv->app_id, "fgr.rowconfig(9999, -weight 0.1)");
    dope_cmd(pv->app_id, "fprev.set(-content fgr -scrolly yes)");

    showSlide(pv, curr_index);

    pv->pvm->update_load_display(pv,i+1);

    dope_cmd(pv->app_id,"appw.top()");

    return;
}

static int build_view(PRESENTER_VIEW *pv) {
    char req_buf[16], err;

    err = dope_init();

    if (err != 0)
    {
        LOG("DOpE not found!");
        return err;
    }

    /* register DOpE-application */
    pv->app_id = dope_init_app("Presenter");

    /* get screen_height and screenwidth */
    dope_req(pv->app_id,req_buf,sizeof(req_buf),"screen.w");
    fullscr_w = atoi(req_buf);
    dope_req(pv->app_id,req_buf,sizeof(req_buf),"screen.h");
    fullscr_h = atoi(req_buf);

    /* create buttons and set text */
    dope_cmd(pv->app_id, "fscr=new Button()" );
    dope_cmdf(pv->app_id, "fscr.set(-text %s)",FULLSCREEN);
    dope_cmd(pv->app_id, "goto=new Button()" );
    dope_cmd(pv->app_id, "goto.set(-text \"Go to\")");
    dope_cmd(pv->app_id, "first=new Button()" );
    dope_cmdf(pv->app_id, "first.set(-text %s)",FIRST_SLIDE);
    dope_cmd(pv->app_id, "prev=new Button()" );
    dope_cmdf(pv->app_id, "prev.set(-text %s)",PREV_SLIDE);
    dope_cmd(pv->app_id, "next=new Button()" );
    dope_cmdf(pv->app_id, "next.set(-text %s)",NEXT_SLIDE);
    dope_cmd(pv->app_id, "last=new Button()" );
    dope_cmdf(pv->app_id, "last.set(-text %s)",LAST_SLIDE);
    dope_cmd(pv->app_id, "help=new Button()" );
    dope_cmdf(pv->app_id, "help.set(-text %s)",PRES_HELP);

    /* create upper grid and place buttons */
    dope_cmd(pv->app_id, "bg=new Grid()");
    dope_cmd(pv->app_id, "bg.place(fscr,-column 1 -row 1)");
    dope_cmd(pv->app_id, "bg.place(goto,-column 2 -row 1)");
    dope_cmd(pv->app_id, "bg.place(first,-column 3 -row 1)");
    dope_cmd(pv->app_id, "bg.place(prev,-column 4 -row 1)");
    dope_cmd(pv->app_id, "bg.place(next,-column 5 -row 1)");
    dope_cmd(pv->app_id, "bg.place(last,-column 6 -row 1)");
    dope_cmd(pv->app_id, "bg.place(help,-column 7 -row 1)");

    /* create slide grid and info label */
    dope_cmd(pv->app_id, "sg=new Grid()");
    dope_cmd(pv->app_id, "info=new Label()");

    /* create preview frame */
    dope_cmd(pv->app_id, "fprev=new Frame()");

    /* create down grid and place frame and vscreen-grid */
    dope_cmd(pv->app_id, "down=new Grid()" );
    dope_cmd(pv->app_id, "down.place(fprev,-column 1 -row 1)");
    dope_cmd(pv->app_id, "down.place(sg,-column 2 -row 1)" );
    dope_cmdf(pv->app_id, "down.columnconfig(1, -size %d)", PREV_W);

    /* create main grid an place up and down grid */
    dope_cmd(pv->app_id, "gr=new Grid()" );
    dope_cmd(pv->app_id, "gr.place(bg,-column 1 -row 1)" );
    dope_cmd(pv->app_id, "gr.place(down,-column 1 -row 2)" );

    /* create vscreen for slides */
    dope_cmd(pv->app_id, "vscr=new VScreen()" );
    dope_cmdf(pv->app_id, "vscr.setmode(%d,%d,\"RGB16\")",fullscr_w,fullscr_h);

    /* get screenaddress of slide vscreen */
    pv->scradr = vscr_get_fb(pv->app_id, "vscr");
    if (!pv->scradr) {
        LOG_Error("Could not map vscreen!");
        return -ENOMEM;
    }

    /* clear vscreen window */
    memset(pv->scradr,0,sizeof(u16)*fullscr_w*fullscr_h);

    /* add vscreen and info label in grid */
    dope_cmd(pv->app_id, "sg.place(vscr, -column 1 -row 1 -pady 1)");
    dope_cmd(pv->app_id, "sg.place(info,-column 1 -row 2 -align ns)");

    /* create fullscreen window */
    dope_cmd(pv->app_id, "fullw=new Window()" );
    dope_cmd(pv->app_id, "full_vscr=new VScreen()" );
    dope_cmd(pv->app_id, "full_vscr.share(vscr)" );
    dope_cmdf(pv->app_id,
             "fullw.set(-workx 0 -worky 0 -workw %d -workh %d -content full_vscr)",
              fullscr_h, fullscr_w);

    /* create window, place main grid and open window */
    dope_cmd(pv->app_id, "appw=new Window()" );
    dope_cmdf(pv->app_id, "appw.set(-x 200 -y 100 -w %d -h %d -content gr)",
              WIND_W, WIND_H );
    dope_cmd(pv->app_id, "appw.open()" );

    /* window and frame for go_to_slide action */
    dope_cmd(pv->app_id,"go_window = new Window()");
    dope_cmd(pv->app_id,"go_entry=new Entry()");
    dope_cmdf(pv->app_id,"go_window.set(-x 300 -y 80 -content go_entry -title %s)",
              GO_TO_SLIDE);

    /* window and grid for loaded presentations */
    dope_cmd(pv->app_id,"openpw = new Window()");
    dope_cmd(pv->app_id,"openpg = new Grid()");
    dope_cmd(pv->app_id,"openpw.set(-content openpg)");
    dope_cmdf(pv->app_id,"openpw.set(-title %s)",PRES_PRESENTATIONS);

    /* create present_log */
    dope_cmd(pv->app_id,"pvlogw = new Window()");
    dope_cmd(pv->app_id,"pvlogg = new Grid()");
    dope_cmd(pv->app_id,"pvlogw.set(-x 400 -y 500 -content pvlogg)");
    dope_cmdf(pv->app_id,"pvlogw.set(-title %s)",PRESENTER_MESSAGES);

    create_help_window(pv);

    /* bind event handler */
    dope_bind(pv->app_id,"fscr","click", &button_fullscreen_callback, (void *)pv);
    dope_bind(pv->app_id,"goto","click", &button_goto_callback, (void *)pv);
    dope_bind(pv->app_id,"next","click", &button_next_callback, (void *)pv);
    dope_bind(pv->app_id,"prev","click", &button_prev_callback, (void *)pv);
    dope_bind(pv->app_id,"first","click", &button_first_callback, (void *)pv);
    dope_bind(pv->app_id,"last","click", &button_last_callback, (void *)pv);
    dope_bind(pv->app_id,"appw","press", &appw_press_callback, (void *)pv);
    dope_bind(pv->app_id,"full_vscr","press", &full_vscr_press_callback, (void *)pv);
    dope_bind(pv->app_id,"go_entry","commit", &go_entry_commit_callback, (void *)pv);
    dope_bind(pv->app_id,"help","click", &button_help_callback, (void *)pv);
    dope_bind(pv->app_id,"pvlogg","press",&window_pvlog_callback,(void *)pv);

    /* check and possibly wait for file provider */
    pv->pvm->check_fprov(pv);

    return 0;
}

static void check_fprov(PRESENTER_VIEW *pv) {
    int ret;

    /* check if file provider is already online */
    ret = dataprovider->online_check_fprov();

    /* fprov not registered at nameserver until now */
    if (ret==0) {

        /* create wait window */
        dope_cmd(pv->app_id,"waitw= new Window()");
        dope_cmd(pv->app_id,"waitw.set(-x 200 -y 30 -w 132 -h 44)");
        dope_cmd(pv->app_id,"wlabel=new Label()");
        dope_cmdf(pv->app_id,"wlabel.set(-text %s)",PRES_WAIT4_FPROV);
        dope_cmd(pv->app_id,"waitw.set(-content wlabel)");
        dope_cmd(pv->app_id,"waitw.open()");

        /* wait for file provider */
        dataprovider->wait_for_fprov();

        dope_cmd(pv->app_id,"waitw.close()");
    }
}

static void init_load_display(PRESENTER_VIEW *pv, int max_size, char *text) {

    /* create LoadDisplay window */
    dope_cmd(pv->app_id,"progress = new Window()");
    dope_cmd(pv->app_id,"ld = new LoadDisplay()");
    dope_cmdf(pv->app_id,"ld.set(-from 0 -to %d)",max_size);
    dope_cmd(pv->app_id,"ld.barconfig(load, -value 0)");
    dope_cmd(pv->app_id,"progress.set(-x 200 -y 30 -w 160 -h 40 -content ld)");
    dope_cmdf(pv->app_id,"progress.set(-title %s)",text);
    dope_cmd(pv->app_id,"progress.open()");

    dope_cmd(pv->app_id,"progress.top()");

    pv->max_size_load = max_size;
}

static void update_load_display(PRESENTER_VIEW *pv, int value) {

    if (value == CLOSE_LOAD_DISPLAY)
    {
         dope_cmd(pv->app_id,"progress.close()");
    }

    /* update LoadDisplay window */
    dope_cmdf(pv->app_id,"ld.barconfig(load, -value %d)",value);

    if (value >= pv->max_size_load)
    {
        dope_cmd(pv->app_id,"progress.close()");
    }
}

static void eventloop(PRESENTER_VIEW *pv) {
    if (!pv) return;

    /* enter mainloop */
    dope_eventloop(pv->app_id);
} 

static void update_open_pres(PRESENTER_VIEW *pv) {
    int i, size;
    ARRAYLIST *al;
    PRESENTATION *p;
    struct prev_slide_info *caps;
    char pres_name[10], pres_open[10], pres_close[10], pres_reload[10];

    al = pm->pmm->get_presentations(pm);

    if (!al) return;

    arraylist->set_iterator(al);

    size = arraylist->size(al);

    LOGd(_DEBUG,"update_open_pres: amount of presentations: %d",size);

    if (size==0) {
        dope_cmd(pv->app_id,"openpw.close()");
        return;
    }

    dope_cmdf(pv->app_id,"openpg=new Grid()");
    dope_cmd(pv->app_id,"openpw.set(-content openpg)");

    for(i=0;i<size;i++) {

        p = (PRESENTATION *) arraylist->get_next(al);

        sprintf(pres_name,"pr%d",i+1);
        sprintf(pres_open,"opres%d",i+1);
        sprintf(pres_close,"cpres%d",i+1);
        sprintf(pres_reload,"rpres%d",i+1);

        dope_cmdf(pv->app_id,"%s=new Button()",pres_open);
        dope_cmdf(pv->app_id,"%s=new Button()",pres_close);
        dope_cmdf(pv->app_id,"%s=new Button()",pres_reload);
        dope_cmdf(pv->app_id,"%s=new Label()",pres_name);

        dope_cmdf(pv->app_id,"%s.set(-text %s)",pres_open,OPEN_PRESENTATION);
        dope_cmdf(pv->app_id,"%s.set(-text %s)",pres_close,CLOSE_PRESENTATION);
        dope_cmdf(pv->app_id,"%s.set(-text %s)",pres_reload,RELOAD_PRESENTATION);

        dope_cmdf(pv->app_id,"%s.set(-text %c%s%c)",pres_name,'"',
                  convert2DOpEString(p->gen->get_name((PRESENTER *)p)),'"');
        dope_cmdf(pv->app_id,"openpg.place(%s, -column 1 -row %d)",
                  pres_name,i+1);
        dope_cmdf(pv->app_id,"openpg.place(%s, -column 2 -row %d)",
                  pres_open,i+1);
        dope_cmdf(pv->app_id,"openpg.place(%s, -column 3 -row %d)",
                  pres_close,i+1);
        dope_cmdf(pv->app_id,"openpg.place(%s, -column 4 -row %d)",
                  pres_reload,i+1);

        caps = (struct prev_slide_info *) malloc(sizeof(struct prev_slide_info));

        if (caps == NULL) {
            LOG_Error("not enough memory for malloc caps object in update_open_pres");
            break;
        }

        caps->capsuled_pv = pv;
        caps->current_index = p->gen->get_key((PRESENTER *)p);

        LOGd(_DEBUG,"key of presentation set, key: %d",caps->current_index);

        dope_bind(pv->app_id,pres_open,"click",&button_open_presentation,(void *)caps); 
        dope_bind(pv->app_id,pres_close,"click",&button_close_presentation,(void *)caps);
        dope_bind(pv->app_id,pres_reload,"click", &button_reload_callback, (void *)caps);

    }

    dope_cmd(pv->app_id,"openpw.open()");
}

static void reinit_presenter_window(PRESENTER_VIEW *pv) {

    memset(pv->scradr,0,sizeof(u16)*fullscr_w*fullscr_h);

    dope_cmd(pv->app_id,"fprev.set(-content none)");

    dope_cmd(pv->app_id,"fprev.set(-scrolly no)");

    dope_cmdf(pv->app_id,"appw.set(-title %s)",DEFAULT_PRESENTER_TITLE);

    dope_cmd(pv->app_id,"info.set(-text \"\" )");

    /* delete saved current presentation key */
    pv->curr_pres = 0;

}

static int check_slides(PRESENTER_VIEW *pv, int key) {
    PRESENTATION *p = pm->pmm->get_presentation(pm,key);
    ARRAYLIST *al = p->presm->get_slides_of_pres(p);
    int i, j, res, del_slides[arraylist->size(al)];
    char message[4096];
    SLIDE *sl;
    j=0;

    if (! p)
    {
        return -1;
    }

    arraylist->set_iterator(al);

    for (i=0;i<arraylist->size(al);i++) {
        sl  = (SLIDE *) arraylist->get_next(al);

        res = display->check_slide((char *)sl->sm->get_content(sl),
                                   sl->sm->get_content_size(sl));

        /* if res not null slide is damaged, save key of slide in del_slides array */
        if (res != 0) {
            del_slides[j] = sl->gen->get_key((PRESENTER *)sl);
            j++;
        }
    }

    /* true if one or more slide are damaged */
    if (j) {

        /* if all slides are damaged */
        if (j == arraylist->size(al)) {

            /* show error message */
            pv->pvm->present_log(pv,DAMAGED_PRES_MESSAGE);

            return DAMAGED_PRESENTATION;
        }

        /* put damaged message in log window */
        pv->pvm->present_log(pv,DAMAGED_SLIDE_MESSAGE);

        /* delete damaged slides */
        for (i=0;i<j;i++) {

        sl = (SLIDE *) p->presm->get_slide(p,del_slides[i]);

            sprintf(message,"\"%s%c%s\"",p->presm->get_path(p),'/',
                    sl->gen->get_name((PRESENTER *)sl));

            pv->pvm->present_log(pv,message);
            p->presm->del_slide(p,del_slides[i]);

        }

        al = p->presm->get_slides_of_pres(p);
        arraylist->set_iterator(al);

        return -1;
    }
    else {
        al = p->presm->get_slides_of_pres(p);
        arraylist->set_iterator(al);

        return 0;
    }
}

static void present_log(PRESENTER_VIEW *pv, char *text) {

    pv->error_index++;

    dope_cmdf(pv->app_id,"pvlogl%d=new Label()",pv->error_index);

    dope_cmdf(pv->app_id,"pvlogl%d.set(-text %s)",pv->error_index,text);

    dope_cmdf(pv->app_id,"pvlogg.place(pvlogl%d, -column 1 -row %d)",
              pv->error_index,pv->error_index);

    dope_cmd(pv->app_id,"pvlogw.open()");
    dope_cmd(pv->app_id,"pvlogw.top()");
}

static void present_log_reset(PRESENTER_VIEW *pv) {
    dope_cmd(pv->app_id,"pvlogw.close()");

    dope_cmd(pv->app_id,"pvlogg = new Grid()");
    dope_cmd(pv->app_id,"pvlogw.set(-x 300 -y 30 -content pvlogg)");

    dope_bind(pv->app_id,"pvlogg","press",&window_pvlog_callback,(void *)pv);

    pv->error_index=0;
}

void create_help_window(PRESENTER_VIEW *pv) {
    u8 *cbuf_adr, *abuf_adr;          /* VTextScreen character buffer */

    /* create help window */
    dope_cmd(pv->app_id,"helpw = new Window()");
    dope_cmdf(pv->app_id,"helpw.set(-title %s)",PRES_HELP_WINDOW_TITLE);

    dope_cmd(pv->app_id, "vts = new VTextScreen()" );
    dope_cmdf(pv->app_id, "vts.setmode(%d, %d, C8A8PLN)", PRES_HELP_W, PRES_HELP_H);
    dope_cmd(pv->app_id, "helpw.set(-background off -content vts)" );

    /* map vscreen buffer to local address space */
    cbuf_adr = vscr_get_fb(pv->app_id, "vts");

    if (!cbuf_adr) {
        LOG_Error("Could not map VTextScreen!");
        return;
    }

    abuf_adr = cbuf_adr + PRES_HELP_W*PRES_HELP_H;

    sprintf(cbuf_adr,"%s",PRES_HELP_POSSIBLE_CO);
    sprintf(cbuf_adr+PRES_HELP_W*2,"%s",PRES_HELP_SPACE);
    sprintf(cbuf_adr+PRES_HELP_W*3,"%s",PRES_HELP_RIGHT_AR);
    sprintf(cbuf_adr+PRES_HELP_W*4,"%s",PRES_HELP_PAGE_DOWN);
    sprintf(cbuf_adr+PRES_HELP_W*5,"%s",PRES_HELP_LEFT_MOUSE);
    sprintf(cbuf_adr+PRES_HELP_W*6,"%s",PRES_HELP_RIGHT_MOUSE);
    sprintf(cbuf_adr+PRES_HELP_W*7,"%s",PRES_HELP_PAGE_UP);
    sprintf(cbuf_adr+PRES_HELP_W*8,"%s",PRES_HELP_LEFT_AR);
    sprintf(cbuf_adr+PRES_HELP_W*9,"%s",PRES_HELP_ESCAPE);

    memset(abuf_adr,0x38,sizeof(u8)*PRES_HELP_W*PRES_HELP_H);

    dope_cmd(pv->app_id,"vts.refresh()");

}

char * convert2DOpEString(char *text)
{
    int i;

    for (i=0;i<strlen(text);i++)
    {
        if (text[i] == '"')
            text[i] = '\'';
    }

    return text;
}

void showSlide(PRESENTER_VIEW *pv, int slide_nr)
{
    SLIDE *sl;

    /* undefined index */
    if (slide_nr > (arraylist->size(pv->slide_list)-1) || slide_nr < 0)
    return;

    sl = (SLIDE *) arraylist->get_elem(pv->slide_list, slide_nr);
    display->show_slide((char *)sl->sm->get_content(sl), 
                         sl->sm->get_content_size(sl),
                         pv->scradr, fullscr_w, fullscr_h);

    dope_cmd(pv->app_id,"vscr.refresh()");

    dope_cmdf(pv->app_id,"fprev.expose(0,%d)",
              arraylist->get_current_index(pv->slide_list)
              * (PREV_H + 2)
              + (PREV_H>>1));

    /* update info label */
    dope_cmdf(pv->app_id, "info.set(-text %c %d %c %d %c)",'\"',
              arraylist->get_current_index(pv->slide_list)+1,'/',
              arraylist->size(pv->slide_list),'\"');
}

/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct presenter_view_methods pv_meth = {
    show_presentation,
    build_view,
    eventloop,
    update_open_pres,
    reinit_presenter_window,
    init_load_display,
    check_fprov,
    update_load_display,
    check_slides,
    present_log,
    present_log_reset,
};

static PRESENTER_VIEW *create(void) {
    PRESENTER_VIEW *new = malloc(sizeof(PRESENTER_VIEW)+
                          sizeof(struct presenter_data));

    if (new == NULL) {
        LOG_Error("not enough memory for creation of PRESENTER_VIEW");
        return NULL;
    }

    new->pdat   = (struct presenter_data *)((long)new + sizeof(PRESENTER_VIEW));

    new->gen            = &gen_methods;
    new->pvm            = &pv_meth;
    new->curr_pres      = 0;
    new->error_index    = 0;
    new->help_is_open   = 0;

    return new;
}

static struct presenter_view_services services = {
    create,
};

int init_presenter_view(struct presenter_services *p) {

    arraylist = p->get_module(ARRAYLIST_MODULE);
    display   = p->get_module(DISPLAY_MODULE);
    dataprovider = p->get_module(DATAPROVIDER_MODULE);
    presenter = p->get_module(PRESENTER_MODULE);

    /* get default methods */
    presenter->default_presenter_methods(&gen_methods);

    p->register_module(PRESENTER_VIEW_MODULE,&services);

    return 1;
}
