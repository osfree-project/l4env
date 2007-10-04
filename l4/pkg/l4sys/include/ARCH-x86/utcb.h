/*****************************************************************************/
/*!
 * \file    l4sys/include/ARCH-x86/utcb.h
 * \brief   UTCB definitions for X86.
 * \ingroup api_types
 */
/*****************************************************************************/
#ifndef __L4_SYS__INCLUDE__ARCH_X86__UTCB_H__
#define __L4_SYS__INCLUDE__ARCH_X86__UTCB_H__

#include <l4/sys/types.h>

/**
 * \defgroup api_utcb_x86 UTCB functionality for x86
 * \ingroup  api_utcb
 */

/**
 * UTCB structure for l4_thread_ex_regs arguments
 *
 * \ingroup api_utcb_x86
 */
struct l4_utcb_ex_regs_args
{
  l4_threadid_t _res0;        /**< reserved \internal */
  l4_threadid_t caphandler;   /**< Capability handler */
  l4_threadid_t _res1;        /**< reserved \internal */
};

/**
 * UTCB structure for l4_task_new arguments
 *
 * \ingroup api_utcb_x86
 */
struct l4_utcb_task_new_args
{
  l4_umword_t     _res0;      /**< reserved \internal */
  l4_threadid_t   caphandler; /**< Capability handler */
  l4_quota_desc_t quota;      /**< Quota for tasks */
  l4_threadid_t   _res1;      /**< reserved \internal */
};

/**
 * \ingroup api_utcb_x86
 */
enum {
  L4_EXCEPTION_REPLY_DW0_DEALIEN = 1,

  L4_UTCB_EXCEPTION_REGS_SIZE = 16,
  L4_UTCB_GENERIC_DATA_SIZE   = 32,

  L4_UTCB_BUFFER_ACCEPTOR = 0,

  L4_UTCB_INHERIT_FPU = 2,
};

/**
 * UTCB structure for exceptions.
 *
 * \ingroup api_utcb_x86
 */
struct l4_utcb_exception
{
  l4_umword_t gs;      /**< gs register */
  l4_umword_t fs;      /**< fs register */

  l4_umword_t edi;     /**< edi register */
  l4_umword_t esi;     /**< esi register */
  l4_umword_t ebp;     /**< ebp register */
  l4_umword_t pfa;     /**< page fault address */
  l4_umword_t ebx;     /**< ebx register */
  l4_umword_t edx;     /**< edx register */
  l4_umword_t ecx;     /**< ecx register */
  l4_umword_t eax;     /**< eax register */

  l4_umword_t trapno;  /**< trap number */
  l4_umword_t err;     /**< error code */

  l4_umword_t eip;     /**< instruction pointer */
  l4_umword_t dummy1;  /**< dummy \internal */
  l4_umword_t eflags;  /**< eflags */
  l4_umword_t esp;     /**< stack pointer */
};


#include_next <l4/sys/utcb.h>

/*
 * ==================================================================
 * Implementations.
 */

L4_INLINE l4_utcb_t *l4_utcb_get(void)
{
  l4_utcb_t *utcb;
  __asm__ __volatile__ ("mov %%gs:0, %0" : "=r" (utcb));
  return utcb;
}

L4_INLINE l4_umword_t l4_utcb_exc_pc(l4_utcb_t *u)
{
  return u->exc.eip;
}

L4_INLINE l4_umword_t l4_utcb_exc_typeval(l4_utcb_t *u)
{
  return u->exc.trapno;
}

L4_INLINE int l4_utcb_exc_is_pf(l4_utcb_t *u)
{
  return u->exc.trapno == 14;
}

L4_INLINE l4_addr_t l4_utcb_exc_pfa(l4_utcb_t *u)
{
  return (u->exc.pfa & ~3) | (u->exc.err & 2);
}

L4_INLINE void l4_utcb_inherit_fpu(l4_utcb_t *u, int switch_on)
{
  if (switch_on)
    u->buffers[L4_UTCB_BUFFER_ACCEPTOR] |= L4_UTCB_INHERIT_FPU;
  else
    u->buffers[L4_UTCB_BUFFER_ACCEPTOR] &= ~L4_UTCB_INHERIT_FPU;
}

#endif /* ! __L4_SYS__INCLUDE__ARCH_X86__UTCB_H__ */
