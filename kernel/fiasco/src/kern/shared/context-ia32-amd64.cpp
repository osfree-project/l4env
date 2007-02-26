INTERFACE[ia32-segments]:

EXTENSION class Context
{
protected:
  Unsigned64	_gdt_tls[3];
  Unsigned32	_es, _fs, _gs;
};


//-----------------------------------------------------------------------------
IMPLEMENTATION[ia32-segments]:

#include "cpu.h"
#include "gdt.h"

PROTECTED inline NEEDS ["cpu.h", "gdt.h"]
void
Context::switch_gdt_tls()
{
  Cpu::get_gdt()->set_raw(Gdt::gdt_tls1 / 8, _gdt_tls[0]);
  Cpu::get_gdt()->set_raw(Gdt::gdt_tls2 / 8, _gdt_tls[1]);
  Cpu::get_gdt()->set_raw(Gdt::gdt_tls3 / 8, _gdt_tls[2]);
}

//-----------------------------------------------------------------------------
IMPLEMENTATION[ia32,amd64]:

#include "fpu.h"

/**
 * When switching away from the FPU owner, disable the FPU to cause
 * the next FPU access to trap.
 * When switching back to the FPU owner, enable the FPU so we don't
 * get an FPU trap on FPU access.
 */
IMPLEMENT inline NEEDS ["fpu.h"]
void
Context::switch_fpu (Context *t)
{
  if (Fpu::is_owner(this))
    Fpu::disable();
  
  else if (Fpu::is_owner(t))
    Fpu::enable();
}
