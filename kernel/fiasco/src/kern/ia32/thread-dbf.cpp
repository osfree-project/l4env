INTERFACE:

EXTENSION class Thread
{
private:
  static void handle_double_fault (void) asm ("thread_handle_double_fault");

public:
  static bool may_enter_jdb;
};


IMPLEMENTATION:

#include <cstdio>
#include "cpu.h"
#include "kernel_console.h"
#include "processor.h"
#include "reset.h"
#include "trap_state.h"
#include "tss.h"
#include "watchdog.h"


bool Thread::may_enter_jdb = false;

IMPLEMENT
void
Thread::handle_double_fault (void)
{
  volatile Tss *tss = Cpu::get_tss();
  int c;

  Watchdog::disable();

  printf ("\n"
	  "\033[1;31mDOUBLE FAULT!\033[m\n"
	  "EAX=%08x  ESI=%08x  DS=%04x  \n"
	  "EBX=%08x  EDI=%08x  ES=%04x\n"
	  "ECX=%08x  EBP=%08x  GS=%04x\n"
	  "EDX=%08x  ESP=%08x  SS=%04x   ESP0=%08x\n"
	  "EFL=%08x  EIP=%08x  CS=%04x\n",
	  tss->eax,    tss->esi, tss->ds & 0xffff,
	  tss->ebx,    tss->edi, tss->es & 0xffff,
	  tss->ecx,    tss->ebp, tss->gs & 0xffff,
	  tss->edx,    tss->esp, tss->ss & 0xffff, tss->esp0,
	  tss->eflags, tss->eip, tss->cs & 0xffff);

  if (may_enter_jdb)
    {
      puts ("Return reboots, \"k\" tries to enter the L4 kernel debugger...");

      while ((c=Kconsole::console()->getchar(false)) == -1)
	Proc::pause();

      if (c == 'k' || c == 'K')
	{
	  Mword dummy;
	  Trap_state ts;

	  // built a nice trap state the jdb can work with
	  ts.eax    = tss->eax;
	  ts.ebx    = tss->ebx;
	  ts.ecx    = tss->ecx;
	  ts.edx    = tss->edx;
	  ts.esi    = tss->esi;
	  ts.edi    = tss->edi;
	  ts.ebp    = tss->ebp;
	  ts.esp    = tss->esp;
	  ts.cs     = tss->cs;
	  ts.ds     = tss->ds;
	  ts.es     = tss->es;
	  ts.ss     = tss->ss;
	  ts.fs     = tss->fs;
	  ts.gs     = tss->gs;
	  ts.trapno = 8;
	  ts.err    = 0;
	  ts.eip    = tss->eip;
	  ts.eflags = tss->eflags;

	  asm volatile
	    (
	     "call   *%2	\n\t"
	     : "=a"(dummy)
	     : "a"(&ts), "m"(nested_trap_handler)
	     : "ecx", "edx", "memory");
	}
    }
  else
    {
      puts ("Return reboots");
      while ((Kconsole::console()->getchar(false)) == -1)
	Proc::pause();
    }

  puts ("\033[1mRebooting...\033[0m");
  pc_reset();
}
