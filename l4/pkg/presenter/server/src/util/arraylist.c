/*
 * \brief   arraylist class module
 * \date    2003-04-11
 * \author  Jens Syckor <js712688@inf.tu-dresden.de>
 *
 * This file implements an arraylist.
 */

#define ARRAYLIST struct arraylist_struct

#include "arraylist.h"
#include "presenter_conf.h"
#include "module_names.h"

#include <errno.h>

#define _DEBUG 0

#define MAX_ENTRIES 256         /* maximum amount of entries */

struct arraylist_struct {
    /*  entry table */
    void *entry_tab[MAX_ENTRIES];

    /* current field position */
    int current_pos;

    /* size of field */
    int last_index;

};

int init_arraylist(struct presenter_services *p);

/************++++*******/
/*** SERVICE METHODS ***/
/***************++++****/

static int arraylist_add_elem (ARRAYLIST *al, void *value) {
    if (!al) return -EINVAL;

    if (al->last_index == MAX_ENTRIES)
    {
        LOG("No more memory to add element");
        return -ENOMEM;
    }

    al->last_index++;

    LOGd(_DEBUG,"add element at position %d",al->last_index);

    al->entry_tab[al->last_index]=value;

    return 0;
}

static void * arraylist_get_elem (ARRAYLIST *al, int index) {
    if (!al) return NULL;
    if (index > al->last_index) return NULL;
    al->current_pos=index;
    return  al->entry_tab[index];
}

static void arraylist_remove_elem (ARRAYLIST *al, int index) {
    int i;
    if (!al) return;
    if (index < 0) return;

    for (i=index;i<al->last_index;i++) {
        al->entry_tab[i] = al->entry_tab[i+1];
    }

    al->entry_tab[al->last_index] = NULL;

    al->last_index--;
}

static void * arraylist_get_first (ARRAYLIST *al) {
    if (!al) return NULL;
    if (al->last_index==-1) return NULL;
    al->current_pos=0;
    return al->entry_tab[al->current_pos];
}

static void * arraylist_get_last (ARRAYLIST *al) {
    if (!al) return NULL;
    if (al->last_index==-1) return NULL;

    al->current_pos=al->last_index;
    return al->entry_tab[al->current_pos];
}

static void * arraylist_get_next (ARRAYLIST *al) {
    if (!al) return NULL;

    /* if current_pos is at last element do not increment */
    if (al->current_pos==al->last_index) {
        LOGd(_DEBUG,"get element at position %d",al->current_pos);
        return al->entry_tab[al->current_pos];
    }

    al->current_pos++;

    LOGd(_DEBUG,"get element at position %d",al->current_pos);

    return  al->entry_tab[al->current_pos];
}

static void * arraylist_get_prev (ARRAYLIST *al) {
    if (!al) return NULL;

    if (al->current_pos==-1) {
        al->current_pos++;
        LOGd(_DEBUG,"get element at position %d",al->current_pos);
        return al->entry_tab[al->current_pos];
    }

    if (al->current_pos==0) {
        LOGd(_DEBUG,"get element at position %d",al->current_pos);
        return al->entry_tab[al->current_pos];
    }

    al->current_pos--;

    LOGd(_DEBUG,"get element at position %d",al->current_pos);

    return al->entry_tab[al->current_pos];
}

static int arraylist_is_empty (ARRAYLIST *al) {
    if (!al) return 0;
    if (al->last_index==-1) return 0;
    else return 1;
}

static int arraylist_size (ARRAYLIST *al) { 
    if (!al) return 0;

    LOGd(_DEBUG,"arraylist size: %d",al->last_index+1);

    if (al->last_index==-1) return 0;
    else return  al->last_index+1;
}

static void arraylist_set_iterator (ARRAYLIST *al) {
    if (!al) return;

    LOGd(_DEBUG,"set iterator of arraylist");

    al->current_pos=-1; 
}

static ARRAYLIST *arraylist_create(void) {
    struct arraylist_struct *new;

    new = (struct arraylist_struct *)malloc(
                sizeof(struct arraylist_struct) + MAX_ENTRIES*sizeof(void *));
    if (!new) {
            LOG("ArrayList(create): out of memory!\n");
            return NULL;
    }

    new->last_index=-1;
    new->current_pos=-1;

    return new;
}

static void arraylist_destroy(ARRAYLIST *al) {
    int i;

    if (al->last_index==-1) return;

    for (i=0;i<al->last_index;i++) {
        al->entry_tab[i] = NULL;
    }

    al->last_index=-1;

}

static int arraylist_get_current_index(ARRAYLIST *al) {
    if (!al) return 0;
    if (al->current_pos==-1) return 0;
    else return al->current_pos;
}

static struct arraylist_services services = {
    arraylist_create,
    arraylist_destroy,
    arraylist_add_elem,
    arraylist_get_elem,
    arraylist_remove_elem,
    arraylist_get_first,
    arraylist_get_last,
    arraylist_get_next,
    arraylist_get_prev,
    arraylist_is_empty,
    arraylist_size,
    arraylist_set_iterator,
    arraylist_get_current_index,
};


/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_arraylist(struct presenter_services *p) {
        p->register_module(ARRAYLIST_MODULE,&services);
        return 1;
}

