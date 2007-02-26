/* $Id$ */
/**
 * \file   l4util/include/rand.h
 * \brief  Simple Pseudo-Random Number Generator
 *
 * \date   1998
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de> */

#ifndef __L4UTIL_RAND_H
#define __L4UTIL_RAND_H

#define L4_RAND_MAX 65535

#include <l4/sys/compiler.h>
#include <l4/sys/l4int.h>

EXTERN_C_BEGIN

/** Deliver next random number
 * \return random number
 */
l4_uint32_t
l4util_rand(void);

/** Initialize random number generator
 * \param seed value to initialize
 */
void
l4util_srand (l4_uint32_t seed);

EXTERN_C_END

#endif /* __L4UTIL_RAND_H */
