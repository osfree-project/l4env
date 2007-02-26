/*
 * Fiasco-UX
 * Architecture specific floating point unit code
 */

IMPLEMENTATION[ux]:

#include <cassert>
#include <sys/ptrace.h>
#include "cpu.h"
#include "regdefs.h"
#include "space_context.h"

/**
 * Init FPU. Does nothing here.
 */
IMPLEMENT
void
Fpu::init()
{}

/**
 * Save FPU state in Fpu_state s. Distinguish between i387 and SSE state
 * @param s Fpu_state to save context in
 */
IMPLEMENT
void
Fpu::save_state (Fpu_state *s)
{
  assert (s->state_buffer());

  if ((Cpu::features() & FEAT_FXSR))
    ptrace (PTRACE_GETFPXREGS, Space_context::current()->pid(), NULL, s->state_buffer());
  else
    ptrace (PTRACE_GETFPREGS,  Space_context::current()->pid(), NULL, s->state_buffer());
}

/**
 * Restore FPU state from Fpu_state s. Distinguish between i387 and SSE state
 * @param s Fpu_state to restore context from
 */
IMPLEMENT
void
Fpu::restore_state (Fpu_state *s) {

  assert (s->state_buffer());

  if ((Cpu::features() & FEAT_FXSR))  
    ptrace (PTRACE_SETFPXREGS, Space_context::current()->pid(), NULL, s->state_buffer());
  else
    ptrace (PTRACE_SETFPREGS,  Space_context::current()->pid(), NULL, s->state_buffer());
}

/**
 * Disable FPU. Does nothing here.
 */
IMPLEMENT inline
void
Fpu::disable()
{}

/**
 * Enable FPU. Does nothing here.
 */
IMPLEMENT inline
void
Fpu::enable()
{}
