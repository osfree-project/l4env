INTERFACE:

#include "types.h"

class Mem_layout
{
public:
  /// reflect symbols in linker script
  static const char load            asm ("_load");
  static const char image_start	    asm ("_kernel_image_start");
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

IMPLEMENTATION:

#include "config.h"
#include "l4_types.h"

PUBLIC static inline
Address
Mem_layout::user_utcb_ptr() 
{ return *reinterpret_cast<Address*>(Utcb_ptr_page); }

PUBLIC static inline
void
Mem_layout::user_utcb_ptr(Address utcb) 
{ *reinterpret_cast<Address*>(Utcb_ptr_page) = utcb; }

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

PUBLIC static inline NEEDS ["l4_types.h", "config.h"]
unsigned
Mem_layout::max_threads()
{
  unsigned abilimit = L4_uid::max_threads();
  unsigned memlimit = (Tcbs_end - Tcbs) / Config::thread_block_size;

  return abilimit < memlimit ? abilimit : memlimit;
}
