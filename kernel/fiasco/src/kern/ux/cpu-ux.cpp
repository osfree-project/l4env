/*
 * Fiasco-UX
 * Architecture specific cpu init code
 */

IMPLEMENTATION[ux]:

#include "initcalls.h"
#include "regdefs.h"

IMPLEMENT FIASCO_INIT
void
Cpu::init (void)
{
  identify();

  // No Sysenter Support for Fiasco-UX
  cpu_features &= ~FEAT_SEP;
}
