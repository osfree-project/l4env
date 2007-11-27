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
#include "timer.h"
#include "module_names.h"
#include "dataprovider.h"

#include <fcntl.h>
#include <unistd.h>

#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/sys/consts.h>
#include <l4/names/libnames.h>

#include <presenter_fprov-client.h>

#include <dice/dice.h>

#define _DEBUG 0
#define LINUX_O_RDONLY 0

static struct arraylist_services *arraylist;
static struct timer_services *timer;

int init_dataprovider(struct presenter_services *p);
static void dataprovider_wait_for_fprov(void);
static int dataprovider_online_check_fprov(void);

l4_threadid_t id = L4_INVALID_ID;
static CORBA_Object _dice_corba_obj = &id;
CORBA_Environment _dice_corba_env; // = dice_default_environment;

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

        /**
         * Look if we find a line in the form "time [minutes]".
         */
        if (strlen (pathname) > 5 &&
           (pathname[0] == 't' &&
            pathname[1] == 'i' &&
            pathname[2] == 'm' &&
            pathname[3] == 'e' &&
            pathname[4] == ' '))
          timer->timer_setabstime ((double)atoi (&pathname[5]));
        else
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
    l4_size_t bytes_read, ds_size;
    void *content_addr;
    l4_int32_t result, fd, count;
    l4dm_dataspace_t ds;
    ARRAYLIST *list;
	_dice_corba_env = (CORBA_Environment)dice_default_environment;

    if (!dataprovider_online_check_fprov()) dataprovider_wait_for_fprov();

    fd = presenter_fprov_open_call(_dice_corba_obj,fname,LINUX_O_RDONLY,&_dice_corba_env);

    if (fd <= 0) return NULL;

    /* get file size */
    count = presenter_fprov_lseek_call(_dice_corba_obj,fd,0L,SEEK_END,&_dice_corba_env);

    if (count <= 0) {
        presenter_fprov_close_call(_dice_corba_obj,fd,&_dice_corba_env);
        return NULL;
    }

    /* set pointer back to start of file */
    presenter_fprov_lseek_call(_dice_corba_obj,fd,0L,SEEK_SET,&_dice_corba_env);

    bytes_read = presenter_fprov_read_call(_dice_corba_obj,fd,
                                           &L4DM_DEFAULT_DSM,
                                           count,&ds,&_dice_corba_env);

    if (bytes_read <= 0) {
        presenter_fprov_close_call(_dice_corba_obj,fd,&_dice_corba_env);
        return NULL;
    }

    l4dm_mem_size(&ds,&ds_size);
    LOGd(_DEBUG,"dataspace size: %d",ds_size);

    result = l4rm_attach(&ds, ds_size, 0, L4DM_RW, &content_addr);

        if (result) {
            LOG_Error("error attaching dataspace, error: %d", result);
            if (result == -L4_EUSED) {
                LOG_Error("L4_EUSED");
            }
            presenter_fprov_close_call(_dice_corba_obj,fd,&_dice_corba_env);
            return NULL;
        }

    LOGd(_DEBUG,"attached ds at dezimal address %p", content_addr);
    list = parse_config_ds((char *)content_addr);

    presenter_fprov_close_call(_dice_corba_obj,fd,&_dice_corba_env);

    if ((result = l4rm_detach(content_addr))) {
        LOG_Error("Error detaching from ds: %s\n", l4env_errstr(result));
    }

    return list;
}

static int dataprovider_load_content (char *fname, l4dm_dataspace_t *ds) {
    l4_size_t bytes_read;
    l4_int32_t fd,count;
	_dice_corba_env = (CORBA_Environment)dice_default_environment;

    LOGd(_DEBUG,"try to open %s",fname);

    if (!dataprovider_online_check_fprov()) dataprovider_wait_for_fprov();

    fd = presenter_fprov_open_call(_dice_corba_obj,fname,
                                   LINUX_O_RDONLY,&_dice_corba_env);

    if (fd < 0)
        return -L4_ENOENT;

    /* get file size */
    count = presenter_fprov_lseek_call(_dice_corba_obj,fd,
                                       0L,SEEK_END,&_dice_corba_env);

    if (count <= 0) {
        presenter_fprov_close_call(_dice_corba_obj,fd,&_dice_corba_env);
        return -L4_ENOENT;
    }

    /* set pointer back to start of file */
    presenter_fprov_lseek_call(_dice_corba_obj,fd,0L,SEEK_SET,&_dice_corba_env);

    bytes_read = presenter_fprov_read_call(_dice_corba_obj,fd, &L4DM_DEFAULT_DSM,
                                           count,ds,&_dice_corba_env);

    presenter_fprov_close_call(_dice_corba_obj,fd,&_dice_corba_env);

    return bytes_read;

}

static void dataprovider_wait_for_fprov(void) {
    while (names_waitfor_name("PRESENTER_FPROV",_dice_corba_obj,40000)==0) {
        LOGd(_DEBUG,"presenter_fprov not found yet.");
    }

    LOGd(_DEBUG,"presenter_fprov found.");
}

static int dataprovider_online_check_fprov(void) {
    int ret;
    l4_threadid_t local_id;

    ret = names_query_name("PRESENTER_FPROV",&local_id);

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
    timer = p->get_module(TIMER_MODULE);

    p->register_module(DATAPROVIDER_MODULE,&services);

    return 1;
}

