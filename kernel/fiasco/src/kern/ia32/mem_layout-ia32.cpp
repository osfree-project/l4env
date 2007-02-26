INTERFACE [ia32]:

#include "types.h"

EXTENSION class Mem_layout
{
public:
  enum
  {
    V2_utcb_addr      = 0xbff00000,    ///< % 4kB   v2 UTCB map address
    Kip_auto_map      = 0xbfff2000,    ///< % 4kB
    User_max          = 0xc0000000,
    Vmem_start        = User_max,
    Tcbs              = 0xc0000000,    ///< % 256MB
    Tcbs_end          = 0xe0000000,    ///< % 256MB, Tcbs + 512MB
    Slabs_start       = 0xe0000000,    ///<         multipage slabs
    Slabs_end         = 0xea000000,    ///<         slabs_start + 160MB
    Space_index       = 0xea000000,    ///< % 4MB
    Chief_index       = 0xea400000,    ///< % 4MB
    Kip_index         = 0xea800000,    ///< % 4MB   lipc
    Service_page      = 0xeac00000,    ///< % 4MB   global mappings
    Local_apic_page   = Service_page + 0x0000,   ///< % 4kB
    Jdb_adapter_page  = Service_page + 0x1000,   ///< % 4kB
    Tbuf_status_page  = Service_page + 0x2000,   ///< % 4kB
    Smas_trampoline   = Service_page + 0x3000,   ///< % 4kB
    Jdb_bench_page    = Service_page + 0x4000,   ///< % 4kB
    Utcb_ptr_page     = Service_page + 0xfd000,  ///< % 4kB
    Idt               = Service_page + 0xfe000,  ///< % 4kB
    Syscalls          = Service_page + 0xff000,  ///< % 4kB syscall page
    Tbuf_buffer_area  = Service_page + 0x200000, ///< % 2MB
    Ldt_addr          = 0xeb000000,    ///< % 4MB
    Ldt_size          = 0xeb400000,    ///< % 4MB
    // 0xeb800000-0xec000000 (8MB) free
    Smas_version      = 0xec000000,    ///< % 4MB   SMAS pdir version counter
    Smas_area         = 0xec400000,    ///< % 4MB   SMAS segment base/limit
    Smas_io_bmap_bak  = 0xec800000,    ///< % 4MB   SMAS IO bitmap backup
    Smas_io_cnt_bak   = 0xec880000,    ///< % 4kB, same 4MB as Smas_io_bmap_bak
    Jdb_debug_start   = 0xecc00000,    ///< % 4MB   JDB symbols/lines
    Jdb_debug_end     = 0xee000000,    ///< % 4MB
    Ipc_window0       = 0xee000000,    ///< % 8MB
    Ipc_window1       = 0xee800000,    ///< % 8MB, Ipc_window0 + 8MB
    // 0xef000000-0xef800000 (8MB) free
    Kstatic           = 0xef800000,    ///< Io_bitmap - 4MB
    Io_bitmap         = 0xefc00000,    ///< % 4MB
    Io_counter        = 0xefc80000,    ///< % 4kB, same 4MB page as Io_bitmap
    Vmem_end          = 0xf0000000,
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

INTERFACE [ia32-lipc]:

#include "types.h"

EXTENSION class Mem_layout
{
public:

  /* LIPC code area start */
  static const char asm_lipc_code_start    asm ("_asm_lipc_code_start");

  /* LIPC code area end */
  static const char asm_lipc_code_end      asm ("_asm_lipc_code_end");

  /* dummy IRET code*/
  static const char asm_user_invoke_from_localipc 
  asm ("_asm_user_invoke_from_localipc");

  /* restart point */
  static const char lipc_restart_point_offset 
  asm ("_lipc_restart_point_offset");

  /* forward point */
  static const char lipc_forward_point_offset
  asm ("_lipc_forward_point_offset");

  /* see LIPC code */
  static const char lipc_rcv_desc_invalid
  asm ("_lipc_rcv_desc_invalid");

  /* end point */
  static const char lipc_finish_point_offset
  asm ("_lipc_finish_point_offset");

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
