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
#include "kernel_uart.h"



Console *Boot_console::_c;


IMPLEMENT
void Boot_console::init()
{
  Kconsole::console()->register_console(Kernel_uart::uart());
}

IMPLEMENT
Console *const Boot_console::cons()
{
  return _c;
}
