INTERFACE:

class Console;

class Boot_console
{
public:
  static void init();
  static Console *const cons();

private:
  static Console *_c;
};

IMPLEMENTATION:

#include <cstring>
#include <cstdio>

#include "boot_info.h"
#include "kernel_console.h"
#include "keyb.h"
#include "mux_console.h"
#include "initcalls.h"
#include "vga_console.h"


Console *Boot_console::_c;

//STATIC_INITIALIZE_P( Boot_console, MAX_INIT_PRIO );

static Console *vga_console()
{
  static Vga_console v(0xf00b8000,80,25,true,true);
  return &v;
}

static Console *herc_console()
{
  static Vga_console v(0xf00b0000,80,25,true,false);
  return &v;
}


IMPLEMENT FIASCO_INIT
void Boot_console::init()
{
  static Keyb k;
  Kconsole::console()->register_console(&k);

  if( strstr( Boot_info::cmdline(), " -noscreen" ) ) 
      return;

  char const *s;

  Vga_console *c = (Vga_console*)vga_console();
  if(c->is_working())
    Kconsole::console()->register_console(c);
  
  if((s = strstr(Boot_info::cmdline(), " -hercules")) )
    {
      Vga_console *c = (Vga_console*)herc_console();
      if(c->is_working())
	Kconsole::console()->register_console(c);
      else
	puts("Requested hercules console not available!");
    }
  
 
};

IMPLEMENT
Console *const Boot_console::cons()
{
  return _c;
}
