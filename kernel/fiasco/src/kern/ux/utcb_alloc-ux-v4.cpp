IMPLEMENTATION[ux-v4]:

#include <flux/x86/seg.h>
#include "emulation.h"
#include "globals.h"
#include "kmem.h"

IMPLEMENT 
void
Utcb_alloc::init()
{
  global_utcb_ptr = (Address *) Kmem::phys_to_virt	// fill public variable
                    (Emulation::utcb_address_frame);

  Emulation::modify_ldt (0,				// entry
			 Emulation::utcb_address_page,	// address
			 sizeof (Address) - 1);		// limit

  set_gs (7);						// RPL=3,TI=LDT,Index=0
}
