/*****************************************************************************/
/*!
 * \file    l4sys/include/ARCH-arm/utcb.h
 * \brief   UTCB definitions for ARM.
 * \ingroup api_types
 */
/*****************************************************************************/
#ifndef __L4_SYS__INCLUDE__ARCH_ARM__UTCB_H__
#define __L4_SYS__INCLUDE__ARCH_ARM__UTCB_H__

#include <l4/sys/types.h>

/**
 * \defgroup api_utcb_arm UTCB functionality for arm
 * \ingroup  api_utcb
 */

/**
 * UTCB structure for l4_thread_ex_regs arguments
 *
 * \ingroup api_utcb_arm
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
 * \ingroup api_utcb_arm
 */
struct l4_utcb_task_new_args
{
  l4_umword_t     _res0;      /**< reserved \internal */
  l4_threadid_t   caphandler; /**< Capability handler */
  l4_quota_desc_t quota;      /**< Quota for tasks */
  l4_threadid_t   _res1;      /**< reserved \internal */
};

/**
 * \ingroup api_utcb_arm
 */
enum {
  L4_EXCEPTION_REPLY_DW0_DEALIEN = 1,

  L4_UTCB_EXCEPTION_REGS_SIZE = 20,
  L4_UTCB_GENERIC_DATA_SIZE   = 32,

  L4_UTCB_BUFFER_ACCEPTOR = 0,

  L4_UTCB_INHERIT_FPU = 2,
};

/**
 * UTCB structure for exceptions.
 *
 * \ingroup api_utcb_arm
 */
struct l4_utcb_exception
{
  l4_umword_t pfa;     /**< page fault address */
  l4_umword_t err;     /**< error code */

  l4_umword_t r[13];   /**< registers */
  l4_umword_t cpsr;    /**< cpsr */
  l4_umword_t sp;      /**< stack pointer */
  l4_umword_t ulr;     /**< ulr */
  l4_umword_t _dummy1; /**< dummy \internal */
  l4_umword_t pc;      /**< pc */
};

#include_next <l4/sys/utcb.h>

/*
 * ==================================================================
 * Implementations.
 */

L4_INLINE l4_utcb_t *l4_utcb_get(void)
{
  volatile l4_utcb_t *utcb;
  utcb = *(volatile l4_utcb_t **)0xffffd000;
  return (l4_utcb_t *)utcb;
}

L4_INLINE l4_umword_t l4_utcb_exc_pc(l4_utcb_t *u)
{
  return u->exc.pc;
}

L4_INLINE l4_umword_t l4_utcb_exc_typeval(l4_utcb_t *u)
{
  return u->exc.err;
}

L4_INLINE int l4_utcb_exc_is_pf(l4_utcb_t *u)
{
  return u->exc.err & 0x00010000;
}

L4_INLINE l4_addr_t l4_utcb_exc_pfa(l4_utcb_t *u)
{
  return (u->exc.pfa & ~3) | (!(u->exc.err & 0x00020000) << 1);
}

#endif /* ! __L4_SYS__INCLUDE__ARCH_ARM__UTCB_H__ */
