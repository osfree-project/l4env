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

#include <l4/sys/types.h>

enum {
  L4_UTCB_EXCEPTION_IPC_ENABLED  = 1,
  L4_UTCB_EXCEPTION_FPU_INHERIT  = 2,
  L4_UTCB_EXCEPTION_FPU_TRANSFER = 4,
};

#define L4_UTCB_EXCEPTION_IPC_COOKIE1  (-0x5UL)
#define L4_UTCB_EXCEPTION_IPC_COOKIE2  (-0x21504151UL)
#define L4_UTCB_EXCEPTION_REGS_SIZE    16
#define L4_UTCB_GENERIC_DATA_SIZE      16

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
 * UTCB structure for LIPC.
 *
 * Structure size: 24 Bytes
 *
 * \ingroup api_utcb
 */
struct l4_utcb_lipc
{
  l4_uint32_t id_1;	/* Thread ID */
  l4_uint32_t id_2;

  l4_addr_t esp;	/* User SP and IP if the thread is in IPC wait */
  l4_addr_t eip;

  l4_umword_t state;	/* IPC partner, IPC state and thread lock bit */
  l4_umword_t snd_state; /* FPU and sendqueue bit */
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
  l4_umword_t status;   /* l4_umword_t to keep vars umword aligned */
  union {
    l4_umword_t                 values[L4_UTCB_GENERIC_DATA_SIZE];
    struct l4_utcb_exception    exc;
    struct l4_utcb_ex_regs_args ex_regs;
  };
  l4_uint32_t snd_size;
  l4_uint32_t rcv_size;

  struct l4_utcb_lipc		lipc;

  l4_uint32_t			_padding[7]; /* padding to 2^n */
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
 * Function to check whether an IPC was an exception IPC.
 * \ingroup api_utcb
 */
L4_INLINE int l4_utcb_exc_is_exc_ipc(l4_umword_t dw0, l4_umword_t dw1);

/**
 * Enable exception IPC for current handler thread.
 * \ingroup api_utcb
 */
L4_INLINE void l4_utcb_exception_ipc_enable(void);

/**
 * Set UTCB receive size of exception frame size.
 * \ingroup api_utcb
 */
L4_INLINE void l4_utcb_exception_ipc_set_exc_receive_size(void);

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

L4_INLINE int l4_utcb_exc_is_exc_ipc(l4_umword_t dw0, l4_umword_t dw1)
{
  return dw0 == L4_UTCB_EXCEPTION_IPC_COOKIE1
         && dw1 == L4_UTCB_EXCEPTION_IPC_COOKIE2;
}

L4_INLINE void l4_utcb_exception_ipc_enable(void)
{
  l4_utcb_get()->status |= L4_UTCB_EXCEPTION_IPC_ENABLED;
}

L4_INLINE void l4_utcb_exception_ipc_set_exc_receive_size(void)
{
  l4_utcb_get()->rcv_size = L4_UTCB_EXCEPTION_REGS_SIZE;
}

#endif /* ! _L4_SYS_UTCB_H */
