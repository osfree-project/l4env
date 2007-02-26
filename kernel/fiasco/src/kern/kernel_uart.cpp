INTERFACE:

class Kernel_uart
{
public:
  Kernel_uart();
  static void enable_rcv_irq();
};

INTERFACE [serial]:

#include "uart.h"
#include "std_macros.h"

/**
 * Glue between kernel and UART driver.
 */
EXTENSION class Kernel_uart : public Uart
{
private:
  /**
   * Prototype for the UART specific startup implementation.
   * @param uart, the instantiation to start.
   * @param port, the com port number.
   */
  bool startup( unsigned port );
};

//---------------------------------------------------------------------------
IMPLEMENTATION [serial]:

#include <cstring>
#include <cstdlib>
#include <cstdio>

#include "irq_alloc.h"
#include "uart.h"
#include "cmdline.h"
#include "config.h"
#include "panic.h"

PUBLIC static FIASCO_CONST
Uart *const
Kernel_uart::uart()
{
  static Kernel_uart c;
  return &c;
}

IMPLEMENT
Kernel_uart::Kernel_uart()
{
  char const * const cmdline = Cmdline::cmdline();
  char *s;

  unsigned n = Config::default_console_uart_baudrate;
  Uart::TransferMode m = Uart::MODE_8N1;
  unsigned p = Config::default_console_uart;

  if (  (s = strstr(cmdline, " -comspeed "))
      ||(s = strstr(cmdline, " -comspeed=")))
    {
      n = strtoul(s + 11, 0, 0);
      if(n>115200) 
	{
	  puts ("-comspeed greater than 115200 not supported (use 115200)!");
	  n = 115200;
	}
    }

  if (  (s = strstr(cmdline, " -comport "))
      ||(s = strstr(cmdline, " -comport=")))
    p = strtoul(s + 10, 0, 0);

  if(!startup(p))
    panic("uart_init: comport %d is not accepted by the uart driver!\n",p);

  if(!change_mode(m, n))
    panic("uart_init: somthing is wrong with the baud rate (%d)!\n",n);
}

IMPLEMENT
void
Kernel_uart::enable_rcv_irq()
{
  // we must not allocate the IRQ in the constructor but here 
  // since the constructor is called before Dirq::Dirq() constructor
  Irq_alloc *i = Irq_alloc::lookup(uart()->irq());
  if (i)
    i->alloc((Receiver*)-1, true);

  uart()->enable_rcv_irq();
}

//---------------------------------------------------------------------------
IMPLEMENTATION [!serial]: 

IMPLEMENT inline
Kernel_uart::Kernel_uart()
{}   
    
IMPLEMENT inline
void
Kernel_uart::enable_rcv_irq()
{}
