INTERFACE [arm && !sa1100]:

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_umax {
    User_max             = 0xc0000000,
    Tcbs                 = 0xc0000000,  ///< % 256MB
  };
};


//---------------------------------------------------------------------------
INTERFACE [arm && sa1100]:

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


//---------------------------------------------------------------------------
INTERFACE [arm-sa1100]:

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_sa1100 {
    Uart_map_base        = 0xef100000,
    Timer_map_base       = 0xef200000,
    Pic_map_base         = 0xef250000,
    Uart_base            = Uart_map_base + 0x50000,
  };

  enum Phys_layout {
    Uart_phys_base       = 0x80050000,
    Timer_phys_base      = 0x90000000,
    Pic_phys_base        = 0x90050000,
    Sdram_phys_base      = 0xc0000000,
    Flush_area_phys_base = 0xe0000000,
  };
};


INTERFACE [arm-pxa]: //------------------------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_pxa {
    Timer_map_base       = 0xef100000,
    Pic_map_base         = 0xef200000,
    Uart_map_base        = 0xef300000,
    Uart_base            = Uart_map_base / 4,
  };

  enum Phys_layout {
    Timer_phys_base      = 0x40a00000,
    Pic_phys_base        = 0x40d00000,
    Uart_phys_base       = 0x40100000,
    Sdram_phys_base      = 0xa0000000,
    Flush_area_phys_base = 0xe0000000,
  };
};

INTERFACE [arm-integrator]: //----------------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_integrator {
    Uart_map_base        = 0xef100000,
    Timer_map_base       = 0xef200000,
    Pic_map_base         = 0xef300000,
    Integrator_map_base  = 0xef400000,
    Uart_base            = Uart_map_base,
  };

  enum Phys_layout {
    Uart_phys_base       = 0x16000000,
    Timer_phys_base      = 0x13000000,
    Pic_phys_base        = 0x14000000,
    Integrator_phys_base = 0x10000000,
    Sdram_phys_base      = 0x00000000, // keep at 0, see is_physical_memory()
    Flush_area_phys_base = 0xe0000000,
  };
};

INTERFACE [arm-realview]: //----------------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_realview {
    Devices_map_base     = Registers_map_start,
    System_regs_map_base = Devices_map_base,
    System_ctrl_map_base = Devices_map_base + 0x00001000,
    Uart0_map_base       = Devices_map_base + 0x00009000,
    Uart1_map_base       = Devices_map_base + 0x0000a000,
    Uart2_map_base       = Devices_map_base + 0x0000b000,
    Uart3_map_base       = Devices_map_base + 0x0000c000,
    Timer0_map_base      = Devices_map_base + 0x00011000,
    Timer1_map_base      = Devices_map_base + 0x00011020,
    Timer2_map_base      = Devices_map_base + 0x00012000,
    Timer3_map_base      = Devices_map_base + 0x00012020,
    Uart_base            = Uart0_map_base,

  };

  enum Phys_layout_realview {
    Devices_phys_base    = 0x10000000,
    System_regs_phys_base= Devices_phys_base,
    System_ctrl_phys_base= Devices_phys_base + 0x00001000,
    Uart0_phys_base      = Devices_phys_base + 0x00009000,
    Timer0_1_phys_base   = Devices_phys_base + 0x00011000,
    Timer2_3_phys_base   = Devices_phys_base + 0x00012000,
    Sdram_phys_base      = 0x00000000, // keep at 0, see is_physical_memory()

    Flush_area_phys_base = 0xe0000000,
  };
};

INTERFACE [arm && realview && !mpcore]: //--------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_realview_single {
    Gic_cpu_map_base     = Devices_map_base + 0x00040000,
    Gic_dist_map_base    = Gic_cpu_map_base + 0x00001000,
  };

  enum Phys_layout_realview_single {
    Gic_cpu_phys_base    = Devices_phys_base + 0x00040000,
    Gic_dist_phys_base   = Gic_cpu_phys_base + 0x00001000,
  };
};

INTERFACE [arm && realview && mpcore]: //----------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_realview_mpcore {
    Mpcore_scu_map_base  = Registers_map_start + 0x0100000,
    Gic_cpu_map_base     = Mpcore_scu_map_base + 0x0000100,
    Gic_dist_map_base    = Mpcore_scu_map_base + 0x0001000,
  };

  enum Phys_layout_realview_mpcore {
    Mpcore_scu_phys_base = 0x1f000000,
    Gic_cpu_phys_base    = Mpcore_scu_phys_base + 0x0100,
    Gic_dist_phys_base   = Mpcore_scu_phys_base + 0x1000,
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
