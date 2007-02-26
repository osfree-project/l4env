IMPLEMENTATION[sa1100]:

#include "kmem_space.h"

IMPLEMENT
bool Kernel_uart::startup( unsigned port )
{
  if(port!=3) return false;
  return Uart::startup( Kmem_space::UART3_MAP_BASE, 17 );
}
