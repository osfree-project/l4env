
INTERFACE:

#include "l4_types.h"

class Tss
{
public:
  Unsigned32  back_link;
  Unsigned32  esp0;
  Unsigned32  ss0;
  Unsigned32  esp1;
  Unsigned32  ss1;
  Unsigned32  esp2;
  Unsigned32  ss2;
  Unsigned32  cr3;
  Unsigned32  eip;
  Unsigned32  eflags;
  Unsigned32  eax;
  Unsigned32  ecx;
  Unsigned32  edx;
  Unsigned32  ebx;
  Unsigned32  esp;
  Unsigned32  ebp;
  Unsigned32  esi;
  Unsigned32  edi;
  Unsigned32  es;
  Unsigned32  cs;
  Unsigned32  ss;
  Unsigned32  ds;
  Unsigned32  fs;
  Unsigned32  gs;
  Unsigned32  ldt;
  Unsigned16  trace_trap        __attribute__((packed));
  Unsigned16  io_bit_map_offset __attribute__((packed));
};

IMPLEMENTATION:

//-
