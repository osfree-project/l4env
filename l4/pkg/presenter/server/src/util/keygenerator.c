/*
 * \brief   keygenerator class module
 * \date    2003-04-11
 * \author  Jens Syckor <js712688@inf.tu-dresden.de>
 *
 * This file implements a very simple keygenerator.
 * Each presenter object needs such a key.
 */

#define KEYGENERATOR struct keygenerator_struct

#include "presenter_conf.h"
#include "keygenerator.h"
#include "module_names.h"
#include "memory.h"

struct memory_services *mem;

struct keygenerator_struct {
	int key_intern;
};

#define _DEBUG 0

int init_keygenerator(struct presenter_services *p);

/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

static KEYGENERATOR *keygenerator_create (void) {
	struct keygenerator_struct *keygen;
	
	keygen = (struct keygenerator_struct *) mem->alloc(sizeof(struct keygenerator_struct));
	keygen->key_intern=0;
	
	return keygen;
}

static int keygenerator_get_key (KEYGENERATOR *k) {
	k->key_intern++;
	
	LOGd(_DEBUG,"key==%d",k->key_intern);
	
	return k->key_intern;
}

/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct keygenerator_services services = {
        keygenerator_create,
	keygenerator_get_key,
};


/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_keygenerator(struct presenter_services *p) {

        mem=p->get_module(MEMORY_MODULE);
        p->register_module(KEYGENERATOR_MODULE,&services);
        return 1;
}


