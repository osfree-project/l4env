#ifndef __DIETLIBC_LIB_BACKENDS_START_STOP__EXIT_H_
#define __DIETLIBC_LIB_BACKENDS_START_STOP__EXIT_H_

void _exit(int code) __attribute__ ((__noreturn__));
void __thread_doexit(int code);

#endif
