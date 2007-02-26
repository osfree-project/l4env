INTERFACE:

class Vkey
{
};

IMPLEMENTATION [debug-serial-!ux]:

#include "config.h"
#include "kernel_console.h"
#include "kernel_uart.h"
#include "keycodes.h"
#include "virq.h"

static Virq vkey_irq(Config::Vkey_irq);

static char vkey_buffer[256];
static unsigned vkey_tail, vkey_head;

PUBLIC static
int
Vkey::check_(int irq = -1)
{
  int  ret = 0;
  bool hit = false;
  // disable direct console to prevent confusion of user-level keyboard or
  // mouse drivers
  Kconsole::console()->change_state(Console::DIRECT,0,~Console::INENABLED,0);

  while(1)
    {
      int c = Kconsole::console()->getchar(false);

      if (irq == Kernel_uart::uart()->irq() && c == -1)
	{
	  ret = 1;
	  break;
	}
      
      if (c == -1 || c == KEY_ESC)
	break;

      unsigned nh = (vkey_head + 1) % sizeof(vkey_buffer);
      unsigned oh = vkey_head;
      if (nh != vkey_tail)
	{
	  vkey_buffer[vkey_head] = c;
	  vkey_head = nh;
	}

      if (oh == vkey_tail)
	hit = true; 

      ret = 1;
    }

  // re-enable direct console
  Kconsole::console()->change_state(Console::DIRECT,0,~0U,Console::INENABLED);

  if (hit)
    vkey_irq.hit();

  if(Config::serial_esc == Config::SERIAL_ESC_IRQ)
    Kernel_uart::uart()->enable_rcv_irq();

  return ret;
}

PUBLIC static
int
Vkey::get()
{
  if (vkey_tail != vkey_head)
    return vkey_buffer[vkey_tail];

  return -1;
}

PUBLIC static
void
Vkey::clear()
{
  if (vkey_tail != vkey_head)
    vkey_tail = (vkey_tail + 1) % sizeof(vkey_buffer);
}


IMPLEMENTATION [debug-{!serial,ux}]:

PUBLIC static
int
Vkey::get()
{ return 0; }

PUBLIC static
void
Vkey::clear()
{}


IMPLEMENTATION[!debug,!serial]:

PUBLIC static inline
int
Vkey::check_(int)
{ return 0; }
