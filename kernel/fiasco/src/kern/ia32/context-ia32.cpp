IMPLEMENTATION[ia32]:

#include "fpu.h"

/**
 * When switching away from the FPU owner, disable the FPU to cause
 * the next FPU access to trap.
 * When switching back to the FPU owner, enable the FPU so we don't
 * get an FPU trap on FPU access.
 */
IMPLEMENT inline NEEDS ["fpu.h"]
void
Context::switch_fpu (Context *t)
{
  if (Fpu::is_owner(this))
    Fpu::disable();
  
  else if (Fpu::is_owner(t))
    Fpu::enable();
}
