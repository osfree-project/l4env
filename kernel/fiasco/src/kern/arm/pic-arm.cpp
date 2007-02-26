INTERFACE [arm-sa1100]:

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

INTERFACE [arm-pxa]:

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

INTERFACE [arm-integrator]:

#include "kmem.h"

EXTENSION class Pic
{
public:
  enum
  {
    Multi_irq_pending = 1,
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

INTERFACE [arm-realview]:

#include "kmem.h"

EXTENSION class Pic
{
public:
  enum
  {
    Multi_irq_pending = 0,
  };

private:
  enum
  {
    DIST_CTRL         = Kmem::Gic_dist_map_base + 0x000,
    DIST_CTR          = Kmem::Gic_dist_map_base + 0x004,
    DIST_ENABLE_SET   = Kmem::Gic_dist_map_base + 0x100,
    DIST_ENABLE_CLEAR = Kmem::Gic_dist_map_base + 0x180,
    DIST_PRI          = Kmem::Gic_dist_map_base + 0x400,
    DIST_TARGET       = Kmem::Gic_dist_map_base + 0x800,
    DIST_CONFIG       = Kmem::Gic_dist_map_base + 0xc00,

    CPU_CTRL          = Kmem::Gic_cpu_map_base + 0x00,
    CPU_PRIMASK       = Kmem::Gic_cpu_map_base + 0x04,
    CPU_INTACK        = Kmem::Gic_cpu_map_base + 0x0c,
    CPU_EOI           = Kmem::Gic_cpu_map_base + 0x10,
    CPU_PENDING       = Kmem::Gic_cpu_map_base + 0x18,
  };
};

IMPLEMENTATION [arm-{sa1100,pxa}]:

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

IMPLEMENT inline NEEDS [Pic::enable_locked]
void Pic::acknowledge_locked( unsigned irq )
{ Pic::enable_locked(irq); }


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


IMPLEMENTATION [arm-integrator]:

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

IMPLEMENT inline NEEDS [Pic::enable_locked]
void Pic::acknowledge_locked(unsigned irq)
{
  Pic::enable_locked(irq);
}


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

IMPLEMENTATION [arm-realview]:

#include <cstring>
#include <cstdio>

#include "boot_info.h"
#include "config.h"
#include "initcalls.h"
#include "io.h"
#include "panic.h"

IMPLEMENT FIASCO_INIT
void Pic::init()
{
  Io::write<Mword>(0, DIST_CTRL);

  unsigned nr_irqs = ((Io::read<Mword>(DIST_CTR) & 0x1f) + 1) * 32;

  printf("GIC irqs: %d\n", nr_irqs);
  if (nr_irqs != 96)
    panic("Unexpected number of IRQs");

  // level triggered, active low
  for (unsigned i = 32; i < nr_irqs; i += 16)
    Io::write<Mword>(0, DIST_CONFIG + i * 4 / 16);
  for (unsigned i = 32; i < nr_irqs; i += 4)
    Io::write<Mword>(1 << 24, DIST_TARGET + i);
  for (unsigned i = 0; i < nr_irqs; i += 4)
    Io::write<Mword>(0xa0a0a0a0, DIST_PRI + i);
  for (unsigned i = 0; i < nr_irqs; i += 32)
    Io::write<Mword>(0xffffffff, DIST_ENABLE_SET + i * 4 / 32);

  Io::write<Mword>(1, DIST_CTRL);

  Io::write<Mword>(0xf0, CPU_PRIMASK);
  Io::write<Mword>(1, CPU_CTRL);
}

IMPLEMENT inline NEEDS["io.h"]
void Pic::disable_locked( unsigned irq )
{
  Io::write<Mword>(1 << (irq % 32), DIST_ENABLE_CLEAR + (irq / 32) * 4);
  Io::write<Mword>(irq, CPU_EOI);
}

IMPLEMENT inline NEEDS["io.h"]
void Pic::enable_locked(unsigned irq, unsigned /*prio*/)
{
  Io::write<Mword>(1 << (irq % 32), DIST_ENABLE_SET + (irq / 32) * 4);
}

IMPLEMENT inline NEEDS [Pic::enable_locked]
void Pic::acknowledge_locked( unsigned irq )
{
  Io::write<Mword>(irq, CPU_EOI);
}


IMPLEMENT inline
Pic::Status Pic::disable_all_save()
{
  return 0;
}

IMPLEMENT inline
void Pic::restore_all( Status /*s*/ )
{
}

PUBLIC static inline NEEDS["io.h"]
Unsigned32 Pic::pending()
{
  return Io::read<Mword>(CPU_INTACK);
}

PUBLIC static inline
Mword Pic::is_pending(Mword &irqs, Mword irq)
{
  return irqs == irq;
}

IMPLEMENT inline NEEDS [Pic::disable_locked]
void Pic::block_locked(unsigned irq)
{
  disable_locked(irq);
}
