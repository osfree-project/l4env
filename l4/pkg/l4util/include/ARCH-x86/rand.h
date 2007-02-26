/*
 * $Id$
 */

/*****************************************************************************
 * rand.h                                                                    *
 * pseudo-random number generator                                            *
 *****************************************************************************/
#ifndef __L4UTIL_RAND_H
#define __L4UTIL_RAND_H

#define L4_RAND_MAX 65535

#ifdef  __cplusplus
extern "C" {
#endif

unsigned int l4_rand(void);
void l4_srand (unsigned int seed);

#ifdef  __cplusplus
	   }
#endif

#endif /* __L4UTIL_RAND_H */
