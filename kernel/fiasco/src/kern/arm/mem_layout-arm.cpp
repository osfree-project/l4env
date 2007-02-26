INTERFACE [arm]: //----------------------------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout {
    User_max             = 0xd0000000,
    Tcbs                 = 0xd0000000,  ///< % 256MB
    Tcbs_end             = 0xe0000000,
    Slabs_start          = 0xe0000000,
    Slabs_end            = 0xea000000,
    Ipc_window_start     = 0xee000000,
    Ipc_window_end       = 0xef000000,
    Cache_flush_area     = 0xef000000,
    Cache_flush_area_end = 0xef100000,
    Registers_map_start  = 0xef100000,
    Registers_map_end    = 0xef400000,
    Map_base             = 0xf0000000,
    Map_end              = 0xffff0000,
    Kern_lib_base	 = 0xffffe000,
    Ivt_base             = 0xffff0000,
    
    Kernel_max           = 0x00000000,
  };
};
 

INTERFACE [arm-sa1100]: //---------------------------------------------------

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



IMPLEMENTATION [arm]:

PUBLIC static inline
Address
Mem_layout::phys_to_pmem (Address addr)
{
  if (addr < Sdram_phys_base)
    return (Address)-1;
  
  if (addr - Sdram_phys_base  >= Map_end - Map_base)
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

