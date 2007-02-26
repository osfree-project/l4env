/*****************************************************************************
 * random.c                                                                  *
 * pseudo-random number generator                                            *
 *****************************************************************************/

unsigned long l4util_rand(void);
void l4util_srand (unsigned long);

#define L4_RAND_MAX 65535

static unsigned int l4_rand_next = 1;

unsigned long l4util_rand(void)
{
  l4_rand_next = l4_rand_next * 1103515245 + 12345;
  return ((l4_rand_next >>16) & L4_RAND_MAX);
}

void l4util_srand (unsigned long seed)
{
  l4_rand_next = seed;
}

