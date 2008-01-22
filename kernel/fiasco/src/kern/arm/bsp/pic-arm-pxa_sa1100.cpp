INTERFACE [arm-pxa]: // ----------------------------------------

#include "kmem.h"

EXTENSION class Pic
{
public:
  enum
  {
    Multi_irq_pending = 1,
  };

private:
  enum {
    ICIP = Kmem::Pic_map_base + 0x000000,
    ICMR = Kmem::Pic_map_base + 0x000004,
    ICLR = Kmem::Pic_map_base + 0x000008,
    ICCR = Kmem::Pic_map_base + 0x000014,
    ICFP = Kmem::Pic_map_base + 0x00000c,
    ICPR = Kmem::Pic_map_base + 0x000010,
  };
};

INTERFACE [arm-sa1100]: // -------------------------------------

#include "kmem.h"

EXTENSION class Pic
{
public:
  enum
  {
    Multi_irq_pending = 1,
  };

private:
  enum {
    ICIP = Kmem::Pic_map_base + 0x00000,
    ICMR = Kmem::Pic_map_base + 0x00004,
    ICLR = Kmem::Pic_map_base + 0x00008,
    ICCR = Kmem::Pic_map_base + 0x0000c,
    ICFP = Kmem::Pic_map_base + 0x00010,
    ICPR = Kmem::Pic_map_base + 0x00020,
  };
};

// -------------------------------------------------------------
IMPLEMENTATION [arm && (sa1100 || pxa)]:

#include <cstring>
#include <cstdio>

#include "boot_info.h"
#include "config.h"
#include "initcalls.h"
#include "io.h"

IMPLEMENT FIASCO_INIT
void Pic::init()
{
  // only unmasked interrupts wakeup from idle
  Io::write(0x01, ICCR);
  // mask all interrupts
  Io::write(0x00, ICMR);
  // all interrupts are IRQ's (no FIQ)
  Io::write(0x00, ICLR);
}

IMPLEMENT inline NEEDS["io.h"]
void Pic::disable_locked( unsigned irq )
{
  Io::write(Io::read<Mword>(ICMR) & ~(1<<irq), ICMR);
}

IMPLEMENT inline NEEDS["io.h"]
void Pic::enable_locked(unsigned irq, unsigned /*prio*/)
{
  Io::write(Io::read<Mword>(ICMR) | (1<<irq), ICMR);
}

IMPLEMENT inline
void Pic::acknowledge_locked( unsigned /*irq*/ )
{}


IMPLEMENT inline NEEDS["io.h"]
Pic::Status Pic::disable_all_save()
{
  Status s;
  s  = Io::read<Mword>(ICMR);
  Io::write(0, ICMR);
  return s;
}

IMPLEMENT inline NEEDS["io.h"]
void Pic::restore_all( Status s )
{
  Io::write(s, ICMR);
}

PUBLIC static inline NEEDS["io.h"]
Unsigned32 Pic::pending()
{
  return Io::read<Unsigned32>(ICIP);
}

PUBLIC static inline NEEDS[Pic::pending]
Mword Pic::is_pending(Mword &irqs, Mword irq)
{ 
  Mword ret = irqs & (1 << irq);
  irqs &= ~(1 << irq);
  return ret;
}

IMPLEMENT inline NEEDS [Pic::disable_locked]
void Pic::block_locked(unsigned irq)
{ disable_locked(irq); }

