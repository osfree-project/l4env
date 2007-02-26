/**
 * \file   l4vfs/simple_file_server/server/arraylist.c
 * \brief  arraylist class module
 *
 * \date   2003-04-11
 * \author Jens Syckor <js712688@inf.tu-dresden.de>
 *
 * This file implements an arraylist.
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#define ARRAYLIST struct arraylist_struct

#include "arraylist.h"
#include <stdlib.h>
#include <l4/log/l4log.h>

#define MAX_ENTRIES 128		/* maximum amount of entries */

#define _DEBUG 0 

struct arraylist_struct {
	/*  entry table */
	void *entry_tab[MAX_ENTRIES];

	/* current field position */
	int current_pos;
	
	/* size of field */
	int last_index;

};

/************++++*******/
/*** SERVICE METHODS ***/
/***************++++****/

static void arraylist_add_elem (ARRAYLIST *al, void *value) {
	if (!al) return;

	al->last_index++;

	LOGd(_DEBUG,"add element at position %d",al->last_index);

	al->entry_tab[al->last_index]=value;
}

static void * arraylist_get_elem (ARRAYLIST *al, int index) {
	if (!al) return NULL;
        if (index > al->last_index) return NULL;
	al->current_pos=index;
	return  al->entry_tab[index];
}

static void arraylist_remove_elem (ARRAYLIST *al, int index) {
	/* to implement */
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

	/* if current_pos is at last element do not set it higher */
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
	/* to implement */
}

static int arraylist_get_current_index(ARRAYLIST *al) {
	if (!al) return 0;
	if (al->current_pos==-1) return 0;
	else return al->current_pos;
}

static void arraylist_add_elem_at_index(ARRAYLIST *al, void *value, int index) {
	if (!al) return;
	if (index > al->last_index) return;

	LOGd(_DEBUG,"add element at index %d",index);
	
	al->entry_tab[index] = value;
}

struct arraylist_services services = {
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
	arraylist_add_elem_at_index,
};
