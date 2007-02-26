/*
 * \brief   presenter_server implementation
 * \date    2003-04-11
 * \author  Jens Syckor <js712688@inf.tu-dresden.de>
 *
 */

#include <l4/sys/syscalls.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>
#include <l4/env/errno.h>

#include "presenter-server.h"

#include "properties/view.properties"
#include "util/arraylist.h"
#include "model/slide.h"
#include "util/presenter_conf.h"
#include "model/presenter.h"
#include "model/presentation.h"
#include "model/presmanager.h"
#include "util/module_names.h"
#include "controller/presenter_server.h"
#include "controller/presenter_view.h"
#include <l4/presenter/presenter_lib.h>

static struct presmanager_services *presman;
static struct presenter_view_services *pres_view;
PRESMANAGER *pm;
PRESENTER_VIEW *pv;
l4_threadid_t presenter_thread_id;

extern int _DEBUG;

int init_presenter_server (struct presenter_services *);

/**********************************/
/*** FUNCTIONS FOR INTERNAL USE ***/
/**********************************/


static void server_thread(void *arg) {
    CORBA_Server_Environment env = dice_default_server_environment;

    if (!names_register(PRESENTER_NAME)) {
            LOG("Presenter: can't register at nameserver\n");
            return;
    }

    presenter_server_loop(&env);
}


/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

/*** START SERVING ***/
static void start(char *default_path) {
    int err, res, pres_key;

    pm = presman->create();
    pv = pres_view->create();

    if (pv == NULL) {
        LOG("could not build presenter, not enough memory");
        return;
    }

    /* build main window and wait for presenter_fprov */
    err = pv->pvm->build_view(pv);

    if (err != 0) {
        return;
    }

    presenter_thread_id = l4_myself();

    l4thread_create(&server_thread,NULL,L4THREAD_CREATE_ASYNC);

    if (default_path != NULL) {

        /* ok, now try to load from default path */
        pres_key = pm->pmm->build_presentation(pm,default_path);

        if (pres_key > 0) {
            res = pv->pvm->check_slides(pv,pres_key);

            if (res != DAMAGED_PRESENTATION) {
                pv->pvm->update_open_pres(pv);
                pv->pvm->show_presentation(pv,pres_key);
            }
            else {
                pm->pmm->del_presentation(pm,pres_key);
            }
        }

    }

    pv->pvm->eventloop(pv);
}

/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct presenter_server_services services = {
        start,
};

CORBA_int presenter_load_component(CORBA_Object _dice_corba_obj,
                                                        const_CORBA_char_ptr pathname,
                                                        CORBA_Server_Environment *_dice_corba_env)
{
    int res, pres_key;
    pv->pvm->check_fprov(pv);
    pres_key = pm->pmm->build_presentation(pm,(char *)pathname);

    if (pres_key <= 0) {
        pv->pvm->present_log_reset(pv);
        switch (pres_key) {
            case -L4_ENOMEM:
            pv->pvm->present_log(pv, PRESENTATION_NO_MEM);
            break;

            default:
            pv->pvm->present_log(pv, PRES_INTERNAL_ERROR);
            break;
        }
        return pres_key;
    }

    res = pv->pvm->check_slides(pv,pres_key);

    if (res == DAMAGED_PRESENTATION) {
        pm->pmm->del_presentation(pm,pres_key);
        return res;
    }

    pm->pmm->transfer_rights(pm,pres_key);

    pv->pvm->update_open_pres(pv);

    return pres_key;
}



CORBA_int presenter_show_component(CORBA_Object _dice_corba_obj,
                                                        CORBA_int presentation_id,
                                                        CORBA_Server_Environment *_dice_corba_env)
{
    if (presentation_id > 0) {
        pv->pvm->show_presentation(pv,presentation_id);
        return 0;
    }
    else return -1;
}

int init_presenter_server(struct presenter_services *p) {
    pres_view   = p->get_module(PRESENTER_VIEW_MODULE);
    presman     = p->get_module(PRESMANAGER_MODULE);

    p->register_module(PRESENTER_SERVER_MODULE,&services);

    return 1;

}
