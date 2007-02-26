INTERFACE:

#include "types.h"
#include "initcalls.h"

class Boot_info
{
public:

  static void init() FIASCO_INIT;
  static char const *cmdline();

private:
  
  static char _cmdline[256];

};



IMPLEMENTATION:

//#include "static_init.h" 

// initialized after startup cleened out the bss
char Boot_info::_cmdline[256];

//STATIC_INITIALIZE_P(Boot_info, MAX_INIT_PRIO-1);

IMPLEMENT
char const *
Boot_info::cmdline() 
{ 
  return _cmdline;
}
