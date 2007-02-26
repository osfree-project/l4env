/* $Id$ */
/*****************************************************************************/
/**
 * \file    l4sys/include/ARCH-arm/utcb.h
 * \brief   UTCB definitions.
 * \ingroup api_types
 */
/*****************************************************************************/
#ifndef _L4_SYS_UTCB_H
#define _L4_SYS_UTCB_H

#include <l4/sys/types.h>
#include <l4/sys/compiler.h>

#define L4_UTCB_EXCEPTION_IPC_ENABLED  1
#define L4_UTCB_EXCEPTION_IPC_DISABLED 0
#define L4_UTCB_EXCEPTION_IPC_COOKIE1  (-0x5UL)
#define L4_UTCB_EXCEPTION_IPC_COOKIE2  (-0x21504151UL)
#define L4_UTCB_EXCEPTION_REGS_SIZE    20

enum {
  L4_EXCEPTION_REPLY_DW0_DEALIEN = 1,
};

/**
 * UTCB structure for exceptions.
 * \ingroup api_utcb
 */
struct l4_utcb_exception
{
  l4_umword_t pfa;
  l4_umword_t err;

  l4_umword_t r[13];
  l4_umword_t cpsr;
  l4_umword_t sp;
  l4_umword_t ulr;
  l4_umword_t _dummy1;
  l4_umword_t pc;
};

/**
 * UTCB.
 * \ingroup api_utcb
 */
typedef struct
{
  struct l4_utcb_exception exc;
} l4_utcb_t;

/**
 * Get the address to a thread's UTCB.
 * \ingroup api_utcb
 */
l4_utcb_t *l4_utcb_get_disabled(void);

/**
 * Access function to get the program counter of the exception state.
 * \ingroup api_utcb
 */
l4_umword_t l4_utcb_exc_pc(l4_utcb_t *u);


/*
 * ==================================================================
 * Implementations.
 */

L4_INLINE l4_utcb_t *l4_utcb_get_disabled(void)
{
  l4_utcb_t *utcb = NULL;
  return utcb;
}

L4_INLINE l4_umword_t l4_utcb_exc_pc(l4_utcb_t *u)
{
  return u->exc.pc;
}

#endif /* ! _L4_SYS_UTCB_H */
