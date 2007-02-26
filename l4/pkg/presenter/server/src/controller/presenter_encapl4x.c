struct private_presenter_encapl4x;
#define PRESENTER_ENCAPL4X struct private_presenter_encapl4x

#include <stdio.h>
#include <stdlib.h>

#include <l4/dm_phys/dm_phys.h>

#include "util/presenter_conf.h"
#include "util/arraylist.h"
#include "util/module_names.h"
#include "controller/presenter_encapl4x.h"

#include <presenter_fprov-client.h>
#include <presenter_exec-client.h>
#include <l4/presenter/presenter_exec_lib.h>

#include <fcntl.h>
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/sys/consts.h>
#include <l4/names/libnames.h>

#include <dice/dice.h>

#define _DEBUG 1
#define TMP_PATH "/tmp/"
#define SINGLE_PS "/tmp/presenter_sld.ps"
#define PNG_FILE "/tmp/presenter_sld.png"

l4_threadid_t id_fprov = L4_INVALID_ID;
l4_threadid_t id_exec = L4_INVALID_ID;
static CORBA_Object _dice_corba_obj_fprov = &id_fprov;
static CORBA_Object _dice_corba_obj_exec = &id_exec;
CORBA_Environment _dice_corba_env_fprov = dice_default_environment;
CORBA_Environment _dice_corba_env_exec = dice_default_environment;

PRESENTER_ENCAPL4X {
        /* all methods of encapl4x */
        struct presenter_encapl4x_methods *enl4xm;

        char *fname;
};

int init_presenter_encapl4x(struct presenter_services *);

static char *calc_complete_path(char *fname) {
	int length_fname, length_path;
	char *complete_path;

	length_fname = strlen(fname);

	length_path = strlen(TMP_PATH);

	complete_path = malloc(length_fname+length_path+1);

	complete_path = strncpy(complete_path,TMP_PATH,length_path);
	strcpy(complete_path+length_path,fname);

	return complete_path;	

}

/* find the amount of pages in binary, here ps file */
static int calc_amount_of_pages(void) {
	int fd,error,pages;
	l4_size_t bytes_read, ds_size;
	l4_addr_t content_addr;
	l4_threadid_t dm;
        l4dm_dataspace_t *ds;
	char *content, *start_number, *end_number;

	ds = (l4dm_dataspace_t *) malloc(sizeof(l4dm_dataspace_t));

        dm = l4dm_memphys_find_dmphys();

	/* open error log in encapl4x */	
	fd = presenter_fprov_open_call(_dice_corba_obj_fprov,ERROR_LOG,O_RDONLY,&_dice_corba_env_fprov);

	if (fd < 0) return -1;

	bytes_read = presenter_fprov_read_call(_dice_corba_obj_fprov,fd,&dm,128,ds,&_dice_corba_env_fprov);

	if (bytes_read <= 0) return -1;

	presenter_fprov_close_call(_dice_corba_obj_fprov,fd,&_dice_corba_env_fprov);

        l4dm_mem_size(ds,&ds_size);

        error=l4rm_attach(ds,ds_size,0,L4DM_RW,(void *)&content_addr);

        if (error != 0) {
                LOG("error attaching dataspace...");
                LOG("error: %d",error);
                if (error==-L4_EUSED) LOG("L4_EUSED");
                return -1;
        }
        else {
                LOGd(_DEBUG,"attached ds at dezimal address %d",content_addr);
        }

	content = (char *) content_addr;

	/* parse content of ds */
	start_number = strstr(content,"Wrote");	
	start_number = strchr(start_number,32);
	start_number++;	/* ok, now we have start adress of page numbers */

	/* find end of page numbers */
	end_number = strchr(start_number,32);
	end_number[0] = '\0';

	start_number = strdup(start_number);

	pages = atoi(start_number);

	return pages;
}

