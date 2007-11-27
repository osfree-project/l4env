/*
 * \brief   dataprovider class module
 * \date    2003-04-11
 * \author  Jens Syckor <js712688@inf.tu-dresden.de>
 *
 * This file implements a dataprovider which loads all
 * data from unsecure l4linux side.
 */

#include <l4/dm_mem/dm_mem.h>

#include "presenter_conf.h"
#include "arraylist.h"
#include "module_names.h"
#include "dataprovider.h"

#include <fcntl.h>
#include <l4/generic_fprov/generic_fprov-client.h>
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/sys/consts.h>
#include <l4/names/libnames.h>

#include <dice/dice.h>

#define _DEBUG 0

l4_threadid_t id = L4_INVALID_ID;
static CORBA_Object _dice_corba_obj = &id;
// CORBA_Environment _dice_corba_env = dice_default_environment;

static struct arraylist_services *arraylist;

int init_dataprovider(struct presenter_services *p);

ARRAYLIST * parse_config_ds(char *content);

/**********************************/
/*** FUNCTIONS FOR INTERNAL USE ***/
/**********************************/

ARRAYLIST * parse_config_ds(char *content) {
    ARRAYLIST *al = arraylist->create();
    char *pathname;
    int i,k;
    i=0;
    k=0;

    while (content[i] != '\0') {
        while (content[i] != '\n') i++;
        content[i] = '\0';

        pathname = malloc(i-k+1);

        strcpy(pathname,&content[k]);

        LOGd(_DEBUG,"parsed pathname == %s",pathname);

        arraylist->add_elem(al,pathname);
        k=i+1;
        i++;

        LOGd(_DEBUG,"added element");
    }

    LOGd(_DEBUG,"parsing finished successfully");
    return al;
}

/************************/
/**  SPECIFIC METHODS  **/
/************************/

static ARRAYLIST * dataprovider_load_config(char *fname) {
    l4_size_t ds_size;
    void *content_addr;
    l4_int32_t result;
    l4dm_dataspace_t ds;
    ARRAYLIST *list;
	CORBA_Environment _dice_corba_env = dice_default_environment;

    result = l4fprov_file_open_call(_dice_corba_obj, fname, &L4DM_DEFAULT_DSM, 0,
                                    &ds, &ds_size, &_dice_corba_env);

    if (result == -L4_ENOTFOUND) {
        LOG("file not found exception, fname: %s",fname);
        return NULL;
    }

    result = l4rm_attach(&ds, ds_size, 0, L4DM_RW, &content_addr);

    if (result) {
        LOG_Error("error attaching dataspace, error: %d", result);
        if (result == -L4_EUSED) {
            LOG_Error("L4_EUSED");
        }
        return NULL;
    }

    LOGd(_DEBUG,"attached ds at dezimal address %p", content_addr);
    list = parse_config_ds((char *)content_addr);

    if ((result = l4rm_detach(content_addr))) {
        LOG_Error("Error detaching from ds: %s\n", l4env_errstr(result));
    }

    return list;
}

static int dataprovider_load_content (char *fname, l4dm_dataspace_t *ds) {
    l4_int32_t result;
    l4_size_t size;
	CORBA_Environment _dice_corba_env = dice_default_environment;

    LOGd(_DEBUG,"try to open %s",fname);

    result = l4fprov_file_open_call(_dice_corba_obj, fname, &L4DM_DEFAULT_DSM,
                                     0,ds, &size, &_dice_corba_env);

    return result ? -L4_ENOENT : size;
}

static void dataprovider_wait_for_fprov(void) {
    while (names_waitfor_name("TFTP",_dice_corba_obj,4000)==0) {
            LOGd(_DEBUG,"fprov not found yet.");
    }

    LOGd(_DEBUG,"fprov found.");
}

static int dataprovider_online_check_fprov(void) {
    int ret;
    l4_threadid_t local_id;

    ret = names_query_name("TFTP",&local_id);

    if (ret!=0) id = local_id;

    return ret;
}

/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct dataprovider_services services = {
    dataprovider_load_config,
    dataprovider_load_content,
    dataprovider_wait_for_fprov,
    dataprovider_online_check_fprov,
};

int init_dataprovider(struct presenter_services *p) {
    arraylist = p->get_module(ARRAYLIST_MODULE);

    p->register_module(DATAPROVIDER_MODULE,&services);

    return 1;
}

