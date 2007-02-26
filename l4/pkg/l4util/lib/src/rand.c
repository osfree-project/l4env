/*
 * $Id$
 */

/*****************************************************************************
 * random.c                                                                  *
 * pseudo-random number generator                                            *
 *****************************************************************************/

#include <l4/util/rand.h>

static unsigned int l4_rand_next = 1;

l4_uint32_t
l4util_rand(void)
{
  l4_rand_next = l4_rand_next * 1103515245 + 12345;
  return ((l4_rand_next >>16) & L4_RAND_MAX);
}

void
l4util_srand (l4_uint32_t seed)
{
  l4_rand_next = seed;
}

