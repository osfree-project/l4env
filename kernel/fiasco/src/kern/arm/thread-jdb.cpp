IMPLEMENTATION[jdb]:

#include <cstdio>
#include <simpleio.h>
#include "kdb_ke.h"
#include "cpu_lock.h"

extern "C" void sys_kdb_ke()
{
  cpu_lock.lock();
  Thread *t = current_thread();
  unsigned *x = (unsigned*)t->regs()->pc();
  switch(*x) 
    {
    case 0xe35e0002: 
      putstr((char*)t->regs()->r[0]);
      break;
    case 0xe35e0003: 
      putnstr((char*)t->regs()->r[0], t->regs()->r[1]);
      break;
    default:
      kdb_ke("USER ENTRY");
      break; 
    }

  if(Proc::interrupts()) putchar('Ü');
  //cpu_lock.clear();
  //printf("[ Blah: %p ]",__builtin_return_address(0));
  
}
