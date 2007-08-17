/* $Id: utcb.h,v 1.11 2006/06/08 13:24:36 fm3 Exp $ */
/*****************************************************************************/
/**
 * \file    l4sys/include/ARCH-amd64/utcb.h
 * \brief   UTCB definitions.
 * \ingroup api_types
 */
/*****************************************************************************/
#ifndef _L4_SYS_UTCB_H
#define _L4_SYS_UTCB_H

#include <l4/sys/types.h>

enum {
  L4_EXCEPTION_REPLY_DW0_DEALIEN = 1,

  L4_UTCB_EXCEPTION_REGS_SIZE = 23,
  L4_UTCB_GENERIC_DATA_SIZE   = 32,

  L4_UTCB_BUFFER_ACCEPTOR = 0,

  L4_UTCB_INHERIT_FPU = 2,
};

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

struct l4_utcb_ex_regs_args
{
  l4_threadid_t _res0;
  l4_threadid_t caphandler;
  l4_threadid_t _res1;
};

struct l4_utcb_task_new_args
{
  l4_umword_t     _res0;
  l4_threadid_t   caphandler;
  l4_quota_desc_t quota;
  l4_threadid_t   _res1;
};

/**
 * UTCB.
 * \ingroup api_utcb
 */
typedef struct
{
  union {
    l4_umword_t                  values[L4_UTCB_GENERIC_DATA_SIZE];
    struct l4_utcb_exception     exc;
    struct l4_utcb_ex_regs_args  ex_regs;
    struct l4_utcb_task_new_args task_new;
  };

  l4_umword_t buffers[31];
  l4_timeout_t xfer;
} l4_utcb_t;

L4_INLINE l4_utcb_t *l4_utcb_get(void);

L4_INLINE l4_umword_t l4_utcb_exc_pc(l4_utcb_t *u);

L4_INLINE unsigned long l4_utcb_exc_typeval(l4_utcb_t *u);

L4_INLINE int l4_utcb_exc_is_pf(l4_utcb_t *u);

L4_INLINE l4_addr_t l4_utcb_exc_pfa(l4_utcb_t *u);

L4_INLINE void l4_utcb_inherit_fpu(l4_utcb_t *u, int switch_on);

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

#endif /* ! _L4_SYS_UTCB_H */
