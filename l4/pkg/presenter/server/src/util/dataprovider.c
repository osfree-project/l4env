/*
 * \brief   dataprovider class module
 * \date    2003-04-11
 * \author  Jens Syckor <js712688@inf.tu-dresden.de>
 *
 * This file implements a dataprovider which loads all
 * data from unsecure l4linux side.
 */

#include <l4/dm_phys/dm_phys.h>

#include "presenter_conf.h"
#include "arraylist.h"
#include "memory.h"
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

static struct memory_services *mem;

l4_threadid_t id = L4_INVALID_ID;
static CORBA_Object _dice_corba_obj = &id;
CORBA_Environment _dice_corba_env = dice_default_environment;

static struct arraylist_services *arraylist;

int init_dataprovider(struct presenter_services *p);

ARRAYLIST * parse_config_ds(char *content);

/**********************************/
/*** FUNCTIONS FOR INTERNAL USE ***/
/**********************************/

ARRAYLIST * parse_config_ds(char *content) {
	ARRAYLIST *al = arraylist->create();
	char *pathname;
	int i,j,k;
	i=0;
	k=0;

	while (content[i] != '\0') {
		while (content[i] != '\n') i++;
		content[i] = '\0';
		pathname = (char *) mem->alloc(i-k);
		
		for (j=0;j<=i-k;j++) pathname[j]=content[k+j];
		
		LOGd(_DEBUG,"parsed pathname == %s",pathname);

		arraylist->add_elem(al,pathname);
		k=i+1;
		i++;
	}

	return al;
}

/************************/
/**  SPECIFIC METHODS  **/
/************************/

static ARRAYLIST * dataprovider_load_config(char *fname) {
	
	l4_size_t ds_size;
	l4_addr_t content_addr;
	l4_int32_t result,count,error;
	l4_threadid_t dm;
	l4dm_dataspace_t *ds;
	count = 128;

	ds = (l4dm_dataspace_t *) mem->alloc(sizeof(l4dm_dataspace_t));

	while (names_waitfor_name("TFTP",_dice_corba_obj,40000)==0) {
                LOG("fprov not found yet...");
        }

	dm = l4dm_memphys_find_dmphys();

	result = l4fprov_file_open_call(_dice_corba_obj, fname, &dm, 00,ds, &count, &_dice_corba_env);

	if (result==-L4_ENOTFOUND) {
		LOG("file not found exception, fname: %s",fname);
		return NULL;
	}

	l4dm_mem_size(ds,&ds_size);
	LOGd(_DEBUG,"dataspace size: %d",ds_size);

	error=l4rm_attach(ds,ds_size,0,L4DM_RW,(void *)&content_addr);

        if (error != 0) {
                LOG("error attaching dataspace...");
                LOG("error: %d",error);
                if (error==-L4_EUSED) LOG("L4_EUSED");
                return NULL;
        }
        else {
                LOGd(_DEBUG,"attached ds at dezimal address %d",content_addr);
		return parse_config_ds((char *)content_addr);
        }	
		
}

static void dataprovider_load_content (char *fname, l4dm_dataspace_t *ds) {

        l4_int32_t result,count;
        l4_threadid_t dm;
        count = 128;

	LOGd(_DEBUG,"try to open %s",fname);

        while (names_waitfor_name("TFTP",_dice_corba_obj,40000)==0) {
                LOG("fprov not found yet...");
        }

        dm = l4dm_memphys_find_dmphys();

        result = l4fprov_file_open_call(_dice_corba_obj, fname, &dm, 00,ds, &count, &_dice_corba_env);

}

/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct dataprovider_services services = {
	dataprovider_load_config,
	dataprovider_load_content,
};

int init_dataprovider(struct presenter_services *p) {
	arraylist = p->get_module(ARRAYLIST_MODULE);
	mem	  = p->get_module(MEMORY_MODULE);

	p->register_module(DATAPROVIDER_MODULE,&services);
	
	return 1;
}

