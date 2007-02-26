/*
 * \brief   presentation manager class module
 * \date    2003-04-11
 * \author  Jens Syckor <js712688@inf.tu-dresden.de>
 *
 * This file implements a presentation manager which
 * manages and contains all presentations.
 */

struct private_presentermanager;
#define PRESMANAGER struct private_presentermanager
#define PRESENTER PRESMANAGER

#define MAX_ENTRIES 256

#include <stdio.h>

#include <l4/dm_mem/dm_mem.h>
#include <l4/thread/thread.h>
#include <l4/env/errno.h>

#include "controller/presenter_encapl4x.h"

#include "model/presenter.h"
#include "util/presenter_conf.h"
#include "util/arraylist.h"
#include "util/keygenerator.h"
#include "util/module_names.h"
#include "util/dataprovider.h"
#include "model/slide.h"
#include "model/presentation.h"
#include "model/presmanager.h"
#include "controller/presenter_view.h"
#include "properties/view.properties"

#define _DEBUG 0

extern l4_threadid_t presenter_thread_id;

static struct presenter_general_services *presenter;
static struct slide_services *sld_serv;
static struct presentation_services *pres_serv;
static struct keygenerator_services *keygen_serv;
static struct arraylist_services *arraylist;
static struct dataprovider_services *dataprovider;
static struct presenter_encapl4x_services *pres_encapl4x_serv;

PRESMANAGER {
    /* all general methods */
    struct presenter_methods *gen;

    /* all special methods of each slide */
    struct presmanager_methods *pmm;

    /* all general data */
    struct presenter_data *pdat;

    /* private data */
    ARRAYLIST *presentation_list;

    KEYGENERATOR *keygen;

};

int init_presmanager(struct presenter_services *);
static int build_presentation_config(PRESMANAGER *, char *);
static int build_presentation_binary(PRESMANAGER *, char *);
static int handle_presentation_error(PRESMANAGER *, PRESENTATION *, int, char *);

static struct presenter_methods gen_methods;

PRESENTER_VIEW *pv;

/**********************************/
/*** FUNCTIONS FOR INTERNAL USE ***/
/**********************************/

static int handle_presentation_error(PRESMANAGER *pm, PRESENTATION *p, int error, char *msg)
{
    char *log_msg;

    switch (error) {
        case -L4_ENOMEM:
            pv->pvm->update_load_display(pv, CLOSE_LOAD_DISPLAY);
            p->presm->del_all_slides(p);
            pv->pvm->present_log(pv, PRESENTATION_NO_MEM);
            free(p);
            break;

        case -L4_ENOENT:
            log_msg = malloc(strlen(msg) + strlen(PRES_FILE_NOT_FOUND_ERROR) + 4);
            if (log_msg)
            {
                snprintf(log_msg, strlen(msg) + strlen(PRES_FILE_NOT_FOUND_ERROR) + 4,
                         "\"%s %s\"", PRES_FILE_NOT_FOUND_ERROR, msg);
                pv->pvm->present_log(pv, log_msg);
                free(log_msg);
            }
            break;

        default:
            pv->pvm->present_log(pv, PRES_INTERNAL_ERROR);
            break;
    }

    return 0;
}

static char * calc_slide_path(char *path, char *slide_name) {
    int name_length = 0;
    int path_length = 0;
    char *complete_path;

    /* calculate length of slide name*/
    name_length = strlen(slide_name);
    path_length = strlen(path);

    LOGd(_DEBUG,"name_length: %d, path_length: %d, slide_name: %s",
         name_length,path_length,slide_name);

    /* alloc memory for complete path */
    complete_path = malloc(path_length+name_length+1);

    if (! complete_path)
    {
        LOG_Error("Not enough memory!");
        return NULL;
    }

    snprintf(complete_path,path_length+name_length+1,"%s%s",path,slide_name);

    LOGd(_DEBUG,"slide_path: %s",complete_path);

    return complete_path;
}

/*** calculate the name of the file from the configuration file name ***/
static char * calc_real_name(char *fname, int cut_extension) {
    int name_length;
    char *real_name, *name;
    name_length=0;

    /* search for last backslash */
    name = strrchr(fname,47);

    /* if no backslash found restore pointer */
    if (name==NULL) name=fname;
    else name++;

    if (cut_extension) return strdup(name);

    /* calc the length of the config file name */
    while(name[name_length] != '.' && name[name_length] != '\0') name_length++;

    real_name = (char *) malloc(name_length+1);

    if (! real_name)
    {
        LOG_Error("Not enough memory!");
        return NULL;
    }

    snprintf(real_name,name_length+1,"%s",name);

    LOGd(_DEBUG,"real name: %s",real_name);

    return real_name;
}


