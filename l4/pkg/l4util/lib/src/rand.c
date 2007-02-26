/*
 * $Id$
 */

/*****************************************************************************
 * random.c                                                                  *
 * pseudo-random number generator                                            *
 *****************************************************************************/

#include <l4/util/rand.h>

static unsigned int l4_rand_next = 1;

unsigned int l4_rand(void)
{
  l4_rand_next = l4_rand_next * 1103515245 + 12345;
  return ((l4_rand_next >>16) & L4_RAND_MAX);
}

void l4_srand (unsigned int seed)
{
  l4_rand_next = seed;
}

