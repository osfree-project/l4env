INTERFACE:

#include "types.h"

class Mem_layout
{
public:
  /// reflect symbols in linker script
  static const char load            asm ("_load");
  static const char start           asm ("_start");
  static const char end             asm ("_end");
  static const char ecode           asm ("_ecode");
  static const char etext           asm ("_etext");
  static const char edata           asm ("_edata");
  static const char initcall_start  asm ("_initcall_start");
  static const char initcall_end    asm ("_initcall_end");

  static Mword in_tcbs (Address a);
  static Mword in_kernel (Address a);
};

INTERFACE [utcb]:

EXTENSION class Mem_layout
{
public:
  static const char _v2_utcb_start asm ("_v2_utcb_start");
};


IMPLEMENTATION:

IMPLEMENT inline
Mword
Mem_layout::in_tcbs (Address a)
{
  return a >= Tcbs && a < Tcbs_end;
}

IMPLEMENT inline
Mword
Mem_layout::in_kernel (Address a)
{
  return a >= User_max;
}

PUBLIC static inline
Mword
Mem_layout::in_kernel_code (Address a)
{
  return a >= (Address)&start && a < (Address)&ecode;
}
