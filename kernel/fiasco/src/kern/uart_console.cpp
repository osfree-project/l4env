IMPLEMENTATION:

#include <cstdio>
#include <cstring>

#include "cmdline.h"
#include "config.h"
#include "filter_console.h"
#include "kernel_console.h"
#include "kernel_uart.h"
#include "static_init.h"
#include "irq.h"

static void uart_console_init();

STATIC_INITIALIZER_P(uart_console_init ,UART_INIT_PRIO);

static void uart_console_init()
{
  
  if(strstr (Cmdline::cmdline(), " -noserial")) // do not use serial uart 
    return;

  static Filter_console fcon(Kernel_uart::uart());
  int irq = -1;

  if (Config::serial_esc == Config::SERIAL_ESC_IRQ)
    {
      if ((irq = Kernel_uart::uart()->irq()) == -1)
	Config::serial_esc = Config::SERIAL_ESC_NOIRQ;
    }

  switch (Config::serial_esc)
    {
    case Config::SERIAL_ESC_NOIRQ:
      puts("SERIAL ESC: No IRQ for specified uart port.");
      puts("Using serial hack in slow timer handler.");
      break;

    case Config::SERIAL_ESC_IRQ:
      Kernel_uart::uart()->enable_rcv_irq();
      printf("SERIAL ESC: allocated IRQ %d for serial uart\n", irq);
      puts("Not using serial hack in slow timer handler.");
      break;
    }

  Kconsole::console()->register_console(&fcon, 0);
}

