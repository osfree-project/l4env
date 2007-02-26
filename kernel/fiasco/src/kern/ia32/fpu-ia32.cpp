/*
 * Fiasco ia32
 * Architecture specific floating point unit code
 */

IMPLEMENTATION[ia32]:

#include <cassert>
#include <flux/x86/proc_reg.h>
#include "cpu.h"
#include "regdefs.h"

IMPLEMENT
void
Fpu::init() 
{
  // Mark FPU busy, so that first FPU operation will yield an exception
  disable();

  // At first, noone owns the FPU
  set_owner (0);

  // disable Coprocessor Emulation to allow exception #7/NM on TS
  // enable Numeric Error (exception #16/MF, native FPU mode)
  // enable Monitor Coprocessor
  set_cr0 ((get_cr0() & ~CR0_EM) | CR0_NE | CR0_MP);
}
   
/*
 * Save FPU or SSE state
 */
IMPLEMENT inline NEEDS [<cassert>,"regdefs.h","cpu.h"]
void
Fpu::save_state (Fpu_state *s)
{
  assert (s->state_buffer());

  // Both fxsave and fnsave are non-waiting instructions and thus
  // cannot cause exception #16 for pending FPU exceptions.

  if ((Cpu::features() & FEAT_FXSR))
    asm volatile ("fxsave (%0)" : : "r" (s->state_buffer()) : "memory");
  else
    asm volatile ("fnsave (%0)" : : "r" (s->state_buffer()) : "memory");
}  

/*
 * Restore a saved FPU or SSE state
 */
IMPLEMENT inline NEEDS [<cassert>,"regdefs.h","cpu.h"]
void
Fpu::restore_state (Fpu_state *s)
{
  assert (s->state_buffer());

  // Only fxrstor is a non-waiting instruction and thus
  // cannot cause exception #16 for pending FPU exceptions.

  if ((Cpu::features() & FEAT_FXSR))
    asm volatile ("fxrstor (%0)" : : "r" (s->state_buffer()));

  else {

    // frstor is a waiting instruction and we must make sure no
    // FPU exceptions are pending here. We distinguish two cases:
    // 1) If we had a previous FPU owner, we called save_state before and
    //    invoked fnsave which re-initialized the FPU and cleared exceptions
    // 2) Otherwise we call fnclex instead to clear exceptions.

    if (!Fpu::owner())
      asm volatile ("fnclex");

    asm volatile ("frstor (%0)" : : "r" (s->state_buffer()));
  }
}

/*
 * Mark the FPU busy. The next attempt to use it will yield a trap.
 */
IMPLEMENT inline NEEDS ["regdefs.h",<flux/x86/proc_reg.h>]
void
Fpu::disable()
{
  set_cr0 (get_cr0() | CR0_TS);
}

/*
 * Mark the FPU no longer busy. Subsequent FPU access won't trap.
 */
IMPLEMENT inline
void
Fpu::enable()
{ 
  asm volatile ("clts");
}
