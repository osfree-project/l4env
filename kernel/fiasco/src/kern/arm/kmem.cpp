INTERFACE [arm]:

#include "kern_types.h"
#include "kip.h"
#include "mem_layout.h"

class Kmem : public Mem_layout
{
public:

  template< typename _Ty >
  static _Ty *phys_to_virt( P_ptr<_Ty> p )
  {
    return (_Ty*)(p.get_unsigned() + (Map_base - Sdram_phys_base));
  }

  template< typename _Ty >
  static P_ptr<_Ty> virt_to_phys( _Ty *p )
  {
    return (_Ty*)((char*)p - (Map_base - Sdram_phys_base));
  }

  static Mword *kernel_sp();
  static void kernel_sp( Mword * );

  static Mword is_tcb_page_fault( Mword pfa, Mword error );
  static Mword is_kmem_page_fault( Mword pfa, Mword error );
  static Mword is_ipc_page_fault( Mword pfa, Mword error );
  static Mword is_smas_page_fault( Mword pfa );
  static Mword is_io_bitmap_page_fault( Mword pfa );

  static Mword ipc_window( unsigned num );

  static Mword *_sp;
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include "config.h"

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
  return in_kernel(pfa);
}

IMPLEMENT inline
Mword Kmem::is_tcb_page_fault(Mword pfa, Mword /*error*/ )
{
  return in_tcbs(pfa);
}

IMPLEMENT inline
Mword Kmem::is_ipc_page_fault(Mword pfa, Mword /*error*/ )
{
  return (pfa >= Kmem::Ipc_window_start) && (pfa <Kmem::Ipc_window_end);
}

IMPLEMENT inline
Mword Kmem::is_smas_page_fault( Mword /*pfa*/ )
{
  return 0;
}

IMPLEMENT inline
Mword Kmem::is_io_bitmap_page_fault( Mword /*pfa*/ )
{
  return 0;
}

IMPLEMENT inline NEEDS["config.h"]
Mword Kmem::ipc_window( unsigned num )
{
  return Ipc_window_start + num * Config::SUPERPAGE_SIZE * 2;
}

