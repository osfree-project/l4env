// ---------------------------------------------------------------------
INTERFACE [arm-integrator]:

#include "kmem.h"

EXTENSION class Pic
{
public:
  enum
  {
    Multi_irq_pending = 1,
    No_irq_pending = 0,
  };

private:
  enum
  {
    IRQ_STATUS       = Kmem::Pic_map_base + 0x00,
    IRQ_ENABLE_SET   = Kmem::Pic_map_base + 0x08,
    IRQ_ENABLE_CLEAR = Kmem::Pic_map_base + 0x0c,

    FIQ_ENABLE_CLEAR = Kmem::Pic_map_base + 0x2c,

    PIC_START = 0,
    PIC_END   = 31,
  };
};

// ---------------------------------------------------------------------
IMPLEMENTATION [arm && integrator]:

#include "boot_info.h"
#include "config.h"
#include "initcalls.h"
#include "io.h"

IMPLEMENT FIASCO_INIT
void Pic::init()
{
  Io::write(0xffffffff, IRQ_ENABLE_CLEAR);
  Io::write(0xffffffff, FIQ_ENABLE_CLEAR);
}

IMPLEMENT inline NEEDS["io.h"]
void Pic::disable_locked( unsigned irq )
{
  irq -= PIC_START;
  Io::write(1 << irq, IRQ_ENABLE_CLEAR);
}

IMPLEMENT inline NEEDS["io.h"]
void Pic::enable_locked(unsigned irq, unsigned /*prio*/)
{
  irq -= PIC_START;
  Io::write(1 << irq, IRQ_ENABLE_SET);
}

IMPLEMENT inline
void Pic::acknowledge_locked(unsigned)
{}


IMPLEMENT inline
Pic::Status Pic::disable_all_save()
{
  Status s = 0;
  return s;
}

IMPLEMENT inline
void Pic::restore_all( Status /*s*/ )
{
}

PUBLIC static inline NEEDS["io.h"]
Unsigned32 Pic::pending()
{
  return Io::read<Mword>(IRQ_STATUS);
}

PUBLIC static inline
Mword Pic::is_pending(Mword &irqs, Mword irq)
{
  Mword ret = irqs & (1 << irq);
  irqs &= ~(1 << irq);
  return ret;
}

IMPLEMENT inline NEEDS [Pic::disable_locked]
void Pic::block_locked(unsigned irq)
{
  disable_locked(irq);
}

