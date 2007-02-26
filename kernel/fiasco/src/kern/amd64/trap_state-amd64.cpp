
INTERFACE:

#include "l4_types.h"

class Trap_state
{
  friend class Jdb_tcb;
public:
  typedef int (*Handler)(Trap_state*);
  static Handler FIASCO_FASTCALL base_handler asm ("BASE_TRAP_HANDLER");

  // No saved segment registers

  // register state frame
  Mword  _r15;
  Mword  _r14;
  Mword  _r13;
  Mword  _r12;
  Mword  _r11;
  Mword  _r10;
  Mword  _r9;
  Mword  _r8;
  Mword  _rdi;
  Mword  _rsi;
  Mword  _rbp;
  Mword  _cr2;  // we save cr2 over esp for page faults
  Mword  _rbx;
  Mword  _rdx;
  Mword  _rcx;
  Mword  _rax;

  // Processor trap number, 0-31
  Mword  _trapno;

  // Error code pushed by the processor, 0 if none
  Mword  _err;

protected:
  // Processor state frame
  Mword  _rip;
  Mword  _cs;
  Mword  _rflags;
  Mword  _rsp;
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
{ return _rip; }

PUBLIC inline
Mword
Trap_state::cs() const
{ return _cs; }

PUBLIC inline
Mword
Trap_state::flags() const
{ return _rflags; }

PUBLIC inline
Mword
Trap_state::sp() const
{ return _rsp; }

PUBLIC inline
Mword
Trap_state::ss() const
{ return _ss; }

PUBLIC inline
Mword
Trap_state::value() const
{ return _rax; }

PUBLIC inline
Mword
Trap_state::value2() const
{ return _rcx; }

PUBLIC inline
Mword
Trap_state::value3() const
{ return _rdx; }

PUBLIC inline
Mword
Trap_state::value4() const
{ return _rbx; }

PUBLIC inline
void
Trap_state::ip(Mword ip)
{ _rip = ip; }

PUBLIC inline
void
Trap_state::cs(Mword cs)
{ _cs = cs; }

PUBLIC inline
void
Trap_state::flags(Mword flags)
{ _rflags = flags; }

PUBLIC inline
void
Trap_state::sp(Mword sp)
{ _rsp = sp; }

PUBLIC inline
void
Trap_state::ss(Mword ss)
{ _ss = ss; }

PUBLIC inline
void
Trap_state::value(Mword value)
{ _rax = value; }

PUBLIC inline
void
Trap_state::value3(Mword value)
{ _rdx = value; }

PUBLIC inline NEEDS["atomic.h"] 
void
Trap_state::consume_instruction(unsigned count)
{ cas ((Address*)(&_rip), _rip, _rip + count); }

PUBLIC
void
Trap_state::dump()
{
  int from_user = _cs & 3;

  printf("RAX %016lx    RBX %016lx\n", _rax, _rbx);
  printf("RCX %016lx    RDX %016lx\n", _rcx, _rdx);
  printf("RSI %016lx    RDI %016lx\n", _rsi, _rdi);
  printf("RBP %016lx    RSP %016lx\n", _rbp, from_user ? _rsp : (Address)&_rsp);
  printf("R8  %016lx    R9  %016lx\n", _r8,  _r9);
  printf("R10 %016lx    R11 %016lx\n", _r10, _r11);
  printf("R12 %016lx    R12 %016lx\n", _r12, _r13);
  printf("R14 %016lx    R15 %016lx\n", _r14, _r15);
  printf("RIP %016lx RFLAGS %016lx\n", _rip, _rflags);
  printf("CS %04lx SS %04lx\n", _cs, _ss);
  printf("\n");
  printf("trapno %d, error %lx, from %s mode\n",
      (unsigned)_trapno, _err, from_user ? "user" : "kernel");

  if (_trapno == 13)
    {
      if (_err & 1)
	printf("(external event");
      else
	printf("(internal event");
      if (_err & 2)
	{
	  printf(" regarding IDT gate descriptor no. 0x%02lx)\n", _err >> 3);
	}
      else
	{
	  printf(" regarding %s entry no. 0x%02lx)\n",
	      _err & 4 ? "LDT" : "GDT", _err >> 3);
	}
    }
  else if (_trapno == 14)
    printf("page fault linear address %16lx\n", _cr2);
}

extern "C" FIASCO_FASTCALL
void
trap_dump_panic(Trap_state *ts)
{
  ts->dump();
  panic("terminated due to trap");
}
