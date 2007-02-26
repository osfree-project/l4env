#include "cxx_atexit.h"

#define NUM_ATEXIT	64

static __cxa_atexit_function __atexitlist[NUM_ATEXIT];
static volatile int atexit_counter;

int __cxa_atexit(__cxa_atexit_function t) {

  if (atexit_counter<NUM_ATEXIT) {
    __atexitlist[atexit_counter++]=t;
    return 0;
  }
  return -1;
}

void __cxa_do_atexit()
{
  while(atexit_counter) {
    if(__atexitlist[--atexit_counter])
      __atexitlist[atexit_counter]();
  }
}

