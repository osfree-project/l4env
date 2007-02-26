/*** GENERAL INCLUDES ***/
#include <stdlib.h>

/*** L4-SPECIFIC INCLUDES ***/
#include <l4/sys/types.h>

// Default heap size for Qt apps is 16 MB. It can be overridden by
// applications, e.g.: 'l4_ssize_t l4libc_heapsize = 64 * 1048576;'
l4_ssize_t __attribute__((weak)) l4libc_heapsize = 16 * 1048576;


void *operator new(size_t s) {
  return malloc(s);
}

void *operator new[](size_t s) {
  return malloc(s);
}

void operator delete(void *p) {
  free(p);
}

void operator delete[](void *p) {
  free(p);
}

