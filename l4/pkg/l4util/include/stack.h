/**
 * \file   l4util/include/stack.h
 * \brief  Some helper functions for stack manipulation. Newer versions of
 *         gcc forbid to cast the lvalue of an expression resulting that
 *         the following expression is invalid:
 *
 *         *--((l4_threadid_t)esp) = tid
 *
 * \date   03/2004
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef _L4UTIL_STACK_H
#define _L4UTIL_STACK_H

#include <l4/sys/types.h>
#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

L4_INLINE void l4util_stack_push_mword(l4_addr_t *stack, l4_mword_t val);
L4_INLINE void l4util_stack_push_threadid(l4_addr_t *stack, l4_threadid_t val);

/*****************************************************************************/
/**
 * \brief Get current stack pointer.
 * 
 * \return stack pointer.
 */
L4_INLINE l4_addr_t l4util_stack_get_sp(void);

/*
 * Implementations.
 */

#include <l4/util/stack_impl.h>

L4_INLINE void
l4util_stack_push_mword(l4_addr_t *stack, l4_mword_t val)
{
  l4_mword_t *esp = (l4_mword_t*)(*stack);
  *--esp = val;
  *stack = (l4_addr_t)esp;
}

L4_INLINE void
l4util_stack_push_threadid(l4_addr_t *stack, l4_threadid_t val)
{
  l4_threadid_t *esp = (l4_threadid_t*)(*stack);
  *--esp = val;
  *stack = (l4_addr_t)esp;
}

EXTERN_C_END

#endif
