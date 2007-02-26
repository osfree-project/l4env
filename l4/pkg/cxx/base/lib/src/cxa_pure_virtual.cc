#include <l4/cxx/iostream.h>


extern "C" void __cxa_pure_virtual()
{
  L4::cerr << "cxa pure virtual function called\n";
}


extern "C" void __pure_virtual()
{
  L4::cerr << "cxa pure virtual function called\n";
}