/***************************************/
/***  PRESMANAGER SPECIFIC METHODS   ***/
/**************************************/

static int presmanager_build_presentation(PRESMANAGER *pm, char *fname) {
    if (strstr(fname,".pres") != NULL) return build_presentation_config(pm,fname);
    else return build_presentation_binary(pm,fname);
}

static int build_presentation_config (PRESMANAGER *pm, char *fname) {
    int index,size,present_key,res, num_slides = 0;
    SLIDE *sld;
    ARRAYLIST *al;
    PRESENTATION *p;
    l4dm_dataspace_t *ds;
    char *path_slide, *slide_name, *pres_path, *pres_name;

    pv->pvm->present_log_reset(pv);

    if (!pm || !fname)
    {
        return -L4_EINVAL;
    }

    /* create presentation */
    p = pres_serv->create();

    if (! p)
    {
        res = -L4_ENOMEM;
        handle_presentation_error(pm, p, res, fname);
        return res;
    }

    /* load config file and parse it */
    al = dataprovider->load_config(fname);

    if (! al) {
        LOG_Error("Could not load presentation.");
        return -L4_EINVAL;
    }

    /* open load display, should be moved later to a controller class */
    pv->pvm->init_load_display(pv,arraylist->size(al)-1,PRES_LOADING_SLIDES);

    LOGd(_DEBUG,"calc real name of presentation");

    /* calculate name of the presentation from name of configuration file */
    pres_name = calc_real_name(fname,0);

    if (! pres_name)
    {
        res = -L4_ENOMEM;
        handle_presentation_error(pm, p, res, fname);
        return res;
    }

    arraylist->set_iterator(al);
    size = arraylist->size(al);

    /* get directory of slides */
    pres_path = arraylist->get_next(al);

    /* set name and path of presentation */
    p->gen->set_name((PRESENTER *)p,pres_name);
    p->presm->set_path(p,fname);

    for (index=1;index<size;index++) {

        /* get slide name */
        slide_name = arraylist->get_next(al);

        /* build path name for loading slide */
        path_slide = calc_slide_path(pres_path,slide_name);

        ds = (l4dm_dataspace_t *) malloc(sizeof(l4dm_dataspace_t));

        if (! ds || ! path_slide)
        {
            res = -L4_ENOMEM;
            handle_presentation_error(pm, p, res, pres_name);
            return res;
        }

        /* try to load slide and break on error */
        if((res = dataprovider->load_content(path_slide,ds)) > 0) {

            /* create a slide */
            sld = sld_serv->create();

            /* set name and ds in slide */
            slide_name = calc_real_name(slide_name,1);

            if (! slide_name)
            {
                res = -L4_ENOMEM;
                handle_presentation_error(pm, p, res, pres_name);
                return res;
            }

            sld->gen->set_name((PRESENTER *)sld,slide_name);
            if ((res = sld->sm->set_content((SLIDE *)sld,ds)) == 0) {

                /* add slide in presentation */
                p->presm->add_slide(p,sld);
                num_slides++;
            }
            else
            {
                free(sld);
                handle_presentation_error(pm, p, res, slide_name);
                if (res == -L4_ENOMEM)
                    return res;
            }
        }
        else
        {
            handle_presentation_error(pm, p, res, path_slide);
            if (res == -L4_ENOMEM)
                return res;
        }

        pv->pvm->update_load_display(pv,index);
    }

    /* add presentation to presmanager map if minimum one slide was loaded */
    if (num_slides > 0) {
        pm->pmm->add_presentation(pm,p);
    }
    else return -1;

    /* get key of presentation */
    present_key = p->gen->get_key((PRESENTER *)p);

    LOGd(_DEBUG,"build presentation, key (%d)", present_key);

    return present_key;
}

static int build_presentation_binary(PRESMANAGER *pm, char *fname) {
    PRESENTATION *p;
    SLIDE *sld;
    PRESENTER_ENCAPL4X *encap;
    l4dm_dataspace_t *ds;
    int present_key, pages,i;

    if (!pm || !fname) return -1;

    ds = (l4dm_dataspace_t *) malloc(sizeof(l4dm_dataspace_t));

    encap = pres_encapl4x_serv->create();

    /* try to load slide and break on error */
    if(dataprovider->load_content(fname,ds) > 0) {

        /* create presentation */
        p = pres_serv->create();

        pages = encap->encapl4xm->put(encap,calc_real_name(fname,1),ds);

        LOGd(_DEBUG,"amount of pages in file == %d",pages);

        /* something was wrong binary has no pages */
        if (pages <= 0) return -1;

        for (i=1;i<=pages;i++) {

            ds = (l4dm_dataspace_t *) malloc(sizeof(l4dm_dataspace_t));

            /* create a slide */
            sld = sld_serv->create();

            encap->encapl4xm->get(encap,i,ds);

            if(sld->sm->set_content((SLIDE *)sld,ds)==0) {
                    /* add slide in presentation */
                    p->presm->add_slide(p,sld);
            }
        }

        /* set name of presentation */
        p->gen->set_name((PRESENTER *)p,calc_real_name(fname,0));
        p->presm->set_path(p,fname);

        pm->pmm->add_presentation(pm,p);

        /* get key of presentation */
        present_key = p->gen->get_key((PRESENTER *)p);

        return present_key;
    }
    else return -1;
}

