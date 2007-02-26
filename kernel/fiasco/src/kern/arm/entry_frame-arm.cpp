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
	Unsigned32 r[13];
	void dump();
};

EXTENSION class Return_frame
{
public: 
//protected:
	Unsigned32 psr;
  Unsigned32 sp;
  Unsigned32 ulr;
  Unsigned32 km_lr;
  Unsigned32 pc;
};


IMPLEMENTATION[arm]:

#include <cstdio>

IMPLEMENT //inline 
void Syscall_frame::dump()
{
#if 1
	for(int i = 0; i < 15; i+=4 )
		printf("R[%2d]: %08x R[%2d]: %08x R[%2d]: %08x R[%2d]: %08x\n",
					 i,r[i],i+1,r[i+1],i+2,r[i+2],i+3,r[i+3]);
#endif
}

IMPLEMENT inline
Mword Entry_frame::pc() const
{
  return Return_frame::pc; /*r[15];*/
}

IMPLEMENT inline
void Entry_frame::pc( Mword _pc )
{
  Return_frame::pc /*r[15]*/ = _pc;
}

IMPLEMENT inline
Mword Entry_frame::sp() const
{
  return Return_frame::sp;
}

IMPLEMENT inline
void Entry_frame::sp( Mword _sp )
{
  Return_frame::sp = _sp;
}

