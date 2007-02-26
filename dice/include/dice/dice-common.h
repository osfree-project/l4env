#ifndef __DICE_COMMON_H__
#define __DICE_COMMON_H__

// needed for str* functions
#if !defined(LINUX_ON_L4) && !defined(CONFIG_L4_LINUX)
#include <string.h>
#endif

#undef _dice_alloca
#ifdef __GNUC__
#define _dice_alloca(size)	__builtin_alloca(size)
#endif

#ifndef DICE_IID_BITS
#define DICE_IID_BITS 20
#endif

#ifndef DICE_FID_MASK
#define DICE_FID_MASK 0xfffff
#endif

#include "dice/dice-corba-types.h"

/*
 * declares CORBA_alloc and CORBA_free - implementation has to be user provided
 */
void* CORBA_alloc(unsigned long size);
void CORBA_free(void *ptr);

#endif // __DICE_COMMON_H__
