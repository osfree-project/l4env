INTERFACE:

#include <stddef.h>

IMPLEMENTATION:

#include <stdlib.h>
#include <stdio.h>

#include "panic.h"

char __dso_handle;

extern "C" void __cxa_pure_virtual()
{
  panic("cxa pure virtual function called");
}


extern "C" void __pure_virtual()
{
  panic("pure virtual function called");
}

void operator delete(void *)
{
  // This must not happen: We never delete an object of the abstract
  // class slab_cache_anon.  If the compiler was clever, it wouldn't
  // generate a call to this function (because all destructors of
  // abstract base classes have been are marked abstract virtual), and
  // we wouldn't need to define this.
  panic("operator delete (aka __builtin_delete) called");
}
