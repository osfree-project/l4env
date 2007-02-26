INTERFACE:

#include "config_gdt.h"
#include "types.h"

EXTENSION class Utcb_alloc
{
public:
  enum { 
    gdt_utcb_address = GDT_UTCB_ADDRESS,
  };
};
 

IMPLEMENTATION[ia32-v4]:

#include <cstdio>
#include <flux/x86/seg.h>
#include "config_gdt.h"
#include "kmem.h"
#include "space.h"
#include "types.h"
#include "vmem_alloc.h"

IMPLEMENT 
void Utcb_alloc::init()
{
  global_utcb_ptr = (Address*) Kmem::_utcb_ptr_addr;

  if (!Vmem_alloc::page_alloc ((void *) global_utcb_ptr, 0, 
			       Vmem_alloc::ZERO_FILL,
			       Space::Page_user_accessible
			       |Space::Page_writable
			       |Kmem::pde_global()))
    panic ("UTCB pointer page allocation failure");

  fill_descriptor(Kmem::get_gdt() + gdt_utcb_address/8, 
		  (unsigned) global_utcb_ptr, sizeof (Address) - 1,
		  ACC_PL_U | ACC_DATA_W | ACC_A, SZ_32);
  
  set_gs (gs_value());
}

/** Value for gs
 * @return Value the GS register is to be loaded with.
 */
PUBLIC static inline NEEDS [<flux/x86/seg.h>, "types.h"]
Mword Utcb_alloc::gs_value() { return (gdt_utcb_address | SEL_PL_U); }

