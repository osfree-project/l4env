INTERFACE:
class checksum {

};

//extern unsigned saved_checksum_ro;

IMPLEMENTATION:


extern "C" char _start, _etext, _sstack, _stack, _edata, _end;
//#include "kmem.h"

// unsigned saved_checksum_ro;

// calculate simple checksum over kernel text section and read-only data
PUBLIC static 
unsigned checksum::get_checksum_ro()
{
  unsigned *p, sum = 0;

  for (p = (unsigned*)&_start; p < (unsigned*)&_etext; sum += *p++)
    ;

  return sum;
}

// calculate simple checksum over kernel data section
PUBLIC static
unsigned checksum::get_checksum_rw()
{
  unsigned *p, sum = 0;

  for (p = (unsigned*)&_etext; p < (unsigned*)&_edata; sum += *p++)
    ;

  return sum;
}

