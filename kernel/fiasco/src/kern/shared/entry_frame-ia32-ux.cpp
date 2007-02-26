/*
 * Fiasco Kernel-Entry Frame-Layout Code
 * Shared between UX and native IA32.
 */

INTERFACE:

#include "types.h"

EXTENSION class Syscall_frame
{
public:
  //protected:
  Unsigned32 ecx;
  Unsigned32 edx;
  Unsigned32 esi;
  Unsigned32 edi;
  Unsigned32 ebx;
  Unsigned32 ebp;
  Unsigned32 eax;
};

EXTENSION class Return_frame
{
public:
  Unsigned32 eip;
  Unsigned16 cs, __csu;
  Unsigned32 eflags;
  Unsigned32 esp;
  Unsigned16 ss, __ssu;

};


IMPLEMENTATION[ia32-ux]:

//-

