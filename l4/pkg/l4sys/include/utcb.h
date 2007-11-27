/*****************************************************************************/
/*!
 * \file    l4sys/include/utcb.h
 * \brief   UTCB definitions.
 * \ingroup api_types
 */
/*****************************************************************************/
#ifndef _L4_SYS_UTCB_H
#define _L4_SYS_UTCB_H

#include <l4/sys/types.h>

EXTERN_C_BEGIN

/**
 * UTCB.
 * \ingroup api_utcb
 */
typedef struct
{
  union {
    l4_umword_t                  values[L4_UTCB_GENERIC_DATA_SIZE]; /**< UTCB pure values */
    struct l4_utcb_exception     exc;                               /**< UTCB exception values */
    struct l4_utcb_ex_regs_args  ex_regs;                           /**< UTCB l4_thread_ex_regs values */
    struct l4_utcb_task_new_args task_new;                          /**< UTCB l4_task_new values */
  };

  l4_umword_t buffers[31];       /**< buffers */
  l4_timeout_t xfer;             /**< transmit timeout */
} l4_utcb_t;

/**
 * \defgroup api_utcb UTCB functionality
 * \ingroup  api_calls
 * \brief    Defines UTCB related functions and types.
 */

/**
 * Get the address to a thread's UTCB, kernel interface.
 * \ingroup api_utcb
 */
L4_INLINE l4_utcb_t *l4_utcb_get(void);

/**
 * Access function to get the program counter of the exception state.
 * \ingroup api_utcb
 */
L4_INLINE l4_umword_t l4_utcb_exc_pc(l4_utcb_t *u);

/**
 * Get the value out of an exception UTCB that describes the type of
 * exception.
 * \ingroup api_utcb
 */
L4_INLINE unsigned long l4_utcb_exc_typeval(l4_utcb_t *u);

/**
 * Function to check whether an exception IPC is a page fault, also applies
 * to I/O pagefaults.
 *
 * \returns 0 if not, != 0 if yes
 * \ingroup api_utcb
 */
L4_INLINE int l4_utcb_exc_is_pf(l4_utcb_t *u);

/**
 * Function to get the L4 style page fault address out of an exception.
 * \ingroup api_utcb
 */
L4_INLINE l4_addr_t l4_utcb_exc_pfa(l4_utcb_t *u);

/**
 * Enable or disable inheritence of FPU state to receiver.
 * \ingroup api_utcb
 */
L4_INLINE void l4_utcb_inherit_fpu(l4_utcb_t *u, int switch_on);

/**
 * Get the UTCB address. This functions should be used by libraries.
 * \ingroup api_utcb
 *
 * This is a weak function which can be overwritten by applications.
 *
 * \returns UTCB
 */
l4_utcb_t *l4sys_utcb_get(void);

/**************************************************************************
 * Implementations
 **************************************************************************/

L4_INLINE void l4_utcb_inherit_fpu(l4_utcb_t *u, int switch_on)
{
  if (switch_on)
    u->buffers[L4_UTCB_BUFFER_ACCEPTOR] |= L4_UTCB_INHERIT_FPU;
  else
    u->buffers[L4_UTCB_BUFFER_ACCEPTOR] &= ~L4_UTCB_INHERIT_FPU;
}

EXTERN_C_END

#endif /* ! _L4_SYS_UTCB_H */
