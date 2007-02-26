#include <l4/cxx/iostream.h>

void operator delete (void *obj)
{
  L4::cerr << "cxa pure delete operator called for object @" 
           << L4::hex << obj << L4::dec << "\n";
}
