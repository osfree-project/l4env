
#include <l4/cxx/iostream.h>
#include "mem_man.h"

#define ADD(a,b) \
  L4::cout << "add_free("#a","#b")\n"; \
  m.add_free(Region(a,b))

void mem_man_test()
{
  static Mem_man m;
  L4::cout << "mem_man_test: ....\n";
  m.dump();
  ADD(0,9);
  ADD(15,20);
  ADD(25,30);
  ADD(35,40);
  ADD(45,50);
  m.dump();
  ADD(10,11);
  m.dump();
  ADD(14,14);
  m.dump();
  ADD(12,13);
  m.dump();
  ADD(24,31);
  m.dump();
  ADD(21,25);
  m.dump();
  ADD(8,37);
  m.dump();
  ADD(0,44);
  m.dump();
  L4::cout << "mem_man_test: done\n";
}
