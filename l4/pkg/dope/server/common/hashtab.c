/*
 * \brief   DOpE hash table module
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This component provides a generic interface
 * to handle hash  tables. New elements can be
 * added while specifying  an identifier. This
 * identifier is  also used to retrieve a hash
 * table element.
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#define HASHTAB struct hashtab

#include "dopestd.h"
#include "hashtab.h"

struct hashtab_entry;
struct hashtab_entry {
	char *ident;
	void *value;
	struct hashtab_entry *next;
};

struct hashtab {
	u32 tab_size;                   /* hash table size */
	u32 max_hash_length;                /* number of chars for building hashes */
	struct hashtab_entry **tab; /* hash table itself */
};

int init_hashtable(struct dope_services *d);
void hashtab_print_info(HASHTAB *h);

/**********************************/
/*** FUNCTIONS FOR INTERNAL USE ***/
/**********************************/

/*** CALCULATE HASH VALUE FOR A GIVEN STRING BY ADDING ITS CHARACTERS ***/
static u32 hash_value(char *ident,u32 max_cnt) {
	u32 result=0;

	if (!ident) return 0;
	while ((*ident!=0) && (max_cnt>0)) {
		result += *ident;
		ident++;
		max_cnt--;
	}
	return result;
}


/*** COMPARE TWO STRINGS ***/
static s16 cmp_str(char *s1,char *s2) {

	while (*s1 && *s2) {
		if (*s1 > *s2) return 1;
		if (*s1 < *s2) return -1;
		s1++;
		s2++;
	}
	if (!(*s1) && !(*s2)) return 0;
	if (s1) return 1;
	return -1;
}

/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/


/*** CREATE A NEW HASH TABLE OF THE SPECIFIED SIZE ***/
static HASHTAB *hashtab_create(u32 tab_size,u32 max_hash_length) {
	struct hashtab *new_hashtab;
	u32 i;

	new_hashtab = (struct hashtab *)malloc(
		sizeof(struct hashtab) + tab_size*sizeof(void *));
	if (!new_hashtab) {
		INFO(printf("HashTable(create): out of memory!\n");)
		return NULL;
	}

	new_hashtab->tab_size = tab_size;
	new_hashtab->max_hash_length = max_hash_length;
	new_hashtab->tab = (struct hashtab_entry **)((long)new_hashtab + sizeof(struct hashtab));
	for (i=0;i<tab_size;i++) {
		new_hashtab->tab[i]=NULL;
	}
	return new_hashtab;
}


/*** DESTROY A HASH TABLE ***/
static void hashtab_destroy(HASHTAB *h) {
	u32 i;
	struct hashtab_entry *curr;
	struct hashtab_entry *next;

	if (!h) return;

	/* go through all elements of the hash table */
	for (i=0;i<h->tab_size;i++) {

		curr=h->tab[i];
		/* if there exists a list at the current hashtab pos - destroy it */
		if (curr) {
			while (curr->next) {
				next=curr->next;
				free(curr);
				curr=next;
			}
			free(curr);
		}
	}
}


/*** ADD NEW HASH TABLE ENTRY ***/
static void hashtab_add_elem(HASHTAB *h, char *ident, void *value) {
	u32 hashval;
	struct hashtab_entry *ne;

	if (!h) return;
	hashval = hash_value(ident,h->max_hash_length)%(h->tab_size);
	ne = (struct hashtab_entry *)malloc(sizeof(struct hashtab_entry));
	ne->ident=ident;
	ne->value=value;
	ne->next=h->tab[hashval];
	h->tab[hashval]=ne;
}


/*** REQUESTS AN ELEMENT OF A HASH TABLE ***/
static void *hashtab_get_elem(HASHTAB *h,char *ident) {
	u32 hashval;
	struct hashtab_entry *ce;

	if (!h) return NULL;
	hashval = hash_value(ident,h->max_hash_length)%(h->tab_size);
	ce=h->tab[hashval];
	if (!ce) return NULL;
	while (cmp_str(ident,ce->ident)) {
		ce=ce->next;
		if (!ce) return NULL;
	}
	return ce->value;
}


/*** REMOVES AN ELEMENT FROM A HASH TABLE ***/
static void hashtab_remove_elem(HASHTAB *h,char *ident) {
	/* to be done */
}


/*** PRINT INFORMATION ABOUT A HASH TABLE (ONLY FOR DEBUGGING ISSUES) ***/
void hashtab_print_info(HASHTAB *h) {
	u32 i;
	struct hashtab_entry *e;
	if (!h) {
		printf(" hashtab is zero!\n");
		return;
	}
	printf(" tab_size=%lu\n",h->tab_size);
	printf(" max_hash_length=%lu\n",h->max_hash_length);
	for (i=0;i<h->tab_size;i++) {
		printf(" hash #%lu: ",i);
		e=h->tab[i];
		if (!e) printf("empty");
		else {
			while (e) {
				printf("%s, ",e->ident);
				e=e->next;
			}
		}
		printf("\n");
	}

}


/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct hashtab_services services = {
	hashtab_create,
	hashtab_destroy,
	hashtab_add_elem,
	hashtab_get_elem,
	hashtab_remove_elem
};


/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_hashtable(struct dope_services *d) {
	d->register_module("HashTable 1.0",&services);
	return 1;
}
