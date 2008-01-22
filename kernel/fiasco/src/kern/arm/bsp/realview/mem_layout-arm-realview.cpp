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
    Mpcore_scu_map_base  = Registers_map_start + 0x00100000,
    Gic_cpu_map_base     = Mpcore_scu_map_base + 0x00000100,
    Gic_dist_map_base    = Mpcore_scu_map_base + 0x00001000,
  };

  enum Phys_layout_realview_mpcore {
    Mpcore_scu_phys_base = 0x1f000000,
    Gic_cpu_phys_base    = Mpcore_scu_phys_base + 0x00000100,
    Gic_dist_phys_base   = Mpcore_scu_phys_base + 0x00001000,
  };
};


