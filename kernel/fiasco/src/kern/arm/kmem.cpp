INTERFACE:

#include "kern_types.h"
#include "kip.h"

class Kmem 
{
public:
  // address space constants
  enum {

    CACHE_FLUSH_AREA     = 0xef000000,
    CACHE_FLUSH_AREA_END = 0xef002000,
    UART3_MAP_BASE       = 0xef050000,
    UART3_MAP_END        = 0xef051000,
    HW_REGS_MAP_BASE     = 0xef100000,
    HW_REGS_MAP_END      = 0xef200000,
    PHYS_MAP_BASE        = 0xf0000000,
    PHYS_MAP_END         = 0xffff0000,
    IVT_BASE             = 0xffff0000,
    


    PHYS_SDRAM_BASE   = 0xc0000000,
    PHYS_UART3_BASE   = 0x80050000,
    PHYS_HW_REGS_BASE = 0x90000000,
    PHYS_FLUSH_BASE   = 0xe0000000,

    mem_tcbs          = 0xd0000000,
    mem_tcbs_end      = 0xe0000000,
    mem_user_max      = 0xd0000000,
    mem_kernel_max    = 0x00000000,
    ipc_window_start  = 0xee000000,
    ipc_window_end    = 0xef000000,

    _mappings_1_addr     = 0xe0000000, // slab area (MUST BE RENAMED)
    _mappings_end_1_addr = 0xea000000, // ---------------------------
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

  static Mword is_kmem_page_fault( Mword pfa, Mword error );
  static Mword is_tcb_page_fault( Mword pfa, Mword error );
  static Mword is_ipc_page_fault( Mword pfa, Mword error );
  static Mword is_smas_page_fault( Mword pfa, Mword error );
  static Mword is_io_bitmap_page_fault( Mword pfa, Mword error );

  static Mword ipc_window( unsigned num );

  static Kernel_info *info(); 

private:

  static Mword *_sp;


};

IMPLEMENTATION:

#include "config.h"
#include "kip_init.h"

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


IMPLEMENT inline
Mword Kmem::is_kmem_page_fault( Mword pfa, Mword /*error*/ )
{
  return pfa >= Kmem::mem_user_max;
}

IMPLEMENT inline
Mword Kmem::is_tcb_page_fault(Mword pfa, Mword /*error*/ )
{
  return (pfa >=Kmem::mem_tcbs) && (pfa <Kmem::mem_tcbs_end);
}

IMPLEMENT inline
Mword Kmem::is_ipc_page_fault(Mword pfa, Mword /*error*/ )
{
  return (pfa >= Kmem::ipc_window_start) && (pfa <Kmem::ipc_window_end);
}

IMPLEMENT inline
Mword Kmem::is_smas_page_fault( Mword /*pfa*/, Mword /*error*/ )
{
  return 0;
}

IMPLEMENT inline
Mword Kmem::is_io_bitmap_page_fault( Mword /*pfa*/, Mword /*error*/ )
{
  return 0;
}

IMPLEMENT
Kernel_info *Kmem::info()
{
  return Kip::kip();
}

IMPLEMENT inline NEEDS["config.h"]
Mword Kmem::ipc_window( unsigned num )
{
  return ipc_window_start + num * Config::SUPERPAGE_SIZE;
}
