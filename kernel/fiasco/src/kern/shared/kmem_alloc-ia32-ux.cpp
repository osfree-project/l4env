IMPLEMENTATION[ia32-ux]:

#include <cstdio>

#include "lmm.h"
#include "kip.h"
#include "kmem.h"
#include "types.h"
#include "helping_lock.h"


IMPLEMENT
void *Kmem_alloc::_phys_to_virt( void *phys ) const
{
  return (void*)Kmem::phys_to_virt((Mword)phys);
}

IMPLEMENT
void *Kmem_alloc::_virt_to_phys( void *virt ) const
{
  return (void*)Kmem::virt_to_phys(virt);

}
