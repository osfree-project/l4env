IMPLEMENTATION [arm-sa1100]:

#include "mem_layout.h"

IMPLEMENT
bool Kernel_uart::startup(unsigned port, int /*irq*/)
{
  if(port!=3) return false;
  return Uart::startup(Mem_layout::Uart_base, 17);
}

IMPLEMENTATION [arm-pxa]:

#include "mem_layout.h"

IMPLEMENT
bool Kernel_uart::startup(unsigned port, int /*irq*/)
{
  if(port!=3) return false;
  return Uart::startup(Mem_layout::Uart_base, 22);
}

IMPLEMENTATION [arm-isg]:

#include "mem_layout.h"

IMPLEMENT
bool Kernel_uart::startup(unsigned port, int /*irq*/)
{
  if(port!=3) return false;
  return Uart::startup(Mem_layout::Uart_base, 6);
}

IMPLEMENTATION [arm-integrator]:

#include "mem_layout.h"

IMPLEMENT
bool Kernel_uart::startup(unsigned port, int /*irq*/)
{
  if(port!=3) return false;
  return Uart::startup(Mem_layout::Uart_base, 1);
}

IMPLEMENTATION [arm-realview]:

#include "mem_layout.h"

IMPLEMENT
bool Kernel_uart::startup(unsigned port, int /*irq*/)
{
  if(port!=3) return false;
  return Uart::startup(Mem_layout::Uart_base, 123456);
}
