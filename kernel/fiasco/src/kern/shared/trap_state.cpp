
INTERFACE:

#include "l4_types.h"

class Trap_state
{
  friend class Jdb_tcb;

public:
  typedef int (*Handler)(Trap_state*);
  static Handler FIASCO_FASTCALL base_handler asm ("BASE_TRAP_HANDLER");

  // Saved segment registers
  Mword  _es;
  Mword  _ds;
  Mword  _gs;                                      // => utcb->values[ 0]
  Mword  _fs;                                      // => utcb->values[ 1]

  // PUSHA register state frame
  Mword  _edi;                                     // => utcb->values[ 2]
  Mword  _esi;                                     // => utcb->values[ 3]
  Mword  _ebp;                                     // => utcb->values[ 4]
  Mword  _cr2;  // we save cr2 over esp for PFs    // => utcb->values[ 5]
  Mword  _ebx;                                     // => utcb->values[ 6]
  Mword  _edx;                                     // => utcb->values[ 7]
  Mword  _ecx;                                     // => utcb->values[ 8]
  Mword  _eax;                                     // => utcb->values[ 9]

  // Processor trap number, 0-31
  Mword  _trapno;                                  // => utcb->values[10]

  // Error code pushed by the processor, 0 if none
  Mword  _err;                                     // => utcb->values[11]

protected:
  // Processor state frame
  Mword  _eip;                                     // => utcb->values[12]
  Mword  _cs;                                      // => utcb->values[13]
  Mword  _eflags;                                  // => utcb->values[14]
  Mword  _esp;                                     // => utcb->values[15]
  Mword  _ss;
};

IMPLEMENTATION:

#include <cstdio>
#include <panic.h>
#include "cpu.h"
#include "atomic.h"

Trap_state::Handler Trap_state::base_handler FIASCO_FASTCALL;

PUBLIC inline
Mword
Trap_state::ip() const
{ return _eip; }

PUBLIC inline
Mword
Trap_state::cs() const
{ return _cs; }

PUBLIC inline
Mword
Trap_state::flags() const
{ return _eflags; }

PUBLIC inline
Mword
Trap_state::sp() const
{ return _esp; }

PUBLIC inline
Mword
Trap_state::ss() const
{ return _ss; }

PUBLIC inline
Mword
Trap_state::value() const
{ return _eax; }

PUBLIC inline
Mword
Trap_state::value2() const
{ return _ecx; }

PUBLIC inline
Mword
Trap_state::value3() const
{ return _edx; }

PUBLIC inline
Mword
Trap_state::value4() const
{ return _ebx; }

PUBLIC inline
void
Trap_state::ip(Mword ip)
{ _eip = ip; }

PUBLIC inline
void
Trap_state::cs(Mword cs)
{ _cs = cs; }

PUBLIC inline
void
Trap_state::flags(Mword flags)
{ _eflags = flags; }

PUBLIC inline
void
Trap_state::sp(Mword sp)
{ _esp = sp; }

PUBLIC inline
void
Trap_state::ss(Mword ss)
{ _ss = ss; }

PUBLIC inline
void
Trap_state::value(Mword value)
{ _eax = value; }

PUBLIC inline
void
Trap_state::value3(Mword value)
{ _edx = value; }

PUBLIC inline NEEDS["atomic.h"] 
void
Trap_state::consume_instruction(unsigned count)
{ cas ((Address*)(&_eip), _eip, _eip + count); }

PUBLIC
void
Trap_state::dump()
{
  int from_user = _cs & 3;

  printf("EAX %08lx EBX %08lx ECX %08lx EDX %08lx\n"
         "ESI %08lx EDI %08lx EBP %08lx ESP %08lx\n"
         "EIP %08lx EFLAGS %08lx\n"
         "CS %04lx SS %04lx DS %04lx ES %04lx FS %04lx GS %04lx\n"
         "trap %ld (%s), error %08lx, from %s mode\n",
	 _eax, _ebx, _ecx, _edx,
	 _esi, _edi, _ebp, from_user ? _esp : (Unsigned32)&_esp,
	 _eip, _eflags,
	 _cs & 0xffff, from_user ? _ss & 0xffff : Cpu::get_ss() & 0xffff,
	 _ds & 0xffff, _es & 0xffff, _fs & 0xffff, _gs & 0xffff,
	 _trapno, Cpu::exception_string(_trapno), _err, from_user ? "user" : "kernel");

  if (_trapno == 13)
    {
      if (_err & 1)
	printf("(external event");
      else
	printf("(internal event");
      if (_err & 2)
	printf(" regarding IDT gate descriptor no. 0x%02lx)\n", _err >> 3);
      else
	printf(" regarding %s entry no. 0x%02lx)\n",
	      _err & 4 ? "LDT" : "GDT", _err >> 3);
    }
  else if (_trapno == 14)
    printf("page fault linear address %08lx\n", _cr2);
}

extern "C" FIASCO_FASTCALL
void
trap_dump_panic(Trap_state *ts)
{
  ts->dump();
  panic("terminated due to trap");
}
