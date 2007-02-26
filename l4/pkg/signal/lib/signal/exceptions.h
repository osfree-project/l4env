#ifndef l4signal_EXC_H
#define l4signal_EXC_H

#include <l4/sys/types.h>
#include <l4/util/idt.h>
#include "local.h"

// stack layout after an exception occured
typedef struct exception_state
{
    l4_umword_t edi;
    l4_umword_t esi;
    l4_umword_t ebp;
    l4_umword_t esp;
    l4_umword_t ebx;
    l4_umword_t edx;
    l4_umword_t ecx;
    l4_umword_t eax;

    l4_umword_t addr;
    l4_umword_t error;
    l4_umword_t eip;
    l4_umword_t cs;
    l4_umword_t eflags;
} exception_state;

// handlers to raise exceptions
void l4signal_segv_handler(exception_state *ex);
void l4signal_fpe_handler(exception_state *ex);
void l4signal_illegal_handler(exception_state *ex);
void l4signal_stackf_handler(exception_state *ex);

#endif
