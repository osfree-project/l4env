INTERFACE:

class Checksum 
{
};


IMPLEMENTATION:

#include "linker_syms.h"


// calculate simple checksum over kernel text section and read-only data
PUBLIC static 
unsigned Checksum::get_checksum_ro()
{
  unsigned *p, sum = 0;

  for (p = (unsigned*)&_start; p < (unsigned*)&_etext; sum += *p++)
    ;

  return sum;
}

// calculate simple checksum over kernel data section
PUBLIC static
unsigned Checksum::get_checksum_rw()
{
  unsigned *p, sum = 0;

  for (p = (unsigned*)&_etext; p < (unsigned*)&_edata; sum += *p++)
    ;

  return sum;
}

