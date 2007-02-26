
// external prototype cause we don't want to include stdlib.h cause we
// use the plain mode without the path to any C library
int atexit(void (*__function)(void));

int __cxa_atexit(void (*function)(void));

int
__cxa_atexit(void (*function)(void))
{
  return atexit(function);
}

