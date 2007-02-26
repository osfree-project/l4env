#ifndef L4_CXX_ATEXIT_H__
#define L4_CXX_ATEXIT_H__

typedef void (*__cxa_atexit_function)(void);

void __cxa_do_atexit();
int __cxa_atexit(__cxa_atexit_function t) asm ("atexit");

#endif
