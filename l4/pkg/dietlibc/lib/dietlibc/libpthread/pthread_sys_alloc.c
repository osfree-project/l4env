#ifdef L4_THREAD_SAFE

#include <unistd.h>

#ifdef WANT_THREAD_SAFE
#include <pthread.h>
#endif

#include "thread_internal.h"
#include <stdlib.h>
#include <l4/semaphore/semaphore.h>

static l4semaphore_t mutex_alloc = L4SEMAPHORE_UNLOCKED;

void free(void *ptr) {
  l4semaphore_down(&mutex_alloc);
  __libc_free(ptr);
  l4semaphore_up(&mutex_alloc);
}

void *malloc(size_t size) {
  register void *ret;
  l4semaphore_down(&mutex_alloc);
  ret=__libc_malloc(size);
  l4semaphore_up(&mutex_alloc);
  return ret;
}

void* realloc(void* ptr, size_t size) {
  register void *ret;
  l4semaphore_down(&mutex_alloc);
  ret=__libc_realloc(ptr, size);
  l4semaphore_up(&mutex_alloc);
  return ret;
}

#endif
