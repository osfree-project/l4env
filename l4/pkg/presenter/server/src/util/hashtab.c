#define HASHTAB struct hashtab_struct

#include "presenter_conf.h"
#include "memory.h"
#include "module_names.h"
#include "hashtab.h"
#include "keygenerator.h"

extern int _DEBUG;

struct memory_services *mem;

struct hashtab_entry_struct;
struct hashtab_entry_struct {
	int ident;
	void *value;
	struct hashtab_entry_struct *next;
};

struct hashtab_struct {
	u32 tab_size;					/* hash table size */
	struct hashtab_entry_struct **tab;	/* hash table itself */
};

int init_hashtable(struct presenter_services *p);
void hashtab_print_info(HASHTAB *h);

/**********************************/
/*** FUNCTIONS FOR INTERNAL USE ***/
/**********************************/

/*** CALCULATE HASH VALUE FOR A GIVEN STRING BY ADDING ITS CHARACTERS ***/
static int hash_value(int ident) {
	return ident;
}


/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/


/*** CREATE A NEW HASH TABLE OF THE SPECIFIED SIZE ***/
static HASHTAB *hashtab_create(u32 tab_size) {
	struct hashtab_struct *new_hashtab;
	u32 i;
	
	new_hashtab = (struct hashtab_struct *)mem->alloc(
		sizeof(struct hashtab_struct) + tab_size*sizeof(void *));
	if (!new_hashtab) {
		LOG("HashTable(create): out of memory!\n");
		return NULL;
	}
	
	new_hashtab->tab_size = tab_size;
	new_hashtab->tab = (struct hashtab_entry_struct **)((long)new_hashtab + sizeof(struct hashtab_struct));
	for (i=0;i<tab_size;i++) {
		new_hashtab->tab[i]=NULL;
	}
	return new_hashtab;	
}


/*** DESTROY A HASH TABLE ***/
static void hashtab_destroy(HASHTAB *h) {
	u32 i;
	struct hashtab_entry_struct *curr;
	struct hashtab_entry_struct *next;
	
	if (!h) return;
	
	/* go through all elements of the hash table */
	for (i=0;i<h->tab_size;i++) {
	
		curr=h->tab[i];
		/* if there exists a list at the current hashtab pos - destroy it */
		if (curr) {
			while (curr->next) {
				next=curr->next;
				mem->free(curr);
				curr=next;
			}
			mem->free(curr);
		}
	}
}


/*** ADD NEW HASH TABLE ENTRY ***/
static void hashtab_add_elem(HASHTAB *h, int ident, void *value) {
	int hashval;
	struct hashtab_entry_struct *ne;
	
	if (!h) return;
	hashval = hash_value(ident);

	ne = (struct hashtab_entry_struct *)mem->alloc(sizeof(struct hashtab_entry_struct));
	ne->ident=ident;
	ne->value=value;
	ne->next=h->tab[hashval];
	h->tab[hashval]=ne;	
}


/*** REQUESTS AN ELEMENT OF A HASH TABLE ***/
static void *hashtab_get_elem(HASHTAB *h,int ident) {
	int hashval;
	struct hashtab_entry_struct *ce;

	if (!h) return NULL;
	hashval = hash_value(ident);
	ce=h->tab[hashval];
	if (!ce) return NULL;
	return ce->value;
}


/*** REMOVES AN ELEMENT FROM A HASH TABLE ***/
static void hashtab_remove_elem(HASHTAB *h,int ident) {
	/* to be done */
}


/*** PRINT INFORMATION ABOUT A HASH TABLE (ONLY FOR DEBUGGING ISSUES) ***/
void hashtab_print_info(HASHTAB *h) {
	u32 i;
	struct hashtab_entry_struct *e;
	if (!h) {
		LOG(" hashtab is zero!\n");
		return;
	}
	LOG(" tab_size=%lu\n",h->tab_size);
	for (i=0;i<h->tab_size;i++) {
		LOG(" hash #%lu: ",i);
		e=h->tab[i];
		if (!e) LOG("empty");
		else {
			while (e) {
				LOG("%d, ",e->ident);
				e=e->next;
			}
		}
		LOG("\n");
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

int init_hashtable(struct presenter_services *p) {

	mem=p->get_module(MEMORY_MODULE);	
	p->register_module(HASHTAB_MODULE,&services);
	return 1;
}
