INTERFACE [ia32]:

#include "types.h"

EXTENSION class Mem_layout
{
public:
  enum
  {
    V2_utcb_addr      = 0xbff00000,    ///< % 4KB   v2 UTCB map address
    Kip_auto_map      = 0xbfff2000,    ///< % 4KB
    User_max          = 0xc0000000,
    Tcbs              = 0xc0000000,    ///< % 256MB
    Tcbs_end          = 0xe0000000,    ///< % 256MB, Tcbs + 512MB
    Slabs_start       = 0xe0000000,    ///<         multipage slabs
    Slabs_end         = 0xea000000,    ///<         slabs_start + 160MB
    Space_index       = 0xea000000,    ///< % 4MB
    Service_page      = 0xeac00000,    ///< % 4MB   global mappings
    Local_apic_page   = Service_page + 0x0000,   ///< % 4KB
    Kmem_tmp_page_1   = Service_page + 0x1000,   ///< % 4KB size 8KB
    Kmem_tmp_page_2   = Service_page + 0x3000,   ///< % 4KB size 8KB
    Tbuf_status_page  = Service_page + 0x5000,   ///< % 4KB
    Tbuf_ustatus_page = Tbuf_status_page,
    Smas_trampoline   = Service_page + 0x6000,   ///< % 4KB
    Jdb_bench_page    = Service_page + 0x7000,   ///< % 4KB
    Jdb_bts_area      = Service_page + 0xf000,   ///< % 4KB size 0x81000
    Utcb_ptr_page     = Service_page + 0xfd000,  ///< % 4KB
    Idt               = Service_page + 0xfe000,  ///< % 4KB
    Syscalls          = Service_page + 0xff000,  ///< % 4KB syscall page
    Tbuf_buffer_area  = Service_page + 0x200000, ///< % 2MB
    Tbuf_ubuffer_area = Tbuf_buffer_area,
    Ldt_addr          = 0xeb000000,    ///< % 4MB
    Ldt_size          = 0xeb400000,    ///< % 4MB
    // 0xeb800000-0xec000000 (8MB) free
    Smas_version      = 0xec000000,    ///< % 4MB   SMAS pdir version counter
    Smas_area         = 0xec400000,    ///< % 4MB   SMAS segment base/limit
    Smas_io_bmap_bak  = 0xec800000,    ///< % 4MB   SMAS IO bitmap backup
    Smas_io_cnt_bak   = 0xec880000,    ///< % 4KB, same 4MB as Smas_io_bmap_bak
    Jdb_debug_start   = 0xecc00000,    ///< % 4MB   JDB symbols/lines
    Jdb_debug_end     = 0xee000000,    ///< % 4MB
    Ipc_window0       = 0xee000000,    ///< % 8MB
    Ipc_window1       = 0xee800000,    ///< % 8MB, Ipc_window0 + 8MB
    // 0xef000000-0xef800000 (8MB) free
    Kstatic           = 0xef800000,    ///< Io_bitmap - 4MB
    Io_bitmap         = 0xefc00000,    ///< % 4MB
    Io_counter        = 0xefc80000,    ///< % 4KB, same 4MB page as Io_bitmap
    Vmem_end          = 0xf0000000,
    Adap_vram_mda_beg = 0xf00b0000,    ///< % 8KB video RAM MDA memory
    Adap_vram_mda_end = 0xf00b8000,
    Adap_vram_cga_beg = 0xf00b8000,    ///< % 8KB video RAM CGA memory
    Adap_vram_cga_end = 0xf00c0000,
    Adap_vidbios1_beg = 0xf00c0000,    ///< % 8KB video BIOS memory
    Adap_vidbios1_end = 0xf00c8000,
    Adap_vidbios2_beg = 0xf00e4000,    ///< % another 8KB video BIOS memory
    Adap_vidbios2_end = 0xf00ec000,
    Kernel_image      = Vmem_end,
    Boot_state_start  = Kernel_image,
    Boot_state_end    = Kernel_image + 0x400000,
    Smas_start        = 0xf0400000,    ///< % 4MB   SMAS spaces
    Smas_end          = 0xfc400000,    ///< % 4MB
    Physmem           = 0xfc400000,    ///< % 4MB   kernel memory
  };

  template < typename T > static T* boot_data (T const *addr);

private:
  static Address physmem_offs asm ("PHYSMEM_OFFS");
};

IMPLEMENTATION [ia32]:

#include <cassert>

Address Mem_layout::physmem_offs;


IMPLEMENT inline NEEDS[<cassert>]
template < typename T > T*
Mem_layout::boot_data (T const *addr)
{
  // boot data are located in the first 4MB page
  assert ((Address)addr < 4<<20);
  return (T*) ((Address)addr + Boot_state_start);
}

PUBLIC static inline NEEDS[<cassert>]
Address
Mem_layout::boot_data (Address addr)
{
  assert (addr < 4<<20);
  return addr + Boot_state_start;
}

PUBLIC static inline
void
Mem_layout::kphys_base (Address base)
{
  physmem_offs = (Address)Physmem - base;
}

PUBLIC static inline NEEDS[<cassert>]
Address
Mem_layout::pmem_to_phys (Address addr)
{
  assert (in_pmem(addr));
  return addr - physmem_offs;
}

PUBLIC static inline NEEDS[<cassert>]
Address
Mem_layout::pmem_to_phys (const void *ptr)
{
  Address addr = reinterpret_cast<Address>(ptr);

  assert (in_pmem(addr));
  return addr - physmem_offs;
}

PUBLIC static inline
Address
Mem_layout::phys_to_pmem (Address addr)
{
  return addr + physmem_offs;
}

PUBLIC static inline
Mword
Mem_layout::in_boot_state (Address addr)
{
  return addr >= Boot_state_start && addr < Boot_state_end;
}

PUBLIC static inline
Mword
Mem_layout::in_pmem (Address addr)
{
  return addr >= Physmem;
}
