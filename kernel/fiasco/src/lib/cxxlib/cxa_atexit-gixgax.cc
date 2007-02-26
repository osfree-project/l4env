
#include <cstdlib>
#include <cstdio>

typedef void (function_t)(void);

#if 0
#define NUM_CXA_ATEXITIS_MAX 128

static function_t *cxa_atexits[ NUM_CXA_ATEXITIS_MAX ];
static unsigned num_cxa_atexits = 0;

static void do_the_cleaning()
{
  while(num_cxa_atexits)
    cxa_atexits[--num_cxa_atexits]();
}
#endif

extern "C" void __cxa_atexit( function_t *f ) 
{
  atexit(f);
}

#if 0
{
  if(!num_cxa_atexits)
    atexit(&do_the_cleaning);

  if(num_cxa_atexits<NUM_CXA_ATEXITIS_MAX)
    cxa_atexits[num_cxa_atexits++] = f;
  else
    printf("ERROR: too many cxa_exits, can't register %p\n",f);
}

#endif
