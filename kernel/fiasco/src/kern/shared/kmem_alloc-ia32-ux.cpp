IMPLEMENTATION [ia32,ux]:

#include <cstdio>

#include "kip_init.h"
#include "kmem.h"
#include "types.h"
#include "helping_lock.h"

IMPLEMENT
Kmem_alloc::Kmem_alloc()
{
  void *kmem_base = Kmem::phys_to_virt (Kmem::kmem_base());

  Address kmem_size
    = reinterpret_cast<Address>(Kmem::phys_to_virt(Kmem::himem()))
    - reinterpret_cast<Address>(kmem_base);

  Kip_init::setup_kmem_region (Kmem::virt_to_phys (kmem_base), kmem_size);

  a->init();
  a->free(kmem_base, kmem_size);
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [{ia32,ux}-debug]:

#include "div32.h"

PUBLIC
void
Kmem_alloc::debug_dump()
{ 
  a->dump(); 

  unsigned long free = a->avail();
  printf("Used %ld%%, %ldkB out of %ldkB of Kmem\n",
         (unsigned long) div32(100ULL * (orig_free() - free), orig_free()),
	 (orig_free() - free + 1023)/1024,
	 (orig_free()        + 1023)/1024);
}

PRIVATE inline
unsigned long
Kmem_alloc::orig_free()
{
  return Kip::k()->reserved1.high - Kip::k()->reserved1.low;
}
