INTERFACE:

#include "kern_types.h"

class Kmem 
{
public:
  // address space constants
  enum {

    CACHE_FLUSH_AREA     = 0xef000000,
    CACHE_FLUSH_AREA_END = 0xef002000,
    UART3_MAP_BASE       = 0xef050000,
    UART3_MAP_END        = 0xef051000,
    HW_REGS_MAP_BASE     = 0xef060000,
    HW_REGS_MAP_END      = 0xef070000,
    PHYS_MAP_BASE        = 0xf0000000,
    PHYS_MAP_END         = 0xffff0000,
    IVT_BASE             = 0xffff0000,
    


    PHYS_SDRAM_BASE   = 0xc0000000,
    PHYS_UART3_BASE   = 0x80050000,
    PHYS_HW_REGS_BASE = 0x90000000,
    PHYS_FLUSH_BASE   = 0xe0000000,

    mem_tcbs          = 0xc0000000,
    mem_tcbs_end      = 0xe0000000,
    mem_user_max      = 0xc0000000,

  };


  template< typename _Ty >
  static _Ty *phys_to_virt( P_ptr<_Ty> p )
  {
    return (_Ty*)(p.get_unsigned() + (PHYS_MAP_BASE - PHYS_SDRAM_BASE));
  }

  template< typename _Ty >
  static P_ptr<_Ty> virt_to_phys( _Ty *p )
  {
    return (_Ty*)((char*)p - (PHYS_MAP_BASE - PHYS_SDRAM_BASE));
  }

  static Mword *kernel_sp();
  static void kernel_sp( Mword * );


private:

  static Mword *_sp;


};

IMPLEMENTATION:

Mword *Kmem::_sp = 0;

IMPLEMENT inline
Mword *Kmem::kernel_sp()
{
  return _sp;
}

IMPLEMENT inline
void Kmem::kernel_sp( Mword *sp )
{
  _sp = sp;
}

