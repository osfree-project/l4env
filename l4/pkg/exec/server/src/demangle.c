#include <stdlib.h>

void* demangle_malloc(size_t size);
void* demangle_calloc(size_t nmemb, size_t size);
void* demangle_realloc(void *ptr, size_t size);
void  demangle_free(void *ptr);

void*
demangle_malloc(size_t size)
{
  return malloc(size);
}

void*
demangle_calloc(size_t nmemb, size_t size)
{
  return calloc(nmemb, size);
}

void*
demangle_realloc(void *ptr, size_t size)
{
  return realloc(ptr, size);
}

void
demangle_free(void *ptr)
{
  return free(ptr);
}
