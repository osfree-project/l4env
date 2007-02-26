IMPLEMENTATION:

#include <cstdio>

#include "irq_alloc.h"
#include "jdb_module.h"
#include "kernel_console.h"
#include "static_init.h"
#include "types.h"


//===================
// Std JDB modules
//===================

/**
 * @brief 'IRQ' module.
 * 
 * This module handles the 'R' command that
 * provides IRQ attachment and listing functions.
 */
class Jdb_attach_irq
  : public Jdb_module
{
public:
private:
  static char     subcmd;
  static unsigned irq;
};

char     Jdb_attach_irq::subcmd;
unsigned Jdb_attach_irq::irq;
static Jdb_attach_irq jdb_attach_irq INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

PUBLIC Jdb_module::Action_code
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
  	      while((r=Irq_alloc::lookup(i++)))
  		{
  		  printf("\nIRQ %02x: %p (%s)", 
  			 i-1, r->owner(), 
			 ((Mword)r->owner()==0)
			   ? "unassigned"
			   : (((Smword)r->owner()==-1)
			     ? "JDB"
			     : "user"));
		}
	      putchar('\n');
	    }
	  return NOTHING;
	}
    }
  else if ( (unsigned*)args = &irq )
    {
      Irq_alloc *i = Irq_alloc::lookup(irq);
      if(i) i->alloc((Receiver*)-1,true);
    }
  return NOTHING;
}

PUBLIC
int const Jdb_attach_irq::num_cmds() const
{ 
  return 1;
}

PUBLIC
Jdb_module::Cmd const *const Jdb_attach_irq::cmds() const
{
  static Cmd cs[] =
    { Cmd( 0, "R", "irq", " cmd=%c",
	   "R{l|a}<num>\tlist IRQ threads, attach Jdb to IRQ", &subcmd )
    };

  return cs;
}

