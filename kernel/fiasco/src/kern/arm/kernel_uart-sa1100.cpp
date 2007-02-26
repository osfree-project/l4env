IMPLEMENTATION [arm-sa1100]:

#include "mem_layout.h"

IMPLEMENT
bool Kernel_uart::startup( unsigned port )
{
  if(port!=3) return false;
  return Uart::startup(Mem_layout::Uart_base, 17);
}

IMPLEMENTATION [arm-pxa]:

#include "mem_layout.h"

IMPLEMENT
bool Kernel_uart::startup( unsigned port )
{
  if(port!=3) return false;
  return Uart::startup(Mem_layout::Uart_base, 22);
}

IMPLEMENTATION [arm-isg]:

#include "mem_layout.h"

IMPLEMENT
bool Kernel_uart::startup( unsigned port )
{
  if(port!=3) return false;
  return Uart::startup(Mem_layout::Uart_base, 6);
}

