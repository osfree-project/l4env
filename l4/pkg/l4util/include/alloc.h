/*!
 * \file   util/include/alloc.h
 * \brief  Allocator using a bit-array
 *
 * \date   09/14/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __UTIL_INCLUDE_ALLOC_H_
#define __UTIL_INCLUDE_ALLOC_H_
#include <l4/sys/l4int.h>
#include <l4/util/bitops.h>
#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

typedef struct {
    int base, count, next_elem;
    l4_uint32_t *bits;
} l4util_alloc_t;

l4util_alloc_t *l4util_alloc_init(int count, int base);
int l4util_alloc_avail(l4util_alloc_t *alloc, int elem);
int l4util_alloc_occupy(l4util_alloc_t *alloc, int elem);
int l4util_alloc_alloc(l4util_alloc_t *alloc);
int l4util_alloc_free(l4util_alloc_t *alloc, int elem);

EXTERN_C_END
#endif
