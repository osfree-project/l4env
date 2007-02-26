
#include "presenter_conf.h"

#define MAX_POOL_ENTRIES 100

#define _DEBUG 0

struct pool_entry {
	char	*name;			/* id of system module */
	void	*structure;		/* system module structure */
};

static struct pool_entry pool[MAX_POOL_ENTRIES];
static long pool_size=0;


/*** PROTOTYPES ***/
long pool_add(char *name,void *structure);
void pool_remove(char *name);
void *pool_get(char *name);


/*** UTILITY: CHECK IF TWO STRINGS ARE EQUAL ***/
static u16 streq(char *s1,char *s2) {
	int i;
	for (i=0;i<256;i++) {
		if (*(s1) != *(s2++)) return 0;
		if (*(s1++) == 0) return 1;
	}
	return 1;
}

/*** ADD NEW POOL ENTRY ***/
long pool_add(char *name,void *structure) {
	long i;
	if (pool_size>=100) return 0;
	else {
		for (i=0;pool[i].name!=NULL;i++) {};
		
		pool[i].name=name;
		pool[i].structure=structure;
		
		pool_size++;
		LOGd(_DEBUG,"Pool(add): %s\n",name);
		return 1;
	}
}


/*** REMOVE POOLENTRY FROM POOL ***/
void pool_remove(char *name) {
	long i;
	char *s;
	for (i=0;i<100;i++) {
		s=pool[i].name;
		if (s!=NULL) {
			if (streq(name,pool[i].name)) {
			 	pool[i].name=NULL;
			 	pool[i].structure=NULL;
			 	pool_size--;
			 	return;
			}
		}
	}
}


/*** GET STRUCTURE OF A SPECIFIED POOL ENTRY ***/
void *pool_get(char *name) {
	long i;
	char *s;
	for (i=0;i<MAX_POOL_ENTRIES;i++) {
		s=pool[i].name;
		if (s!=NULL) {
			if (streq(name,pool[i].name)) {
				LOGd(_DEBUG,"Pool(get): module matched: %s\n",name);
			 	return pool[i].structure;
			}
		}
	}
	LOG("Pool(get): module not found: %s\n",name);
//	l4_sleep(10000);
	return NULL;
}
