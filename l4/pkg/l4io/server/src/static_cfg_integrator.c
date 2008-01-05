
#include "static_cfg.h"

static l4io_desc_device_t
	sys = { SYS_CTRL, 1,
		{
			{ L4IO_RESOURCE_MEM, 0xc0000000, 0xc0000fff },
		}
	},
	kbd = { AMBA_KMI_KBD, 2,
		{
			{ L4IO_RESOURCE_MEM, 0x18000000, 0x18000fff },
			{ L4IO_RESOURCE_IRQ, 20, 20 }
		}
	},
	mou = { AMBA_KMI_MOUSE, 2,
		{
			{ L4IO_RESOURCE_MEM, 0x19000000, 0x19000fff },
			{ L4IO_RESOURCE_IRQ, 21, 21 }
		}
	},
	lcd = { AMBA_LCD_PL110, 1,
		{
			{ L4IO_RESOURCE_MEM, 0xc0020000, 0xc0020fff },
		}
	},
	smc91x = { SMC91X, 2,
		{
			{ L4IO_RESOURCE_MEM, 0xc8000000, 0xc8000fff },
			{ L4IO_RESOURCE_IRQ, 27, 27 },
		}
	};

register_device_group("integrator", &sys, &kbd, &mou, &lcd, &smc91x);
