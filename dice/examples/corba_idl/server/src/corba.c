//#define DICE_TARGET_API L4_V2
#include "dice/dice.h"
#include <stdlib.h>

l4_ssize_t l4libc_heapsize = 10*1024;

void *CORBA_alloc(unsigned long size)
{
    return malloc(size);
}

void CORBA_free(void* addr)
{
    free(addr);
}

