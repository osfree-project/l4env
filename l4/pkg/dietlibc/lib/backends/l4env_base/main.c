/*!
 * \file   dietlibc/lib/backends/l4env_base/main.c
 * \brief  
 *
 * \date   08/19/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <l4/log/l4log.h>
#include <l4/util/bitops.h>
#include "buddy.h"

static char memory_pool[1024*30] __attribute__((aligned(1024)));
static char memory_pool30[1024*30] __attribute__((aligned(1024)));

void test1(void);
void test1(void){
    l4buddy_root *root = l4buddy_create(memory_pool, sizeof(memory_pool));

    l4buddy_debug_dump(root);

    l4buddy_t *b1, *b2, *b3, *b4, *b5, *b6;

    b1 = l4buddy_alloc(root, 5*1024);
    l4buddy_debug_dump(root);
    b2 = l4buddy_alloc(root, 2*1024);
    l4buddy_debug_dump(root);
    b3 = l4buddy_alloc(root, 3*1024);
    l4buddy_debug_dump(root);
    l4buddy_free(root, b1);
    l4buddy_debug_dump(root);
    b4 = l4buddy_alloc(root, 2*1024);
    b5 = l4buddy_alloc(root, 3*1024);
    b6 = l4buddy_alloc(root, 4*1024);

    l4buddy_debug_dump(root);

    l4buddy_free(root, b2);
    l4buddy_debug_dump(root);
    l4buddy_free(root, b3);
    l4buddy_debug_dump(root);
    l4buddy_free(root, b4);
    l4buddy_debug_dump(root);
    l4buddy_free(root, b5);
    l4buddy_debug_dump(root);
    l4buddy_free(root, b6);
    l4buddy_debug_dump(root);
}

void test2(void);
void test2(void){
    int i;
    l4buddy_root *root = l4buddy_create(memory_pool, sizeof(memory_pool));

    unsigned char*addr;

    l4buddy_debug_dump(root);
    for(i=1;;i++){
	LOG("allocating %dth element\n", i);
	addr = (unsigned char*)l4buddy_alloc(root, 1024);
	if(addr) memset(addr, 0, 1024);
	l4buddy_debug_dump(root);
	if(!addr) break;
    }
    LOG("allocated %d KB\n", i);
}

void test3(void);
void test3(void){
    int i;
    l4buddy_root *root = l4buddy_create(memory_pool30, sizeof(memory_pool30));

    unsigned char**addr = alloca(30*sizeof(void*));

    l4buddy_debug_dump(root);
    for(i=0;;i++){
	LOG("allocating %dth element\n", i);
	addr[i] = (unsigned char*)l4buddy_alloc(root, 1024);
	if(addr[i]) memset(addr[i], 0x55, 1024);
	if(!addr[i]) break;
    }
    LOG("allocated %d KB\n", i);
    if(i!=29){
	printf("allocated %d times 1KB, expected %d", i, 29);
	exit(1);
    }
    for(i--;i>=0;i--){
	LOG("freeing %dth element\n", i);
	l4buddy_free(root, addr[i]);
    }
    l4buddy_debug_dump(root);
}

static char memory_pool_test4 [64*1024*1024+64*1024+1024];
void test4(void);
void test4(void){
    int i;
    l4buddy_root *root = l4buddy_create(memory_pool_test4,
					sizeof(memory_pool_test4));

    unsigned char**addr = alloca(64*1024*sizeof(void*));
    unsigned allocated=0;
    unsigned allocated_real=0;

    memset(addr, 0, 64*1024*sizeof(void*));
    l4buddy_debug_dump(root);
    LOG("free in pool: %d KB, allocated in pool: %d KB",
	l4buddy_debug_free(root)/1024, l4buddy_debug_allocated(root)/1024);
    for(i=0; i<64*1024;i++){
	unsigned size = 1024*(i%9)+1024;
	unsigned size_real;
	size_real = 1<<l4util_log2(size);
	if(size_real<size) size_real<<=1;

	addr[i]=l4buddy_alloc(root, size);
	if(addr[i]){
	    allocated+=size;
	    allocated_real+=size_real;
	}
    }
    LOG("allocated %d KB (%d KB) in sum",
	allocated/1024, allocated_real/1024);
    LOG("free in pool: %d KB, allocated in pool: %d KB",
	l4buddy_debug_free(root)/1024, l4buddy_debug_allocated(root)/1024);
    l4buddy_debug_dump(root);
    for(i=0; i<64*1024;i++){
	if(addr[i]){
	    l4buddy_free(root, addr[i]);
	    addr[i]=0;
	}
    }
    l4buddy_debug_dump(root);
    LOG("free in pool: %d KB, allocated in pool: %d KB",
	l4buddy_debug_free(root)/1024, l4buddy_debug_allocated(root)/1024);
}

int main(int argc, const char**argv){
    test4();
    return 0;
}

