/*
 * Fiasco-UX
 * Architecture specific IDT code
 */

IMPLEMENTATION[ux]:

#include <flux/x86/seg.h>
#include "emulation.h"

IMPLEMENT
void
Idt::load_idt (pseudo_descriptor *desc)
{
  Emulation::lidt (desc);
}
