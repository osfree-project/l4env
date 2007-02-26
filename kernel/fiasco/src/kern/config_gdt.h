#ifndef CONFIG_GDT_H
#define CONFIG_GDT_H

#define GDT_TSS (0x08)
#define GDT_CODE_KERNEL (0x10)
#define GDT_DATA_KERNEL (0x18)
#define GDT_CODE_USER (0x20)
#define GDT_DATA_USER (0x28)
#define GDT_MAX (0x30)

#endif
