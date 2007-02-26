/*
 * \brief	DOpE cache handling module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This module provides the needed functionality
 * to handle caches.  Although the caches can be 
 * used for every kind of  data the primary  use
 * of this module is the management of image and
 * font caches.
 */


#define CACHE struct cache_struct

#include "dope-config.h"
#include "memory.h"
#include "cache.h"

struct memory_services *mem;

struct cache_elem_struct {
	void	*data;					/* cached data block */
	s32	 	size;					/* size of data block */
	s32	 	ident;					/* data block identifier */
	void	 (*destroy)(void *);	/* data block destroy function */
};

struct cache_struct {
	s32		max_entries;	/* size of ring buffer */
	s32		max_size;		/* max amount of cached data */
	s32		curr_size;		/* current amount of cached data */
	s32		idxtokill;		/* 'oldest' ring buffer index */
	s32 	idxtoadd;		/* ring buffer index for new element */
	struct cache_elem_struct	*elem;			/* pointer to ring buffer */
};

int init_cache(struct dope_services *d);



/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

/*** CREATE NEW CACHE ***/
static CACHE *create(s32 max_entries,s32 max_size) {
	struct cache_struct *c;
	struct cache_elem_struct *e;
	s32 i;
	
	/* get memory for cache struct and the ring buffer */
	c= (struct cache_struct *)mem->alloc(sizeof(struct cache_struct) + max_entries*sizeof(struct cache_elem_struct));
	if (!c) {
		DOPEDEBUG(printf("Cache(create): out of memory\n");)
		return NULL;
	}

	/* set values in cache struct */
	c->max_entries=max_entries;
	c->max_size=max_size;
	c->curr_size=0;
	c->idxtokill=0;
	c->idxtoadd=0;
	c->elem = (struct cache_elem_struct *)((long)c + sizeof(struct cache_struct));
	
	/* clear all ring buffer elements */
	e=c->elem;
	for (i=0;i<max_entries;i++) {
		e->data=NULL;
		e++;
	}

	return c;
}


/*** REMOVE ELEMENT FROM CACHE ***/
static void remove_elem(CACHE *cache,s32 index) {
	struct cache_elem_struct *e;
	if (!cache) {
		DOPEDEBUG(printf("Cache(remove_elem): cache == NULL\n");)
		return;
	}
	if (index >= cache->max_entries) {
		DOPEDEBUG(printf("Cache(remove_elem): index out of range\n");)
		return;
	}
	e=cache->elem + index;
	DOPEDEBUG(printf("Cache(remove_element): removing element %lu\n",index);)
	if (e->data) {
		if (e->destroy) e->destroy(e->data);
		else mem->free(e->data);
		cache->curr_size -= e->size;
		e->data=NULL;
	}
}


/*** DESTROY CACHE ***/
static void destroy(CACHE *cache) {
	s32 i;
	if (!cache) return;
	
	/* free cache entries */
	for (i=0;i<cache->max_entries;i++) remove_elem(cache,i);
	
	/* free cache struct itself */
	mem->free(cache);
}


static void reduce_cachesize(CACHE *cache,long needed_size) {
	if (!cache) return;
	
	DOPEDEBUG(printf("Cache(reduce_cachesize): old size is %lu\n",cache->curr_size);)
	while (cache->curr_size > needed_size) {
		remove_elem(cache,cache->idxtokill);
		cache->idxtokill = (cache->idxtokill + 1)%(cache->max_entries);	
	}
	DOPEDEBUG(printf("Cache(reduce_cachesize): new size is %lu\n",cache->curr_size);)
}


/*** INSERT ELEMENT INTO CACHE ***/
/* parameters:                                                               */
/*   elem     - data block to add to cache                                   */
/*   elemsize - size of data block (needed to determine when element must be */
/*              removed)                                                     */
/*   ident    - identifier for this element (needed to determine if the      */
/*              element did not got overwritten by another                   */
/*   destroy  - function that is called when an element needs to be          */
/*              destroyed                                                    */
/* returns:                                                                  */
/*   index to cache element. The cached element can be accessed later using  */
/*   the function get_element with this index and the identifier as args.    */
/**/
static s32 add_elem(CACHE *cache,void *elem,s32 elemsize,s32 ident,void (*destroy)(void *)) {
	struct cache_elem_struct *e;
	s32 new_idx;
	
	if (!cache) return -1;
	
	/* check if cache grows bigger that its maximum size */
	if (cache->curr_size + elemsize > cache->max_size) {
		reduce_cachesize(cache,cache->max_size - elemsize);
	}
	
	new_idx = cache->idxtoadd;
	e=cache->elem + new_idx;
	DOPEDEBUG(printf("Cache(add_elem): add element at index %lu\n",new_idx);)
	
	/* gets another cache element overwritten? */
	if (e->data) {
		DOPEDEBUG(printf("Cache(add_elem): old element gets overwritten\n");)

		/* remove old element */
		remove_elem(cache,new_idx);
		cache->idxtokill = (cache->idxtokill + 1)%(cache->max_entries);
	}
	
	e->data=elem;
	e->size=elemsize;
	e->ident=ident;
	e->destroy=destroy;
	
	cache->curr_size += elemsize;
	cache->idxtoadd = (cache->idxtoadd + 1)%(cache->max_entries);

	return new_idx;
}


/*** GET CACHED DATA ***/
static void *get_elem(struct cache_struct *cache,s32 index,s32 ident) {
	
	if (!cache) {
		DOPEDEBUG(printf("Cache(get_elem): cache == NULL\n");)
		return NULL;
	}
	if (index >= cache->max_entries) {
		DOPEDEBUG(printf("Cache(get_elem): index %lu out of range\n",index);)
		return NULL;
	}
		
	if ((cache->elem + index)->ident == ident) {
		return (cache->elem + index)->data;
	}
	DOPEDEBUG(printf("Cache(get_elem): element %lu is not cached anymore\n",index);)
	return NULL;
}



/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct cache_services services = {
	create,
	destroy,
	add_elem,
	get_elem,
	remove_elem
};



/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_cache(struct dope_services *d) {

	mem=d->get_module("Memory 1.0");;
	d->register_module("Cache 1.0",&services);
	return 1;
}
