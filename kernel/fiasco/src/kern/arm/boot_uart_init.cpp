IMPLEMENTATION:


#include <cstring>
#include <cstdlib>
#include <cstdio>

#include "uart.h"
#include "gdb_server.h"
#include "config.h"
#include "boot_info.h"
#include "static_init.h"


STATIC_INITIALIZER_P(boot_console_init,MAX_INIT_PRIO);

static void boot_console_init()
{
  unsigned n = 115200;
  Uart::TransferMode m = Uart::MODE_8N1;

  static Uart sercon;  
  //  static Gdb_serv gdb(&sercon);
  static Uart &gdb = sercon;

  // this console driver uses the mapping from the loader.
  // So uart ist mapped one to one at 0x80050000
  if(sercon.startup(0x80050000, 17) && sercon.change_mode(m, n)) {
    Console::stdout= &gdb;
    Console::stderr= &gdb;
    Console::stdin = &gdb;
  }
}

