/*
 * Fiasco Kernel-Entry Frame-Layout Code
 * Shared between UX and native IA32.
 */

INTERFACE:

#include "types.h"


EXTENSION class Syscall_frame
{
protected:
  Unsigned32 r[16];
};


IMPLEMENTATION[arm]:

IMPLEMENT inline
Mword Entry_frame::pc() const
{
  return r[15];
}

IMPLEMENT inline
void Entry_frame::pc( Mword _pc )
{
  r[15] = _pc;
}

IMPLEMENT inline
Mword Entry_frame::sp() const
{
  return r[13];
}

IMPLEMENT inline
void Entry_frame::sp( Mword _sp )
{
  r[13] = _sp;
}
