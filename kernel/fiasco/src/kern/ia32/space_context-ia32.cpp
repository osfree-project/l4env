INTERFACE:

EXTENSION class Space_context
{
public:
  Space_context();
};

IMPLEMENTATION[ia32]:

#include <cstring>
#include "config.h"
#include "kmem.h"

IMPLEMENT
Space_context::Space_context() // con: create new empty address space
{
  memset(_dir, 0, Config::PAGE_SIZE);	// initialize to zero
}

IMPLEMENT inline NEEDS ["kmem.h"]
Space_context *
Space_context::current()
{
  Address sc;
  asm volatile ("movl %%cr3, %0" : "=r" (sc));
  return reinterpret_cast<Space_context*>(Kmem::phys_to_virt(sc));
}

IMPLEMENT inline NEEDS ["kmem.h"]
void
Space_context::make_current()
{
  asm volatile ("movl %0, %%cr3" : : "r" (Kmem::virt_to_phys(this)));
}

