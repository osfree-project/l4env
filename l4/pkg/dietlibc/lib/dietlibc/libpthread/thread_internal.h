#ifndef __THREAD_INTERNAL_H__
#define __THREAD_INTERNAL_H__

#ifdef L4_THREAD_SAFE

/* TODO: bring in definitions from the "real" libpthread 
 *       whenever they are needed.
 */

/* diet libc syscalls */
void  __libc_free(void*ptr);
void *__libc_malloc(size_t size);
void *__libc_realloc(void*ptr,size_t size);

#endif // L4_THREAD_SAFE

#endif
