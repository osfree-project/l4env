INTERFACE:

#include "mux_console.h"

class Kconsole : public Mux_console
{
public:
  int  getchar( bool blocking = true );
  void getchar_chance();

  static Mux_console *console();

private:
  static bool initialized;
};

IMPLEMENTATION:

#include "config.h"
#include "console.h"
#include "mux_console.h"
#include "processor.h"


IMPLEMENT
int Kconsole::getchar( bool blocking )
{
  if (!blocking)
    return Mux_console::getchar(false);

  while(1)
    {
      int c;
      if ((c = Mux_console::getchar(false)) != -1)
	return c;

      if(Config::getchar_does_hlt)
	Proc::halt();
      else
	Proc::pause();
    }
}



bool Kconsole::initialized = false;

IMPLEMENT 
Mux_console *Kconsole::console()
{
  static Kconsole cons;
  if (!initialized) 
    {
      initialized = true;
      Console::stdout = &cons;
      Console::stderr = &cons;
      Console::stdin  = &cons;
    }
  return &cons;
}

