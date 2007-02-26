/*
 * \brief   presenter view controller module
 * \date    2003-04-11
 * \author  Jens Syckor <js712688@inf.tu-dresden.de>
 *
 * This file implements the controller which interacts with DOpE.
 */

struct private_view;
#define PRESENTER_VIEW struct private_view

#include <string.h>

#include <l4/dm_phys/dm_phys.h>

#include <l4/dope/vscreen.h>        /* DOpE VScreen lib */

#include "properties/view.properties"
#include "util/arraylist.h"
#include "util/presenter_conf.h"
#include "model/presenter.h"
#include "util/memory.h"
#include "util/module_names.h"
#include "model/slide.h"
#include "model/presentation.h"
#include "controller/presenter_view.h"
#include "view/display.h"

PRESENTER_VIEW {
	struct presenter_view_methods *pvm;
	
	u16 *scradr;
	u16 *scradr_fullscr;
	long app_id;                 /* DOpE application id */
	PRESENTATION *p;
	ARRAYLIST *slide_list;

};

#define WIND_H 320  
#define WIND_W 480 

#define FULLSCR_H 768
#define FULLSCR_W 1024

#define VSCR_H 768
#define VSCR_W 1024

extern int _DEBUG;

static struct arraylist_services *arraylist;
static struct pres_display_services *display;
static struct memory_services *mem;

int init_presenter_view(struct presenter_services *);

static void full_vscr_press_callback(dope_event *e,void *arg) {
        char curr_ascii;

 	PRESENTER_VIEW *pv = (PRESENTER_VIEW *) arg;

        if (e->type == EVENT_TYPE_PRESS) {
		curr_ascii = dope_get_ascii(pv->app_id, e->press.code);

		switch (curr_ascii) {
		case 'g':
			dope_cmd(pv->app_id,"b.close()");
			dope_cmd(pv->app_id,"a.top()");
			break;
		 }
	}
}

static void vscr_press_callback(dope_event *e,void *arg) {
        char curr_ascii;
	SLIDE *sl;

	PRESENTER_VIEW *pv = (PRESENTER_VIEW *) arg;
	
        if (e->type == EVENT_TYPE_PRESS) {
                curr_ascii = dope_get_ascii(pv->app_id, e->press.code);

                switch (e->press.code) {
                case 105: /* left arrow */
			sl = (SLIDE *)	arraylist->get_prev(pv->slide_list);
			display->show_slide((char *)sl->sm->get_content_addr(sl),pv->scradr);
			dope_cmd(pv->app_id,"vscr.refresh()");
                        break;
                case 106: /* right arrow */
			sl = (SLIDE *) arraylist->get_next(pv->slide_list);
			display->show_slide((char *)sl->sm->get_content_addr(sl),pv->scradr);
			dope_cmd(pv->app_id,"vscr.refresh()");
                        break;
                }

                switch (curr_ascii) {
                case 'f':
                      	dope_cmdf(pv->app_id, "b.set(-x -5 -y -22 -w %d -h %d)", FULLSCR_W, FULLSCR_H );
			dope_cmd(pv->app_id, "b.open()");
                        break;

                case 'g':
                      	dope_cmdf(pv->app_id, "a.set(-x 200 -y 100 -w %d -h %d)", WIND_W, WIND_H );
                        break;
                }
        }
}

