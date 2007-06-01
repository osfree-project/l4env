/* $Id: utcb.h,v 1.11 2006/06/08 13:24:36 fm3 Exp $ */
/*****************************************************************************/
/**
 * \file    l4sys/include/ARCH-x86/utcb.h
 * \brief   UTCB definitions for V2/X0.
 * \ingroup api_types
 */
/*****************************************************************************/
#ifndef _L4_SYS_UTCB_H
#define _L4_SYS_UTCB_H

#include <l4/sys/types.h>

enum {
  L4_UTCB_EXCEPTION_FPU_INHERIT  = 2,
};

#define L4_UTCB_EXCEPTION_REGS_SIZE    23
#define L4_UTCB_GENERIC_DATA_SIZE      32

enum {
  L4_EXCEPTION_REPLY_DW0_DEALIEN = 1,
};

/**
 * \defgroup api_utcb UTCB functionality
 * \ingroup  api_calls
 * \brief    Defines UTCB related functions and types.
 */

/**
 * UTCB structure for exceptions.
 *
 * Structure size: 4*16=64 Bytes
 *
 * \ingroup api_utcb
 */
struct l4_utcb_exception
{
  l4_umword_t r15;
  l4_umword_t r14;
  l4_umword_t r13;
  l4_umword_t r12;
  l4_umword_t r11;
  l4_umword_t r10;
  l4_umword_t r9;
  l4_umword_t r8;
  l4_umword_t rdi;
  l4_umword_t rsi;
  l4_umword_t rbp;
  l4_umword_t pfa;
  l4_umword_t rbx;
  l4_umword_t rdx;
  l4_umword_t rcx;
  l4_umword_t rax;

  l4_umword_t trapno;
  l4_umword_t err;
  union {
    l4_umword_t eip;
    l4_umword_t rip;
  };
  l4_umword_t dummy1;
  union {
    l4_umword_t rflags;
    l4_umword_t eflags;
  };
  union {
    l4_umword_t esp;
    l4_umword_t rsp;
  };
  l4_umword_t ss;
};

/**
 * UTCB structure for l4_thread_ex_regs arguments
 *
 * Structure size: v2: 24 Bytes
 *                 x0: 12 Bytes
 *
 * \ingroup api_utcb
 */
struct l4_utcb_ex_regs_args
{
  l4_threadid_t _res0;
  l4_threadid_t caphandler;
  l4_threadid_t _res1;
};

/**
 * UTCB.
 * \ingroup api_utcb
 */
typedef struct
{
  union {
    l4_umword_t                 values[L4_UTCB_GENERIC_DATA_SIZE];
    struct l4_utcb_exception    exc;
    struct l4_utcb_ex_regs_args ex_regs;
  };
  l4_umword_t buffers[31];
  l4_timeout_t xfer;

} l4_utcb_t;

/**
 * Get the address to a thread's UTCB.
 * \ingroup api_utcb
 */
l4_utcb_t *l4_utcb_get(void);

/**
 * Access function to get the program counter of the exception state.
 * \ingroup api_utcb
 */
l4_umword_t l4_utcb_exc_pc(l4_utcb_t *u);


/*
 * ==================================================================
 * Implementations.
 */

L4_INLINE l4_utcb_t *l4_utcb_get(void)
{
  return *((l4_utcb_t**)(0xffffffffeacfd000));
}

L4_INLINE l4_umword_t l4_utcb_exc_pc(l4_utcb_t *u)
{
  return u->exc.rip;
}


#endif /* ! _L4_SYS_UTCB_H */
