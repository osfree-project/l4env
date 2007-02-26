/*
 * Fiasco-UX
 * Architecture specific cpu init code
 */

INTERFACE[ux]:

class Tss;

EXTENSION class Cpu 
{
private:
  static Tss *tss asm ("CPU_TSS");
  static int msr_dev;
  static unsigned long _gs asm ("CPU_GS");
};


IMPLEMENTATION[ux]:

#include <cerrno>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include "gdt.h"
#include "initcalls.h"
#include "processor.h"
#include "regdefs.h"
#include "tss.h"

Proc::Status volatile Proc::virtual_processor_state = 0;
Tss *Cpu::tss;
int Cpu::msr_dev;
unsigned long Cpu::_gs;

IMPLEMENT FIASCO_INIT
void
Cpu::init (void)
{
  identify();

  // No Sysenter Support for Fiasco-UX
  _features &= ~FEAT_SEP;

  // Determine CPU frequency
  FILE *fp;
  if ((fp = fopen ("/proc/cpuinfo", "r")) != NULL)
    {
      char buffer[128];
      float val;

      while (fgets (buffer, sizeof (buffer), fp))
        if (sscanf (buffer, "cpu MHz%*[^:]: %f", &val) == 1)
          {
            _frequency    = (Unsigned64)(val * 1000) * 1000;
	    scaler_tsc_to_ns = muldiv (1 << 27, 1000000000,    _frequency);
	    scaler_tsc_to_us = muldiv (1 << 27, 32 * 1000000,  _frequency);
	    scaler_ns_to_tsc = muldiv (1 << 27, _frequency, 1000000000);
            break;
          }

      fclose (fp);
    }

  // XXX hardcoded
  msr_dev = open("/dev/msr", O_RDWR);
  if (msr_dev == -1)
    msr_dev = open("/dev/cpu/0/msr", O_RDWR);
}

PUBLIC static inline
void
Cpu::set_fast_entry (void (*)(void))
{}

PUBLIC static FIASCO_INIT
void
Cpu::init_tss (Address tss_mem)
{
  tss = reinterpret_cast<Tss*>(tss_mem);
  tss->_ss0 = Gdt::gdt_data_kernel;
}

PUBLIC static inline
Tss*
Cpu::get_tss ()
{ return tss; }

PUBLIC static inline
void
Cpu::enable_rdpmc()
{
}

IMPLEMENT inline
int
Cpu::can_wrmsr()
{
  return msr_dev != -1;
}

PUBLIC static
Unsigned64
Cpu::rdmsr (Unsigned32 reg)
{
  Unsigned64 msr;

  if (lseek(msr_dev, reg, SEEK_SET) >= 0)
    read(msr_dev, &msr, sizeof(msr));

  return msr;
}

PUBLIC static inline
Unsigned64
Cpu::rdpmc (Unsigned32, Unsigned32 reg)
{
  return rdmsr(reg);
}

PUBLIC static
void
Cpu::wrmsr (Unsigned64 msr, Unsigned32 reg)
{
  if (lseek(msr_dev, reg, SEEK_SET) >= 0)
    write(msr_dev, &msr, sizeof(msr));
}

PUBLIC static inline
void
Cpu::wrmsr (Unsigned32 low, Unsigned32 high, Unsigned32 reg)
{
  Unsigned64 msr = ((Unsigned64)high << 32) | low;
  wrmsr(msr, reg);
}

PUBLIC static inline
void
Cpu::debugctl_enable()
{}

PUBLIC static inline
void
Cpu::debugctl_disable()
{}

PUBLIC static inline
void
Cpu::set_gs(Unsigned32 val)
{ _gs = val; }

PUBLIC static inline
Unsigned32
Cpu::get_gs()
{ return _gs; }

