/*
 * Fiasco-IA32
 * Architecture specific main startup/shutdown code
 */

IMPLEMENTATION[ia32]:

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "cmdline.h"
#include "config.h"
#include "io.h"
#include "irq.h"
#include "idt.h"
#include "kdb_ke.h"
#include "kernel_console.h"
#include "pic.h"
#include "reset.h"
#include "timer.h"
#include "terminate.h"

int exit_question_active;


extern "C" void __attribute__ ((noreturn))
_exit(int)
{
  if (exit_question_active)
    pc_reset();

  while(1)
    { 
      Proc::halt(); 
      Proc::pause();
    }
}


static
void
exit_question()
{
  exit_question_active = 1;

  Pic::Status irqs = Pic::disable_all_save();
  if (Config::getchar_does_hlt && Config::getchar_does_hlt_works_ok)
    {
      Idt::set_timer_vector_stop();
      Timer::enable();
      Proc::sti();
    }

  // make sure that we don't acknowledg the exit question automatically
  Kconsole::console()->change_state(Console::PUSH, 0, ~Console::INENABLED, 0);
  puts("\nReturn reboots, \"k\" enters L4 kernel debugger...");

  char c = Kconsole::console()->getchar();

  if (c == 'k' || c == 'K') 
    {
      Pic::restore_all(irqs);
      kdb_ke("_exit");
    }
  else
    {
      // It may be better to not call all the destruction stuff because of
      // unresolved static destructor dependency problems. So just do the
      // reset at this point.
      puts("\033[1mRebooting.\033[m");
    }
}

void
main_arch()
{
  // console initialization
  set_exit_question(&exit_question);

  Pic::disable_all_save();

  // enable debugging and internal interrupts
  for(int i = 0; i<Config::Max_num_irqs; ++i) 
    {
      Irq *irq = Irq::lookup(i);
      if(irq && (irq->owner() == (Receiver*)-1)) 
	{
	  // internal used irq
	  irq->maybe_enable();
	}
    }

  char const *s;
  if ((!strstr (Cmdline::cmdline(), " -nojdb")) &&
      ((s = strstr (Cmdline::cmdline(), " -jdb_cmd="))))
    {
      // extract the control sequence from the command line
      char ctrl[128];
      char *d;

      for (s=s+10, d=ctrl; 
	   d < ctrl+sizeof(ctrl)-1 && *s && *s != ' '; *d++ = *s++)
	;
      *d = '\0';
      kdb_ke_sequence(ctrl);
    }

  // kernel debugger rendezvous
  if ((!strstr (Cmdline::cmdline(), " -nokdb")  ||
       !strstr (Cmdline::cmdline(), " -nojdb")) &&
       !strstr (Cmdline::cmdline(), "nowait"))
    kdb_ke("init");
}
