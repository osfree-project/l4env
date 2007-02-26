#include <stdlib.h>

typedef void (*function)(void);

#define NUM_ATEXIT	128

static function __atexitlist[NUM_ATEXIT];
static volatile int atexit_counter;

int atexit(function t) {

  if (atexit_counter<NUM_ATEXIT) {
    __atexitlist[atexit_counter++]=t;
    return 0;
  }
  return -1;
}

extern void _exit(int code) __attribute__((noreturn));

//#include <stdio.h>

void exit(int code) {
  //printf("Run %d atexits\n",i);
  while(atexit_counter) {
    //printf(" @%p\n",__atexitlist[i-1]);
    if(__atexitlist[--atexit_counter])
      __atexitlist[atexit_counter]();
  }
  //  puts("run _exit");
  _exit(code);
}
