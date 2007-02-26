#include "dice/dice.h"
#include <stdlib.h>

l4_ssize_t l4libc_heapsize = 100*1024;

void *CORBA_alloc(unsigned long size)
{
  return malloc(size);
}

void CORBA_free(void* addr)
{
  free(addr);
}

