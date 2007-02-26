/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/examples/exception/main.c
 * \brief  Short exception handler example
 *
 * \date   08/16/2004
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/l4rm/l4rm.h>
#include <l4/util/idt.h>
#include <l4/log/l4log.h>

/**
 * Exception state, it is saved by the low level exception entry function
 * (pf_entry in exc_entry.S)
 */
typedef struct exc_state
{
  /* registers, saved with pusha */
  l4_umword_t  edi;
  l4_umword_t  esi;
  l4_umword_t  ebp;
  l4_umword_t  esp;
  l4_umword_t  ebx;
  l4_umword_t  edx;
  l4_umword_t  ecx;
  l4_umword_t  eax;

  /* exception stack frame */
  l4_umword_t  addr;       /* page-fault address */
  l4_umword_t  error;      /* exception error code */
  l4_umword_t  eip;        /* instruction pointer */
  l4_umword_t  cs;         /* code segment */
  l4_umword_t  eflags;     /* flags */
} exc_state_t;

/* pf exception entry point, see exc_entry.S */
extern void * pf_entry;

/* IDT */
#define PF_EXC_NO       14
#define NO_IDT_ENTRIES  (PF_EXC_NO + 1)
#define IDT_SIZE        (sizeof(l4util_idt_header_t) + \
                         NO_IDT_ENTRIES * sizeof(l4util_idt_desc_t))

/*****************************************************************************/
/**
 * \brief  Page-fault handler, called from pf_entry
 * 
 * \param  state         Exception state
 */
/*****************************************************************************/ 
void
pf_handler(exc_state_t * state);
void
pf_handler(exc_state_t * state)
{
  LOGl("[PF] %s at 0x%08x, eip 0x%08x", (state->error & 2) ? "write" : "read",
       state->addr, state->eip);
  enter_kdebug("PF");
}

/*****************************************************************************/
/**
 * \brief  Main.
 */
/*****************************************************************************/ 
int
main(int argc, char * argv[])
{
  unsigned char idt_buf[IDT_SIZE];
  l4util_idt_header_t * idt = (l4util_idt_header_t *)idt_buf;

  /* init idt */
  l4util_idt_init(idt, NO_IDT_ENTRIES);
  l4util_idt_entry(idt, PF_EXC_NO, (void *)&pf_entry);
  l4util_idt_load(idt);

  /* enable exception in region mapper */
  l4rm_enable_pagefault_exceptions();

  /* raise page fault */
  *(int *)0 = 1;

  /* done */
  return 0;
}
