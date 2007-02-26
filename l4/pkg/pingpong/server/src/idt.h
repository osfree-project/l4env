#ifndef __IDT_H
#define __IDT_H

typedef struct
{
  l4_uint32_t  a,b;
} __attribute__ ((packed)) desc_t;

typedef struct
{
  l4_uint16_t limit;
  void        *base;
  desc_t      desc[0x20];
} __attribute__ ((packed)) idt_t;

#define MAKE_IDT_DESC(idt, nr, address)                                \
  do {                                                                 \
    idt.desc[nr].a = (l4_uint32_t)address & 0x0000ffff;                \
    idt.desc[nr].b = 0x0000ef00 | ((l4_uint32_t)address & 0xffff0000); \
  } while (0)

#endif
