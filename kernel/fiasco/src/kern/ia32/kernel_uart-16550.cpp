IMPLEMENTATION[16550]:

IMPLEMENT
bool Kernel_uart::startup( unsigned port )
{
  switch(port) {
  case 1: return Uart::startup( 0x3f8, 4 );
  case 2: return Uart::startup( 0x2f8, 3 );
  case 3: return Uart::startup( 0x3e8, (unsigned)-1 );
  case 4: return Uart::startup( 0x2e8, (unsigned)-1 );
  default: return false;
  }
}
