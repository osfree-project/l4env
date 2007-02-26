/*
 * \brief   presenter_server implemtation
 * \date    2003-04-11
 * \author  Jens Syckor <js712688@inf.tu-dresden.de>
 *
 */

#include <l4/dm_phys/dm_phys.h>

#include "controller/presenter_server.h"
#include "presenter-server.h"
#include "util/arraylist.h"
#include "model/slide.h"
#include "util/presenter_conf.h"
#include "model/presenter.h"
#include "model/presentation.h"
#include "model/presmanager.h"
#include "util/memory.h"
#include "util/module_names.h"
#include "controller/presenter_view.h"

static struct presmanager_services *presman;
static struct presenter_view_services *pres_view;
PRESMANAGER *pm;

extern int _DEBUG;

int init_presenter_server (struct presenter_services *);

/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

/*** START SERVING ***/
static void start(void) {
	LOG("presenter_server started, waiting for calls");
	pm = presman->create();
	presenter_server_loop(0);

}


/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct presenter_server_services services = {
        start,
};



CORBA_int presenter_load_component(CORBA_Object _dice_corba_obj,
                                                        const_CORBA_char_ptr pathname,
                                                        CORBA_Environment *_dice_corba_env)
{       
	return pm->pmm->build_presentation(pm,(char *)pathname);
}



CORBA_int presenter_show_component(CORBA_Object _dice_corba_obj,
                                                        CORBA_int presentation_id,
                                                        CORBA_Environment *_dice_corba_env)
{
	PRESENTATION *p = pm->pmm->get_presentation(pm,presentation_id);
	PRESENTER_VIEW *pv = pres_view->create();
	pv->pvm->show_presentation(pv,p);	
	return 1;
}

int init_presenter_server(struct presenter_services *p) {

	presman 	= p->get_module(PRESMANAGER_MODULE);
	pres_view	= p->get_module(PRESENTER_VIEW_MODULE);

	p->register_module(PRESENTER_SERVER_MODULE,&services);

	return 1;

}
