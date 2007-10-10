INTERFACE [amd64]:

#include "types.h"

EXTENSION class Mem_layout
{
public:
  enum
  {
    V2_utcb_addr      = 0xffffffff8ff00000UL,    ///< % 4kB v2 UTCB map address
    Kip_auto_map      = 0xffffffff8fff2000UL,    ///< % 4kB
    User_max          = 0xffffffffa0000000UL,
    Tcbs              = 0xffffffffa0000000UL,    ///< % 1024MB
    Tcbs_end          = 0xffffffffe0000000UL,    ///< % Tcbs + 1024MB
    Slabs_start       = 0xffffffffe0000000UL,    ///<       multipage slabs
    Slabs_end         = 0xffffffffea000000UL,    ///<       slabs_start + 160MB
    Space_index		= 200,
    Service_page      = 0xffffffffeac00000UL,    ///< % 4MB global mappings
    Local_apic_page   = Service_page + 0x0000,   ///< % 4KB
    Kmem_tmp_page_1   = Service_page + 0x1000,   ///< % 4KB size 8KB
    Kmem_tmp_page_2   = Service_page + 0x3000,   ///< % 4KB size 8KB
    Tbuf_status_page  = Service_page + 0x5000,   ///< % 4KB
    Tbuf_ustatus_page = Tbuf_status_page,
    Smas_trampoline   = Service_page + 0x6000,   ///< % 4KB
    Jdb_bench_page    = Service_page + 0x7000,   ///< % 4KB
    Utcb_ptr_page     = Service_page + 0xfd000,  ///< % 4KB
    Idt               = Service_page + 0xfe000,  ///< % 4KB
    Syscalls          = Service_page + 0xff000,  ///< % 4KB syscall page
    Tbuf_buffer_area  = Service_page + 0x200000, ///< % 2MB
    Tbuf_ubuffer_area = Tbuf_buffer_area,
    // 0xffffffffeb800000-0xfffffffffec000000 (8MB) free
    Smas_version      = 0xffffffffec000000UL, ///< % 4MB SMAS pdir version counter
    Smas_area         = 0xffffffffec400000UL, ///< % 4MB SMAS segment base/limit
    Smas_io_bmap_bak  = 0xffffffffec800000UL, ///< % 4MB SMAS IO bitmap backup
    Smas_io_cnt_bak   = 0xffffffffec880000UL, ///< % 4KB, same 4MB as Smas_io_bmap_bak
    Jdb_debug_start   = 0xffffffffecc00000UL,    ///< % 4MB   JDB symbols/lines
    Jdb_debug_end     = 0xffffffffee000000UL,    ///< % 4MB
    Ipc_window0       = 0xffffffffee000000UL,    ///< % 8MB
    Ipc_window1       = 0xffffffffee800000UL,    ///< % 8MB, Ipc_window0 + 8MB
    // 0xffffffffef000000-0xffffffffef800000 (8MB) free
    Kstatic           = 0xffffffffef800000UL,    ///< % 4MB Io_bitmap
    Io_bitmap         = 0xffffffffefc00000UL,    ///< % 4MB
    Vmem_end          = 0xfffffffff0000000UL,
    Adap_vram_mda_beg = 0xfffffffff00b0000UL,	 ///< % 8KB video RAM MDA
    Adap_vram_mda_end = 0xfffffffff00b8000UL,
    Adap_vram_cga_beg = 0xfffffffff00b8000UL,	 ///< % 8KB video RAM CGA
    Adap_vram_cga_end = 0xfffffffff00c0000UL,
    Adap_vidbios1_beg = 0xfffffffff00c0000UL,	 ///< % 8KB video BIOS
    Adap_vidbios1_end = 0xfffffffff00c8000UL,
    Adap_vidbios2_beg = 0xfffffffff00e4000UL,	 ///< % another 8KB video BIOS
    Adap_vidbios2_end = 0xfffffffff00ec000UL,
    Kernel_image      = Vmem_end,
    Boot_state_start  = Kernel_image,
    Boot_state_end    = Kernel_image + 0x400000,
    Smas_start        = 0xfffffffff0400000UL,    ///< % 4MB   SMAS spaces
    Smas_end          = 0xfffffffffc400000UL,    ///< % 4MB
    Physmem           = 0xfffffffffc400000UL,    ///< % 4MB   kernel memory
    Kernel_end	      = 0xffffffffffffffffUL+1,    ///< % end of address space
  };

  template < typename T > static T* boot_data (T const *addr);

private:
  static Address physmem_offs asm ("PHYSMEM_OFFS");
};

IMPLEMENTATION [amd64]:

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
