
INTERFACE:

#include "l4_types.h"

class Trap_state
{
public:
  typedef int (*Handler)(Trap_state*);
  static Handler FIASCO_FASTCALL base_handler asm ("BASE_TRAP_HANDLER");

  // Saved segment registers
  Mword  es;
  Mword  ds;
  Mword  gs;                                      // => utcb->values[ 0]
  Mword  fs;                                      // => utcb->values[ 1]

  // PUSHA register state frame
  Mword  edi;                                     // => utcb->values[ 2]
  Mword  esi;                                     // => utcb->values[ 3]
  Mword  ebp;                                     // => utcb->values[ 4]
  Mword  cr2;  // we save cr2 over esp for PFs    // => utcb->values[ 5]
  Mword  ebx;                                     // => utcb->values[ 6]
  Mword  edx;                                     // => utcb->values[ 7]
  Mword  ecx;                                     // => utcb->values[ 8]
  Mword  eax;                                     // => utcb->values[ 9]

  // Processor trap number, 0-31
  Mword  trapno;                                  // => utcb->values[10]

  // Error code pushed by the processor, 0 if none
  Mword  err;                                     // => utcb->values[11]

  // Processor state frame
  Mword  eip;                                     // => utcb->values[12]
  Mword  cs;                                      // => utcb->values[13]
  Mword  eflags;                                  // => utcb->values[14]
  Mword  esp;                                     // => utcb->values[15]
  Mword  ss;
};

IMPLEMENTATION:

#include <cstdio>
#include <panic.h>
#include "cpu.h"

Trap_state::Handler Trap_state::base_handler FIASCO_FASTCALL;

PUBLIC inline
Mword
Trap_state::ip()
{ return eip; }

PUBLIC inline
Mword
Trap_state::sp()
{ return esp; }

PUBLIC inline
Mword
Trap_state::flags()
{ return eflags; }

PUBLIC
Mword
Trap_state::value()
{ return eax; }

PUBLIC
void
Trap_state::dump()
{
  int from_user = cs & 3;

  printf("EAX %08lx EBX %08lx ECX %08lx EDX %08lx\n"
         "ESI %08lx EDI %08lx EBP %08lx ESP %08lx\n"
         "EIP %08lx EFLAGS %08lx\n"
         "CS %04lx SS %04lx DS %04lx ES %04lx FS %04lx GS %04lx\n"
         "trapno %ld, error %08lx, from %s mode\n",
	 eax, ebx, ecx, edx,
	 esi, edi, ebp, from_user ? esp : (Unsigned32)&esp,
	 eip, eflags,
	 cs & 0xffff, from_user ? ss & 0xffff : Cpu::get_ss() & 0xffff,
	 ds & 0xffff, es & 0xffff, fs & 0xffff, gs & 0xffff,
	 trapno, err, from_user ? "user" : "kernel");

  if (trapno == 13)
    {
      if (err & 1)
	printf("(external event");
      else
	printf("(internal event");
      if (err & 2)
	printf(" regarding IDT gate descriptor no. 0x%02lx)\n", err >> 3);
      else
	printf(" regarding %s entry no. 0x%02lx)\n",
	      err & 4 ? "LDT" : "GDT", err >> 3);
    }
  else if (trapno == 14)
    printf("page fault linear address %08lx\n", cr2);
}

extern "C" FIASCO_FASTCALL
void
trap_dump_panic(Trap_state *ts)
{
  ts->dump();
  panic("terminated due to trap");
}