static int put_into_encapl4x(PRESENTER_ENCAPL4X *encapl4x,char *fname, l4dm_dataspace_t *ds) {
	int fd, ds_size, flags, error,pages;
	char *complete_path, *argv;
	argv="";
	fd = - 1;

	complete_path = calc_complete_path(fname);

	LOGd(_DEBUG,"complete_path: %s",complete_path);

	/* calculate size of dataspace */
        l4dm_mem_size((l4dm_dataspace_t *)ds,&ds_size);

        LOGd(_DEBUG,"ds_size: %d",ds_size);

	encapl4x->fname = complete_path;
 
        sprintf(argv,"%s %s %s","psnup",encapl4x->fname,"//dev//zero");

	flags = O_CREAT | O_RDWR;

	/* open file */
	fd = presenter_fprov_open_call(_dice_corba_obj_fprov,complete_path,flags,&_dice_corba_env_fprov);

        if (fd <= 0) {
		LOG("error opening file, got fd %d",fd);	
		return -1;
	}

	/* transfer ownership of dataspace to encapl4x file provider */
        if ((error = l4dm_transfer(ds, *_dice_corba_obj_fprov))) {
                LOG("Error transfering dataspace ownership");
                return -1;
        }

	/* write file into encapsuled L4Linux */
	presenter_fprov_write_call(_dice_corba_obj_fprov,fd,ds,ds_size,&_dice_corba_env_fprov);

	/* close file and free fd */
        presenter_fprov_close_call(_dice_corba_obj_fprov,fd,&_dice_corba_env_fprov);

	/* exec psnup to calculate amount of page in ps file */
 	presenter_exec_execvp_call(_dice_corba_obj_exec,"psnup",argv,&_dice_corba_env_exec);

	/* calculate amount of pages from psnup output in encapl4x */
	pages =  calc_amount_of_pages();

	return pages;
}

static int get_slide_from_encapl4x(PRESENTER_ENCAPL4X *encapl4x,int index, l4dm_dataspace_t *ds) {
	int fd;
	l4_threadid_t dm;
        l4_size_t bytes_read;

	char *argv, *convert_argv;
	argv="";
	convert_argv="";

	sprintf(argv,"%s -p%d %s %s","psselect",index,encapl4x->fname,SINGLE_PS);

	/* exec psselect to strip single ps slide */
        presenter_exec_execvp_call(_dice_corba_obj_exec,"psselect",argv,&_dice_corba_env_exec);

	sprintf(convert_argv,"%s %s %s","convert",SINGLE_PS,PNG_FILE);

	/* exec convert to convert single ps slide into png file */
        presenter_exec_execvp_call(_dice_corba_obj_exec,"convert",convert_argv,&_dice_corba_env_exec);

	 /* open file */
        fd = presenter_fprov_open_call(_dice_corba_obj_fprov,PNG_FILE,0,&_dice_corba_env_fprov);

        if (fd <= 0) {
                LOG("error opening file, got fd %d",fd);
                return -1;
        }

	dm = l4dm_memphys_find_dmphys();

	bytes_read = presenter_fprov_read_call(_dice_corba_obj_fprov,fd,&dm,128,ds,&_dice_corba_env_fprov);

        if (bytes_read <= 0) return -1;

        presenter_fprov_close_call(_dice_corba_obj_fprov,fd,&_dice_corba_env_fprov);

	return bytes_read;
}

static void encapl4x_wait_for_fprov(void) {

        while (names_waitfor_name("ENCAPL4X_FPROV",_dice_corba_obj_fprov,40000)==0) {
                LOG("presenter_fprov in encapl4x not found yet.");
        }

        LOG("encapl4x_fprov found.");
}

static void encapl4x_wait_for_exec(void) {
	while (names_waitfor_name(PRESENTER_EXEC,_dice_corba_obj_exec,40000)==0) {
                LOG("presenter_exec in encapl4x not found yet.");
        }

        LOG("presenter_exec found.");
}

/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct presenter_encapl4x_methods encapl4x_meth = {
	put_into_encapl4x,
	get_slide_from_encapl4x,
};

static PRESENTER_ENCAPL4X *create(void) {
	PRESENTER_ENCAPL4X *new = malloc(sizeof(PRESENTER_ENCAPL4X));

	new->enl4xm = &encapl4x_meth;

	encapl4x_wait_for_fprov();

	encapl4x_wait_for_exec();
	
	return new;
}

static struct presenter_encapl4x_services services = {
        create,
};

int init_presenter_encapl4x(struct presenter_services *p) {

	p->register_module(ENCAPL4X_MODULE,&services);

	return 1;
}
