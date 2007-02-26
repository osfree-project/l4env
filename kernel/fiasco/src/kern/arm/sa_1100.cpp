INTERFACE:

#include "kmem.h"
#include "types.h"

class Sa1100 
{
public:
  enum {
    RSRR = Kmem::HW_REGS_MAP_BASE + 0x030000,

    /* interrupt controller */
    ICIP = Kmem::HW_REGS_MAP_BASE + 0x050000,
    ICMR = Kmem::HW_REGS_MAP_BASE + 0x050004,
    ICLR = Kmem::HW_REGS_MAP_BASE + 0x050008,
    ICCR = Kmem::HW_REGS_MAP_BASE + 0x05000c,
    ICFP = Kmem::HW_REGS_MAP_BASE + 0x050010,
    ICPR = Kmem::HW_REGS_MAP_BASE + 0x050020,

    /* OS Timer */
    OSMR0 = Kmem::HW_REGS_MAP_BASE + 0x000000,
    OSMR1 = Kmem::HW_REGS_MAP_BASE + 0x000004,
    OSMR2 = Kmem::HW_REGS_MAP_BASE + 0x000008,
    OSMR3 = Kmem::HW_REGS_MAP_BASE + 0x00000c,
    OSCR  = Kmem::HW_REGS_MAP_BASE + 0x000010,
    OSSR  = Kmem::HW_REGS_MAP_BASE + 0x000014,
    OWER  = Kmem::HW_REGS_MAP_BASE + 0x000018,
    OIER  = Kmem::HW_REGS_MAP_BASE + 0x00001c,

    RSRR_SWR = 1,
    
  };

  static inline void  hw_reg( Mword value, Mword reg );
  static inline Mword hw_reg( Mword reg );

};


IMPLEMENTATION:

IMPLEMENT inline
void Sa1100::hw_reg( Mword value, Mword reg )
{
  *(Mword volatile*)reg = value;
}

IMPLEMENT inline
Mword Sa1100::hw_reg( Mword reg )
{
  return *(Mword volatile*)reg;
}
//-
