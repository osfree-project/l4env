IMPLEMENTATION [integrator]:

#include "arm/uart_integrator.h"

IMPLEMENT L4::Uart *Uart::uart()
{
  static L4::Uart_integrator uart(1,1);
  return &uart;
}

IMPLEMENTATION [realview && 926]:

#include "arm/uart_integrator.h"

IMPLEMENT L4::Uart *Uart::uart()
{
  static L4::Uart_integrator uart(44, 44);
  return &uart;
}

IMPLEMENTATION [realview && mpcore]:

#include "arm/uart_integrator.h"

IMPLEMENT L4::Uart *Uart::uart()
{
  static L4::Uart_integrator uart(36, 36);
  return &uart;
}
