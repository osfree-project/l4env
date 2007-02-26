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

#define KEYGENERATOR_MAX_KEYS 256

struct keygenerator_struct {

    int released_keys[KEYGENERATOR_MAX_KEYS];
    int key_intern;
    int curr_rel_keys;
    int key_amount;
};

#define _DEBUG 0

int init_keygenerator(struct presenter_services *p);

/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

static KEYGENERATOR *keygenerator_create (int number_of_keys)
{
    struct keygenerator_struct *keygen;

    keygen = malloc(sizeof(struct keygenerator_struct));
    if (!keygen)
      return NULL;

    keygen->key_intern=0;
    keygen->key_amount=number_of_keys;
    keygen->curr_rel_keys=0;

    return keygen;
}

static int keygenerator_get_key (KEYGENERATOR *k) {
    int key;

    if (k->curr_rel_keys > 0) {

        key = k->released_keys[k->curr_rel_keys-1];

        LOGd(_DEBUG,"using released key %d",key);

        k->curr_rel_keys--;

        return key;
    }

    k->key_intern++;

    LOGd(_DEBUG,"key==%d",k->key_intern);

    return k->key_intern;
}

static void keygenerator_release_key(KEYGENERATOR *k, int key) {

    k->released_keys[k->curr_rel_keys] = key;

    LOGd(_DEBUG,"released key: %d",k->released_keys[k->curr_rel_keys]);

    k->curr_rel_keys++;

}

/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct keygenerator_services services = {
    keygenerator_create,
    keygenerator_get_key,
    keygenerator_release_key,
};


/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_keygenerator(struct presenter_services *p) {

        p->register_module(KEYGENERATOR_MODULE,&services);
        return 1;
}


