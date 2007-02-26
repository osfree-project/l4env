IMPLEMENTATION:

#include <cstdio>

#include "irq_alloc.h"
#include "jdb_module.h"
#include "kernel_console.h"
#include "static_init.h"
#include "thread.h"
#include "types.h"


//===================
// Std JDB modules
//===================

/**
 * 'IRQ' module.
 * 
 * This module handles the 'R' command that
 * provides IRQ attachment and listing functions.
 */
class Jdb_attach_irq : public Jdb_module
{
public:
  Jdb_attach_irq() FIASCO_INIT;
private:
  static char     subcmd;
  static unsigned irq;
};

char     Jdb_attach_irq::subcmd;
unsigned Jdb_attach_irq::irq;
static Jdb_attach_irq jdb_attach_irq INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

IMPLEMENT
Jdb_attach_irq::Jdb_attach_irq()
  : Jdb_module("INFO")
{}

PUBLIC
Jdb_module::Action_code
Jdb_attach_irq::action( int cmd, void *&args, char const *&fmt, int & )
{
  if (cmd!=0)
    return NOTHING;

  if ((char*)args == &subcmd)
    {
      switch(subcmd) 
	{
	case 'a': // attach
	  args = &irq;
	  fmt = " irq: %i\n";
	  return EXTRA_INPUT;
	  
	case 'l': // list
  	    {
  	      unsigned i = 0;
  	      Irq_alloc *r;
	      putchar('\n');
  	      while((r = Irq_alloc::lookup(i++)))
  		{
  		  printf("IRQ %02x: "L4_PTR_FMT" ", i-1, (Mword)r->owner());
		  if (r->owner() == 0)
		    puts("(unassigned)");
		  else if ((Smword)r->owner() == -1)
		    puts("(JDB)");
		  else
		    {
		      Thread::lookup(context_of(r->owner()))->print_uid();
		      putchar('\n');
		    }
		}
	    }
	  return NOTHING;
	}
    }
  else if (args == (void*)&irq)
    {
      Irq_alloc *i = Irq_alloc::lookup(irq);
      if (i)
	i->alloc((Receiver*)-1, true);
    }
  return NOTHING;
}

PUBLIC
int
Jdb_attach_irq::num_cmds() const
{ 
  return 1;
}

PUBLIC
Jdb_module::Cmd const *
Jdb_attach_irq::cmds() const
{
  static Cmd cs[] =
    {   { 0, "R", "irq", " [l]ist/[a]ttach: %c",
	   "R{l|a<num>}\tlist IRQ threads, attach Jdb to IRQ", &subcmd }
    };

  return cs;
}
