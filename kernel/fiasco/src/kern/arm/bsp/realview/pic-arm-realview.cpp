INTERFACE [arm && realview]:

#include "kmem.h"

EXTENSION class Pic
{
public:
  enum
  {
    Multi_irq_pending = 0,

    No_irq_pending = 1023,
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
    CPU_BPR           = Kmem::Gic_cpu_map_base + 0x08,
    CPU_INTACK        = Kmem::Gic_cpu_map_base + 0x0c,
    CPU_EOI           = Kmem::Gic_cpu_map_base + 0x10,
    CPU_RUNINT        = Kmem::Gic_cpu_map_base + 0x14,
    CPU_PENDING       = Kmem::Gic_cpu_map_base + 0x18,
  };
};

INTERFACE [arm && realview && mpcore]:

EXTENSION class Pic
{
private:
  enum
  {
    INTMODE_NEW_NO_DDC = 1 << 23,
  };
};


//-------------------------------------------------------------------
IMPLEMENTATION [arm && !mpcore]:

PRIVATE static inline
void Pic::configure_core()
{}

PUBLIC static inline
bool Pic::is_ipi(unsigned)
{ return 0; }

PRIVATE static inline
unsigned Pic::platform_irqs()
{ return 96; }

//-------------------------------------------------------------------
IMPLEMENTATION [arm && mpcore]:

#include "platform.h"

PRIVATE static
void Pic::unlock_config()
{ Io::write<Mword>(0xa05f, Platform::Sys::Lock); }

PRIVATE static
void Pic::lock_config()
{ Io::write<Mword>(0x0, Platform::Sys::Lock); }

PRIVATE static
void Pic::configure_core()
{
  // Enable 'new' interrupt-mode, no DCC
  unlock_config();
  Io::write<Mword>(Io::read<Mword>(Platform::Sys::Pld_ctrl1) | INTMODE_NEW_NO_DDC,
                   Platform::Sys::Pld_ctrl1);
  lock_config();
}

PRIVATE static inline
unsigned Pic::platform_irqs()
{ return 64; }

//-------------------------------------------------------------------
IMPLEMENTATION [arm && pic_gic]:

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
  configure_core();

  Io::write<Mword>(0, DIST_CTRL);

  unsigned nr_irqs = ((Io::read<Mword>(DIST_CTR) & 0x1f) + 1) * 32;
  printf("Number of IRQs available: %d\n", nr_irqs);
  if (nr_irqs != platform_irqs())
    panic("Unexpected number of IRQs detected!");

  unsigned int intmask = 1 << Proc::cpu_number();
  intmask |= intmask << 8;
  intmask |= intmask << 16;

  for (unsigned i = 32; i < nr_irqs; i += 16)
    Io::write<Mword>(0, DIST_CONFIG + i * 4 / 16);
  for (unsigned i = 32; i < nr_irqs; i += 4)
    Io::write<Mword>(intmask, DIST_TARGET + i);
  for (unsigned i = 0; i < nr_irqs; i += 4)
    Io::write<Mword>(0xa0a0a0a0, DIST_PRI + i);
  for (unsigned i = 0; i < nr_irqs; i += 32)
    Io::write<Mword>(0xffffffff, DIST_ENABLE_CLEAR + i * 4 / 32);

  Io::write<Mword>(1, DIST_CTRL);
  Io::write<Mword>(1, CPU_CTRL);

  Io::write<Mword>(0xf0, CPU_PRIMASK);
}

IMPLEMENT inline NEEDS["io.h"]
void Pic::disable_locked( unsigned irq )
{ Io::write<Mword>(1 << (irq % 32), DIST_ENABLE_CLEAR + (irq / 32) * 4); }

IMPLEMENT inline NEEDS["io.h"]
void Pic::enable_locked(unsigned irq, unsigned /*prio*/)
{ Io::write<Mword>(1 << (irq % 32), DIST_ENABLE_SET + (irq / 32) * 4); }

IMPLEMENT inline NEEDS [Pic::enable_locked]
void Pic::acknowledge_locked( unsigned irq )
{ Io::write<Mword>(irq, CPU_EOI); }

IMPLEMENT inline
Pic::Status Pic::disable_all_save()
{ return 0; }

IMPLEMENT inline
void Pic::restore_all( Status /*s*/ )
{}

PUBLIC static inline NEEDS["io.h"]
Unsigned32 Pic::pending()
{ return Io::read<Mword>(CPU_INTACK) & 0x3ff; }

PUBLIC static inline
Mword Pic::is_pending(Mword &irqs, Mword irq)
{ return irqs == irq; }

IMPLEMENT inline NEEDS [Pic::disable_locked]
void Pic::block_locked(unsigned irq)
{ disable_locked(irq); }
