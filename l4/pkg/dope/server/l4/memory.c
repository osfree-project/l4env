/*
 * \brief	DOpE memory management module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This component provides an abstraction for allocating
 * and  freeing memory.  It should be used by all  other
 * components  of DOpE rather than  using malloc or free
 * of stdlib.
 */

#include <l4/oskit10_l4env/support.h>
#include <l4/sys/kdebug.h>

//#include <stdlib.h>

#define size_t long		/* just to avoid warnings */

#include "dope-config.h"
#include "module.h"
#include "memory.h"

extern void *malloc(size_t size);
extern void free(void *ptr);
extern void *memmove(void *dst,void *src,size_t size);

int init_memory(struct dope_services *d);

static struct memory_services mem = { 
	malloc,
	free,
	memmove,
};

int init_memory(struct dope_services *d) {
  if(OSKit_libc_support_init(6*1024*1024)) {
    enter_kdebug("memory");
    return 0;
  }

	d->register_module("Memory 1.0",&mem);
	return 1;
}

void *CORBA_alloc(long);

void *CORBA_alloc(long size) {
	return malloc(size);
}
