#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <l4/sys/kdebug.h>
#include <l4/util/list_alloc.h>
#include "libc_support.h"


void reset_malloc(void);

/* for the l4_start_stop dietlibc backend */
void crt0_sys_destruction(void);
void crt0_sys_destruction(void)
{
}

extern unsigned     __malloc_pool[MALLOC_POOL_SIZE / sizeof(unsigned)];
static l4la_free_t *__malloc_list;

void
reset_malloc(void)
{
  l4la_init(&__malloc_list);
  l4la_free(&__malloc_list, __malloc_pool, sizeof(__malloc_pool));
}

void *
malloc(size_t size)
{
  void *a;
  l4_size_t *s;

  size += sizeof(l4_size_t);
  if (EXPECT_FALSE(!(a = l4la_alloc(&__malloc_list, size, 2))))
    {
      printf("ROOT: malloc memory exhausted.\n");
      return 0;
    }
  s  = (l4_size_t*)a;
  *s = size;
  return s+1;
}

void *
calloc(size_t nmemb, size_t size)
{
  size_t sz = size*nmemb;
  return (nmemb && (sz/nmemb != size)) 
    ? 0 
    : malloc(sz);
}

void *
realloc(void *ptr, size_t size)
{
  void *p = malloc(size);
  if (ptr && p)
    memcpy(p, ptr, size);
  return p;
}

void free(void *ptr)
{
  l4_size_t *s = (l4_size_t*)ptr - 1;
  l4la_free(&__malloc_list, s, *s);
}

extern __typeof(malloc)  demangle_malloc  __attribute__((alias("malloc")));
extern __typeof(calloc)  demangle_calloc  __attribute__((alias("calloc")));
extern __typeof(realloc) demangle_realloc __attribute__((alias("realloc")));
extern __typeof(free)    demangle_free    __attribute__((alias("free")));
