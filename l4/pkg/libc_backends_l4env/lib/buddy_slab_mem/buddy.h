/*!
 * \file   libc_backends_l4env/lib/buddy_slab_mem/buddy.h
 * \brief  buddy structures
 *
 * \date   08/18/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __LIBC_BACKENDS_L4ENV_LIB_BUDDY_SLAB_MEM_H_BUDDY_H_
#define __LIBC_BACKENDS_L4ENV_LIB_BUDDY_SLAB_MEM_H_BUDDY_H_

// Element-Size is 1K
#define L4BUDDY_BUDDY_SHIFT 10
#define L4BUDDY_BUDDY_SIZE	 (1<<L4BUDDY_BUDDY_SHIFT)
#define L4BUDDY_BUDDY_MASK	 (L4BUDDY_BUDDY_SIZE-1)

typedef struct l4buddy_t l4buddy_t;
typedef struct l4buddy_root l4buddy_root;

extern l4buddy_root* l4buddy_create(void*base_addr, unsigned size);
extern void* l4buddy_alloc(l4buddy_root*root, unsigned size);
extern void l4buddy_free(l4buddy_root*root, void*addr);

/* debug functions. Only defined in debugging version, ie DEBUG_BUDDY
   was define during compilation of lib */
extern int l4buddy_debug_free(l4buddy_root*root);
extern int l4buddy_debug_allocated(l4buddy_root*root);
extern void l4buddy_debug_dump(l4buddy_root*root);
#endif