static void show_presentation(PRESENTER_VIEW *pv, PRESENTATION *p) {
	ARRAYLIST *al;
	SLIDE *sl;
	al = NULL;
	if (!pv) return;

        dope_init();

        /* register DOpE-application */
        pv->app_id = dope_init_app(p->gen->get_name((PRESENTER *)p));

	pv->p = p;

	pv->slide_list = p->presm->get_slides_of_pres(p);
	arraylist->set_iterator(pv->slide_list);

	/* buttons and set text */
	dope_cmd(pv->app_id, "fscr=new Button()" );
	dope_cmdf(pv->app_id, "fscr.set(-text %s)",FULLSCREEN);
	dope_cmd(pv->app_id, "first=new Button()" );
        dope_cmdf(pv->app_id, "first.set(-text %s)",FIRST_SLIDE);
	dope_cmd(pv->app_id, "prev=new Button()" );
        dope_cmdf(pv->app_id, "prev.set(-text %s)",PREV_SLIDE);
	dope_cmd(pv->app_id, "next=new Button()" );
        dope_cmdf(pv->app_id, "next.set(-text %s)",NEXT_SLIDE);
	dope_cmd(pv->app_id, "last=new Button()" );
        dope_cmdf(pv->app_id, "last.set(-text %s)",LAST_SLIDE);

	/* create spacer grid */
	dope_cmd(pv->app_id, "dummy=new Grid()" );
	dope_cmd(pv->app_id, "dummy.columnconfig(1, -size 50)" );

	/* create upper grid and place buttons */
	dope_cmd(pv->app_id, "bg=new Grid()");
	dope_cmd(pv->app_id, "bg.place(fscr,-column 1 -row 1 -padx 2 -pady 2)");
	dope_cmd(pv->app_id, "bg.place(dummy,-column 2 -row 1 -padx 2 -pady 2)");
	dope_cmd(pv->app_id, "bg.place(first,-column 3 -row 1 -padx 2 -pady 2)");
	dope_cmd(pv->app_id, "bg.place(prev,-column 4 -row 1 -padx 2 -pady 2)");
	dope_cmd(pv->app_id, "bg.place(next,-column 5 -row 1 -padx 2 -pady 2)");
	dope_cmd(pv->app_id, "bg.place(last,-column 6 -row 1 -padx 2 -pady 2)");

	/* create vscreen for slides */
        dope_cmd(pv->app_id, "vscr=new VScreen()" );
	dope_cmdf(pv->app_id, "vscr.setmode(%d,%d,\"RGB16\")",VSCR_W,VSCR_H);

	/* create grid and place vscreen */
	dope_cmd(pv->app_id, "sg=new Grid()" );
        dope_cmd(pv->app_id, "sg.place(vscr, -column 1 -row 1)");

	/* create down grid, preview frame and preview vscr */
	dope_cmd(pv->app_id, "fprev=new Frame()" );
	dope_cmd(pv->app_id, "prev_vscr=new VScreen()" );
	dope_cmdf(pv->app_id, "prev_vscr.setmode(128,%d,\"RGB16\")",VSCR_H);
	dope_cmd(pv->app_id, "fprev.set(-scrollx yes -scrolly yes -content prev_vscr)" );

	/* create down grid and place frame and vscreen-grid */
	dope_cmd(pv->app_id, "down_g=new Grid()" );
	dope_cmd(pv->app_id, "down_g.place(fprev,-column 1 -row 1)" );
	dope_cmd(pv->app_id, "down_g.place(sg,-column 2 -row 1)" );
	dope_cmd(pv->app_id, "down_g.columnconfig(1, -size 128)" );

	/* create main grid an place up and down grid */
	dope_cmd(pv->app_id, "g=new Grid()" );
	dope_cmd(pv->app_id, "g.place(bg,-column 1 -row 1)" );
	dope_cmd(pv->app_id, "g.place(down_g,-column 1 -row 2)" );
	dope_cmd(pv->app_id, "g.rowconfig(1,-size 25)" );

	/* create window, place main grid and open window */
	dope_cmd(pv->app_id, "a=new Window()" );
        dope_cmdf(pv->app_id, "a.set(-x 200 -y 100 -w %d -h %d -fitx yes -fity yes -content g)",WIND_W, WIND_H );
        dope_cmd(pv->app_id, "a.open()" );

	/* create vscreen, grid and window for fullscreen */
	dope_cmd(pv->app_id, "b=new Window()" );
	dope_cmd(pv->app_id, "full_vscr=new VScreen()" );
	dope_cmd(pv->app_id, "full_vscr.share(vscr)" );
	dope_cmdf(pv->app_id, "b.set(-x 20 -y 10 -w %d -h %d -fitx yes -fity yes -content full_vscr)", FULLSCR_H, FULLSCR_W);

	/* bind event handler */
	dope_bind(pv->app_id,"vscr","press",  pv->pvm->vscr_press_callback,  (void *)pv);
	dope_bind(pv->app_id,"full_vscr","press", &full_vscr_press_callback,  (void *)pv);
	dope_bind(pv->app_id,"first","click", &full_vscr_press_callback,  (void *)pv);
	 

	/* get screenaddress of slide vscreen */
	pv->scradr = vscr_get_fb(pv->app_id, "vscr");
        if (!pv->scradr) {
                LOG("Could not map vscreen!\n");
		return;
        } 

	/* now show first slide */
	sl = (SLIDE *) arraylist->get_next(pv->slide_list);
	display->show_slide((char *)sl->sm->get_content_addr(sl),pv->scradr);
	dope_cmd(pv->app_id,"vscr.refresh()");
	
        /* enter mainloop */
        dope_eventloop(pv->app_id);
        return;

}

/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct presenter_view_methods pv_meth = {
	show_presentation,
	vscr_press_callback,
};

static PRESENTER_VIEW *create(void) {
        PRESENTER_VIEW *new = mem->alloc(sizeof(PRESENTER_VIEW));
        new->pvm        = &pv_meth;

        return new;
}

static struct presenter_view_services services = {
	create,
};

int init_presenter_view(struct presenter_services *p) {
	mem 	  =  p->get_module(MEMORY_MODULE);
	arraylist = p->get_module(ARRAYLIST_MODULE);
	display	  = p->get_module(DISPLAY_MODULE);
	p->register_module(PRESENTER_VIEW_MODULE,&services);

	return 1;
}