static void presmanager_add_presentation (PRESMANAGER *pm, PRESENTATION *p) {

    if (!pm) return;

    /* add slide in list of the given presentation */
    arraylist->add_elem(pm->presentation_list,p);

    p->gen->set_key((PRESENTER *)p,keygen_serv->get_key(pm->keygen));
}

static void presmanager_del_presentation (PRESMANAGER *pm, int presentation_key) {
    int i,key;
    PRESENTATION *p;

    arraylist->set_iterator(pm->presentation_list);

    for (i=0;i<arraylist->size(pm->presentation_list);i++) {
        p = (PRESENTATION *) arraylist->get_next(pm->presentation_list);

        key = p->gen->get_key((PRESENTER *)p);

        if (key == presentation_key) {

            arraylist->remove_elem(pm->presentation_list,
                    arraylist->get_current_index(pm->presentation_list));

            p->presm->del_all_slides(p);

            free(p);
            keygen_serv->release_key(pm->keygen,key);
            break;
        }
    }

}

static PRESENTATION * presmanager_get_presentation (PRESMANAGER *pm, int key) {
    int i,pres_key;
    PRESENTATION *p;
    p = NULL;
    if (!pm || key < 1) return NULL;

    arraylist->set_iterator(pm->presentation_list);

    for (i=0;i<arraylist->size(pm->presentation_list);i++) {
        p = (PRESENTATION *) arraylist->get_next(pm->presentation_list);

        pres_key = p->gen->get_key((PRESENTER *)p);

        if (pres_key==key) break;

    }
    return p;
}

static ARRAYLIST * presmanager_get_presentations (PRESMANAGER *pm) {
    if (!pm) return NULL;
    else return pm->presentation_list;
}

static void presmanager_transfer_rights(PRESMANAGER *pm, int presentation_key) {
    int i;
    PRESENTATION *p;
    SLIDE *sl;
    ARRAYLIST *al;
    l4_threadid_t my_id;

    my_id = l4thread_l4_id(l4thread_myself());

    p = pm->pmm->get_presentation(pm,presentation_key);

    if (!p) return;

    al = p->presm->get_slides_of_pres(p);

    arraylist->set_iterator(al);

    for (i=0;i<arraylist->size(al);i++) {
        sl = (SLIDE *) arraylist->get_next(al);
        l4dm_share(sl->sm->get_ds(sl),my_id,L4DM_RO);
        l4dm_transfer(sl->sm->get_ds(sl),presenter_thread_id);
    }

    arraylist->set_iterator(al);
}

static struct presmanager_methods pm_methods = {
    .build_presentation = presmanager_build_presentation,
    .add_presentation   = presmanager_add_presentation,
    .del_presentation   = presmanager_del_presentation,
    .get_presentation   = presmanager_get_presentation,
    .get_presentations  = presmanager_get_presentations,
    .transfer_rights    = presmanager_transfer_rights,
};

/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

static PRESMANAGER *create(void) {
    PRESMANAGER *new = (PRESMANAGER *)malloc(sizeof(PRESENTATION)+
                        sizeof(struct presenter_data));
    new->gen     = &gen_methods;
    new->pmm     = &pm_methods;

    new->keygen = keygen_serv->create(MAX_ENTRIES);
    new->presentation_list = arraylist->create();

    return new;
}

/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct presmanager_services services = {
    create,
};

int init_presmanager(struct presenter_services *p) {
    presenter = p->get_module(PRESENTER_MODULE);
    arraylist = p->get_module(ARRAYLIST_MODULE);
    keygen_serv = p->get_module(KEYGENERATOR_MODULE);
    dataprovider = p->get_module(DATAPROVIDER_MODULE);
    sld_serv     = p->get_module(SLIDE_MODULE);
    pres_serv    = p->get_module(PRESENTATION_MODULE);
    pres_encapl4x_serv = p->get_module(ENCAPL4X_MODULE);

    p->register_module(PRESMANAGER_MODULE,&services);

    /* get default methods */
    presenter->default_presenter_methods(&gen_methods);

    return 1;
}
