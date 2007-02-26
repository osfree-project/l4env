INTERFACE[ux-!context_4k]:

EXTENSION class Mem_layout
{
public:

  /** Virtual memory layout for 2kB kernel thread context. */
  enum
  {
    Vmem_start         = 0x20000000,
    Tcbs               = 0x20000000,  ///< % 256MB
    Tcbs_end           = 0x40000000,
    Glibc_mmap_start   = 0x40000000,  ///<         fixed, Linux kernel spec.
    Glibc_mmap_end     = 0x50000000,  ///<         hoping that is enough
    Slabs_start        = 0x50000000,  ///<         multipage slabs
    Slabs_end          = 0x5f000000,
    Idt                = 0x5f001000,
    Tbuf_status_page   = 0x5f002000,  ///< % 4kB   for jdb_tbuf
    Tbuf_buffer_area   = 0x5f200000,  ///< % 2MB   tracebuffer
    Io_bitmap          = 0x5f800000,  ///< % 4MB   dummy
    Smas_io_bmap_bak   = 0x5fc00000,  ///< % 4MB   dummy
    Vmem_end           = 0x60000000,
    Physmem            = Vmem_end,    ///< % 4MB   physical memory
    Physmem_end        = 0xa0000000,
  };
};

INTERFACE[ux-context_4k]:

EXTENSION class Mem_layout
{
public:

  /** Virtual memory layout for 4kB kernel thread context. */
  enum
  {
    Vmem_start         = 0x20000000,
    Slabs_start        = 0x20000000,  ///<         multipage slabs
    Slabs_end          = 0x2f000000,
    Idt                = 0x2f001000,
    Tbuf_status_page   = 0x2f002000,  ///< % 4kB   for jdb_tbuf
    Tbuf_buffer_area   = 0x2f200000,  ///< % 2MB   tracebuffer
    Io_bitmap          = 0x2f800000,  ///< % 4MB   dummy
    Smas_io_bmap_bak   = 0x2fc00000,  ///< % 4MB   dummy
    Glibc_mmap_start   = 0x40000000,  ///<         fixed, Linux kernel spec.
    Glibc_mmap_end     = 0x50000000,  ///<         hoping that is enough
    Tcbs               = 0x50000000,  ///< % 256MB
    Tcbs_end           = 0x90000000,
    Vmem_end           = 0x90000000,
    Physmem            = Vmem_end,    ///< % 4MB   physical memory
    Physmem_end        = 0xb0000000,
  };
};

INTERFACE[ux]:

EXTENSION class Mem_layout
{
public:

  /** Virtuel memory layout -- user address space. */
  enum
  {
    User_max           = 0xc0000000,
    Utcb_ptr_page      = 0xbfff0000,  ///< % 4kB
    V2_utcb_addr       = 0xbff00000,  ///< % 4kB   v2 UTCB map address
    Trampoline_page    = 0xbfff1000,  ///< % 4kB
    Kip_auto_map       = 0xbfff2000,  ///< % 4kB
    Space_index        = 0xc0000000,  ///< % 4MB   v2
    Chief_index        = 0xc0400000,  ///< % 4MB   v2
    Kip_index          = 0xc0800000,  ///< % 4MB
    Pid_index          = 0xc0c00000,  ///< % 4MB   host pid
    Syscalls           = 0xeacff000,  ///< % 4kB   syscall page
  };

  /** Physical memory layout. */
  enum
  {
    Kernel_start_frame   = 0x1000,      // Frame 0 special-cased by rmgr
    Multiboot_frame      = 0x1000,      // Multiboot info + modules
    Trampoline_frame     = 0x2000,      // Trampoline Page
    Utcb_ptr_frame       = 0x3000,      // UTCB pointer page
    Sigstack_start_frame = 0x4000,      // Kernel Signal Altstack Start
    Sigstack_end_frame   = 0xc000,      // Kernel Signal Altstack End
    Kernel_end_frame     = Sigstack_end_frame
  };

  /// reflect symbols in linker script
  static const char task_sighandler_start  asm ("_task_sighandler_start");
  static const char task_sighandler_end    asm ("_task_sighandler_end");

  static Address const kernel_trampoline_page;
};

INTERFACE [ux-lipc]:

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



IMPLEMENTATION[ux]:

Address const Mem_layout::kernel_trampoline_page =
              phys_to_pmem (Trampoline_frame);

PUBLIC static inline
Address
Mem_layout::pmem_to_phys (Address addr)
{
  return addr - Physmem;
}

PUBLIC static inline
Address
Mem_layout::phys_to_pmem (Address addr)
{
  return addr + Physmem;
}

PUBLIC static inline
Mword
Mem_layout::in_pmem (Address addr)
{
  return (addr & 0xf0000000) == Physmem;
}
