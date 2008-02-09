INTERFACE [arm && !kern_start_0xd]:

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_umax {
    User_max             = 0xc0000000,
    Tcbs                 = 0xc0000000,  ///< % 256MB
  };
};


//---------------------------------------------------------------------------
INTERFACE [arm && kern_start_0xd]:

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_umax {
    User_max             = 0xd0000000,
    Tcbs                 = 0xd0000000,  ///< % 256MB
  };
};


//---------------------------------------------------------------------------
INTERFACE [arm]:

EXTENSION class Mem_layout
{
public:
  enum Virt_layout {
    V2_utcb_addr	 = User_max - 0x10000,
    Tcbs_end             = 0xe0000000,
    Slabs_start          = 0xe0000000,
    Slabs_end            = 0xea000000,
    Service_page         = 0xeac00000,
    Tbuf_status_page     = Service_page + 0x5000,
    Tbuf_ustatus_page    = Tbuf_status_page,
    Tbuf_buffer_area	 = Service_page + 0x200000,
    Tbuf_ubuffer_area    = Tbuf_buffer_area,
    Jdb_tmp_map_area     = Service_page + 0x400000,
    Ipc_window_start     = 0xee000000,
    Ipc_window_end       = 0xef000000,
    Cache_flush_area     = 0xef000000,
    Cache_flush_area_end = 0xef100000,
    Registers_map_start  = 0xef100000,
    Registers_map_end    = 0xef400000,
    Space_index		 = 0xeff00000,
    Map_base             = 0xf0000000,
    Map_end              = 0xffff0000,
    Utcb_ptr_page        = 0xffffd000,
    Kern_lib_base	 = 0xffffe000,
    Ivt_base             = 0xffff0000,
    Syscalls		 = 0xfffff000,

    Kernel_max           = 0x00000000,
  };
};


// -------------------------------------------------------------------------

IMPLEMENTATION [arm && (sa1100 || pxa)]:

PUBLIC static inline
bool
Mem_layout::is_physical_memory(Address addr)
{
  return addr >= Sdram_phys_base
         && addr - Sdram_phys_base < Map_end - Map_base;
}

// ------------------------------------------------------------------------

IMPLEMENTATION [arm && (integrator || realview)]:

#include <cassert>

// Separate version because of Sdram_phys_base == 0 and warnings
PUBLIC static inline NEEDS[<cassert>]
bool
Mem_layout::is_physical_memory(Address addr)
{
  assert(!Sdram_phys_base);
  return addr < Map_end - Map_base;
}

// ------------------------------------------------------------------------
IMPLEMENTATION [arm-!noncont_mem]:


PUBLIC static inline
Address
Mem_layout::phys_to_pmem (Address addr)
{
  if (!is_physical_memory(addr))
    return (Address)-1;

  return addr + Map_base - Sdram_phys_base;
}

PUBLIC static inline
Address
Mem_layout::pmem_to_phys (Address addr)
{
  if (addr < Map_base || addr >= Map_end)
    return (Address)-1;

  return addr - Map_base + Sdram_phys_base;
}

IMPLEMENTATION [arm]:

#include "kip.h"
PUBLIC static inline NEEDS["kip.h"]
Address
Mem_layout::sdram_phys_end()
{
  return Sdram_phys_base + Kip::k()->main_memory_high();
}
