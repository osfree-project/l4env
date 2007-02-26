/*!
 * \file   libc_backends_l4env/examples/malloc/main.c
 * \brief  Malloc example, probably you have to adapt l4libc_heapsize
 *
 * \date   08/18/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 * This program allocates memory in increasin chunks until no more memory
 * is available.
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <l4/util/bitops.h>
#include <l4/log/l4log.h>

l4_ssize_t l4libc_heapsize = 64*1024*1024;
char LOG_tag[9]="alloctst";

#define SIZE (10)
void test4(void);
void test4(void){
    int i;

    unsigned char**addr = alloca(SIZE*sizeof(void*));
    unsigned allocated=0;
    unsigned allocated_real=0;

    for(i=0; i<SIZE;i++){
	unsigned size = (1024*1024)<<i;
	unsigned size_real;
	size_real = size?1<<l4util_log2(size):0;
	if(size_real<size) size_real<<=1;

	addr[i]=malloc(size);
	if(addr[i]){
	    LOG("allocated %d KB, nr %d at %p", size>>10, i, addr[i]);
	    allocated+=size;
	    allocated_real+=size_real;
	    LOGl("allocated_real=%d", allocated_real);
#if 0
	{
	    extern l4buddy_root *malloc_buddy;
	    LOG("buddy_free: %d, alloc: %d",
		l4buddy_debug_free(malloc_buddy),
		l4buddy_debug_allocated(malloc_buddy));
	}
#endif
	}
    }
    LOG("allocated %d KB (%d KB) in sum",
	allocated/1024, allocated_real/1024);
    for(i=0; i<SIZE;i++){
	if(addr[i]){
	    LOG("freeing %d (%p)", i, addr[i]);
	    free(addr[i]);
	    addr[i]=0;
	}
    }
}

int main(int argc, const char**argv){
    test4();
    LOG("returning...");
    exit(0);
}
