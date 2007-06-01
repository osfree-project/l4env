/* $Id$ */
/*****************************************************************************/
/**
 * \file    l4sys/include/ARCH-x86/utcb.h
 * \brief   UTCB definitions for V2/X0.
 * \ingroup api_types
 */
/*****************************************************************************/
#ifndef _L4_SYS_UTCB_H
#define _L4_SYS_UTCB_H

#ifndef DICE
#include <l4/sys/types.h>

#define L4_UTCB_EXCEPTION_REGS_SIZE    16
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
  l4_umword_t gs;
  l4_umword_t fs;

  l4_umword_t edi;
  l4_umword_t esi;
  l4_umword_t ebp;
  l4_umword_t pfa;
  l4_umword_t ebx;
  l4_umword_t edx;
  l4_umword_t ecx;
  l4_umword_t eax;

  l4_umword_t trapno;
  l4_umword_t err;

  l4_umword_t eip;
  l4_umword_t dummy1;
  l4_umword_t eflags;
  l4_umword_t esp;
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

struct l4_utcb_task_new_args
{
  l4_umword_t _res0[2];
  l4_threadid_t caphandler;
  l4_quota_desc_t quota;
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
    struct l4_utcb_task_new_args task_new;
  };

  l4_umword_t buffers[31];
  l4_timeout_t xfer;

} l4_utcb_t;

/**
 * Get the address to a thread's UTCB.
 * \ingroup api_utcb
 */
L4_INLINE l4_utcb_t *l4_utcb_get(void);

/**
 * Access function to get the program counter of the exception state.
 * \ingroup api_utcb
 */
L4_INLINE l4_umword_t l4_utcb_exc_pc(l4_utcb_t *u);

/**
 * Enable exception IPC for current handler thread.
 * \ingroup api_utcb
 */
L4_INLINE void l4_utcb_exception_ipc_enable(void);

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

L4_INLINE void l4_utcb_exception_ipc_enable(void)
{
//  l4_utcb_get()->status |= L4_UTCB_EXCEPTION_IPC_ENABLED;
}


#endif

#endif /* ! _L4_SYS_UTCB_H */
