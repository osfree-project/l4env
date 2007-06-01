//-----------------------------------------------------------------------------
IMPLEMENTATION[ia32-segments]:

#include "cpu.h"
#include "gdt.h"

PROTECTED inline NEEDS ["cpu.h", "gdt.h"]
void
Context::switch_gdt_user_entries(Context *to)
{
  Cpu::get_gdt()->set_raw(Gdt::gdt_user_entry1 / 8, to->_gdt_user_entries[0]);
  Cpu::get_gdt()->set_raw(Gdt::gdt_user_entry2 / 8, to->_gdt_user_entries[1]);
  Cpu::get_gdt()->set_raw(Gdt::gdt_user_entry3 / 8, to->_gdt_user_entries[2]);
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

IMPLEMENTATION[amd64]:

IMPLEMENT inline
Mword
Context::state() const
{
  Mword res;
  asm volatile ("mov (%1), %0		\n\t"
		: "=acd" (res) : "acdbSD" (&_state));
  return res;

}

IMPLEMENT inline
Mword
Context::is_tcb_mapped() const
{
  // Touch the state to page in the TCB. If we get a pagefault here,
  // the handler doesn't handle it but returns immediatly after
  // setting eax to 0xffffffff
  Mword pagefault_if_0;
  asm volatile ("mov (%2), %0	\n\t"
		"setnc %b0	\n\t"
		: "=acd" (pagefault_if_0) : "0"(0UL), "acdbSD"(&_state));
  return pagefault_if_0;
}
