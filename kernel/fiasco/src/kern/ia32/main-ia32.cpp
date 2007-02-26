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
#include "kdb_ke.h"
#include "kernel_console.h"
#include "pic.h"
#include "reset.h"
#include "timer.h"

static int exit_question_active = 0;


extern "C" void __attribute__ ((noreturn))
_exit(int)
{
  if(exit_question_active)
    pc_reset();
  else
    while(1) { Proc::halt(); Proc::pause(); }
}


static void exit_question()
{
  exit_question_active = 1;
  Pic::Status irqs = Pic::disable_all_save();
  set_timer_vector_stop();
  Timer::enable();
  Proc::sti();

  while(1) 
    {
      puts("\nReturn reboots, \"k\" enters L4 kernel debugger...");

      char c = Kconsole::console()->getchar();
    
      if (c == 'k' || c == 'K') 
	{
	  Pic::restore_all(irqs);
	  kdb_ke("_exit");
	}
      else
	// it may be better to not call all the destruction stuff 
	// because of unresolved static destructor dependency 
	// problems.
	// SO just do the reset at this point.
	puts("\033[1mRebooting...\033[0m");
      break;
    }
}

void main_arch()
{
  // console initialization
  atexit(&exit_question);

  Pic::disable_all_save();

  // enable debugging and internal interrupts
  for(int i = 0; i<Config::MAX_NUM_IRQ; ++i) 
    {
      Irq *irq = Irq::lookup(i);
      if(irq && (irq->owner() == (Receiver*)-1)) 
	{
	  // internal used irq
	  irq->maybe_enable();
	}
    }

  // kernel debugger rendezvous
  if ((!strstr (Cmdline::cmdline(), " -nokdb")  ||
       !strstr (Cmdline::cmdline(), " -nojdb")) &&
       !strstr (Cmdline::cmdline(), "nowait"))
    kdb_ke("init");
}
