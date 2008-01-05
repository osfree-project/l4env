
#include "static_cfg.h"

// single core EB
static l4io_desc_device_t
  ctrl = { SYS_CTRL, 1,
           {
             { L4IO_RESOURCE_MEM, 0x10000000, 0x10000fff },
           }
  },
  lcd = { AMBA_LCD_PL110, 1,
           {
             { L4IO_RESOURCE_MEM, 0x10020000, 0x10020fff },
           }
  },
  kbd = { AMBA_KMI_KBD, 2,
           {
              { L4IO_RESOURCE_MEM, 0x10006000, 0x10006fff },
              { L4IO_RESOURCE_IRQ, 52, 52 }
           }
  },
  mouse = { AMBA_KMI_MOUSE, 2,
           {
             { L4IO_RESOURCE_MEM, 0x10007000, 0x10007fff },
             { L4IO_RESOURCE_IRQ, 53, 53 }
           }
  },
  smc91x = { SMC91X, 2,
           {
             { L4IO_RESOURCE_MEM, 0x4e000000, 0x4e000fff },
             { L4IO_RESOURCE_IRQ, 60, 60 }
           }
  };

register_device_group("rv-926", &ctrl, &lcd, &kbd, &mouse, &smc91x);
