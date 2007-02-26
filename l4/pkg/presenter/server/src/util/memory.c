
#include <l4/oskit_support_l4env/support.h>
#include <l4/sys/kdebug.h>

#define size_t long		/* just to avoid warnings */

l4_ssize_t oskit_support_heapsize = 6*1024*1024;
const l4_ssize_t l4libc_heapsize = 128*1024; /**< init mallo heap */
const int l4thread_max_threads = 4;          /**< limit number of threads */
const l4_size_t l4thread_stack_size = 16384; /**< limit stack size */

#include "presenter_conf.h"
#include "memory.h"
#include "module_names.h"

extern void *malloc(size_t size); 
extern void free(void *ptr);
extern void *memmove(void *dst,void *src,size_t size);

int init_memory(struct presenter_services *p);

static struct memory_services mem = { 
	malloc,
	free,
	memmove,
};

int init_memory(struct presenter_services *p) {
	p->register_module(MEMORY_MODULE,&mem);
	return 1;
}

