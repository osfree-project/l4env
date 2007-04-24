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
#define L4_UTCB_GENERIC_DATA_SIZE      29

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
  unsigned long status;
  union {
    l4_umword_t              values[L4_UTCB_GENERIC_DATA_SIZE];
    struct l4_utcb_exception exc;
  };
  unsigned long snd_size;
  unsigned long rcv_size;
} l4_utcb_t;

/**
 * Get the address to a thread's UTCB.
 * \ingroup api_utcb
 */
L4_INLINE l4_utcb_t *l4_utcb_get(void);

/**
 * Enable exception IPC for current handler thread.
 * \ingroup api_utcb
 */
L4_INLINE void l4_utcb_exception_ipc_enable(void);

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

/**
 * Function to check whether an IPC was an exception IPC.
 * \ingroup api_utcb
 */
int l4_utcb_exc_is_exc_ipc(l4_umword_t dw0, l4_umword_t dw1);

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

  __asm__ __volatile__ ("ldr %0, [%1]" : "=r" (utcb) : "r"(0xffffd000));

  return utcb;
}

L4_INLINE l4_utcb_t *l4_utcb_get_disabled(void)
{
  l4_utcb_t *utcb = (l4_utcb_t*)NULL;
  return utcb;
}

L4_INLINE l4_umword_t l4_utcb_exc_pc(l4_utcb_t *u)
{
  return u->exc.pc;
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
