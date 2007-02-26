IMPLEMENTATION:

#include <cstdio>
#include <cstring>

#include "boot_info.h"
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
  
  if(strstr(Boot_info::cmdline(), " -noserial")) // do not use serial uart 
    return;

  static Filter_console fcon(Kernel_uart::uart());
  
  int i = Kernel_uart::uart()->irq();
  if(i!=-1) 
    { 
      Kernel_uart::uart()->enable_rcv_irq();
      printf("SERIAL ESC: allocated IRQ %d for serial uart\n",i);
      puts("Do not use serial hack in slow timer handler, use direct IRQ instead.");

      // not needed because serial has its own IRQ
      // Config::serial_esc = Config::SERIAL_ESC_IRQ;
    }
  else if (Config::serial_esc)
    {
      puts("SERIAL ESC: No IRQ for specified uart port.");
      puts("Use serial hack in slow timer handler.");
    }

  Kconsole::console()->register_console(&fcon);
}
