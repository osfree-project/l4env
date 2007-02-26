
INTERFACE:

#include "l4_types.h"

class Trap_state
{
public:
//  static int (*base_handler)(Trap_state *) asm ("BASE_TRAP_HANDLER");

  Unsigned32 pf_address;
  Unsigned32 error_code;

  Unsigned32 r[13];
  
  Unsigned32 cpsr;
  Unsigned32 usp;
  Unsigned32 ulr;
  Unsigned32 reserved_klr;
  Unsigned32 pc;
};

IMPLEMENTATION:

#include <cstdio>

PUBLIC
void
Trap_state::dump()
{
  char const *excpts[] = 
    {"reset","undefined insn","swi","prefetch abort","data abort","%&#","%&#",
    "%&#"};
  
  printf("EXCEPTION: %s pfa=%08x, error=%08x\n", 
         excpts[(error_code & 0x00700000) >> 20],pf_address, error_code);

  printf("R[0]: %08x %08x %08x %08x  %08x %08x %08x %08x\n"
         "R[8]: %08x %08x %08x %08x  %08x %08x %08x %08x\n",
	 r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], 
	 r[8], r[9], r[10], r[11], r[12], usp, ulr, pc);
}

