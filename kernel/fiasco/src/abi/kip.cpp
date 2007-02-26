INTERFACE:

#include "types.h"

class Kernel_info
{
public:
  static void init_kip( Kernel_info *kip );
  static Kernel_info *const kip();

  void print() const;

  Mword const max_threads() const;

  // returns the 1st address beyond all available physical memory
  Address main_memory_high() const;
  
private:
  static Kernel_info *global_kip;
};


#define L4_KERNEL_INFO_MAGIC (0x4BE6344CL) /* "L4�K" */

IMPLEMENTATION:

Kernel_info *Kernel_info::global_kip;

IMPLEMENT inline
void Kernel_info::init_kip( Kernel_info *kip )
{
  global_kip = kip;
}


IMPLEMENT inline
Kernel_info *const Kernel_info::kip()
{
  return global_kip;
}
