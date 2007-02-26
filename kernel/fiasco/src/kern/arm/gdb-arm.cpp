INTERFACE:

EXTENSION class Gdb_entry_frame
{
public:
  Unsigned32 r[15];
  Unsigned32 spsr;
  Unsigned32 cpsr;
  Unsigned32 ksp;
  Unsigned32 pc;
};

IMPLEMENTATION[arm]:

#include "kernel_uart.h"

IMPLEMENT
Gdb_s::Gdb_s()
  : Gdb_serv(Kernel_uart::uart())
{}


PRIVATE
void Gdb_s::send_mword( Mword val )
{
  send_byte(val % 256); 
  send_byte((val>>8) % 256); 
  send_byte((val >> 16) % 256); 
  send_byte((val >> 24) % 256);
}

IMPLEMENT
void Gdb_s::get_register( unsigned index )
{
  Gdb_entry_frame *e = get_entry_frame();
  if( index < 15 )
    send_mword( e->r[index] );
  else if( index < 16 )
    send_mword( e->pc );
  else if( index < 24 )
    {
      send_byte(0); send_byte(0); send_byte(0); send_byte(0);
      send_byte(0); send_byte(0); send_byte(0); send_byte(0);
      send_byte(0); send_byte(0); send_byte(0); send_byte(index);
    }
  else if( index < 25 )
    send_mword(0);
  else if( index < 26 )
    send_mword( e->cpsr );
}

IMPLEMENT
void Gdb_s::set_register( unsigned /*index*/, char const */*value*/ )
{
}

IMPLEMENT
unsigned Gdb_s::num_registers() const
{
  return 26;
}

IMPLEMENT
Gdb_entry_frame *Gdb_s::get_entry_frame()
{
  extern Gdb_entry_frame _kdebug_stack_top[];
  return _kdebug_stack_top -1;
}
