INTERFACE [ia32,amd64]:

EXTENSION class Utcb_init
{
public:
  /**
   * Value for GS.
   * @return Value the GS register has to be loaded with when entering user
   *         mode.
   */
  static Unsigned32 gs_value();
};


//-----------------------------------------------------------------------------
IMPLEMENTATION [ia32]:

#include <cstdio>
#include "gdt.h"
#include "paging.h"
#include "panic.h"
#include "space.h"
#include "vmem_alloc.h"

IMPLEMENT static inline NEEDS ["gdt.h"]
Unsigned32
Utcb_init::gs_value() 
{ return Gdt::gdt_utcb | Gdt::Selector_user; }

IMPLEMENT 
void
Utcb_init::init()
{
  if (!Vmem_alloc::page_alloc ((void *) Mem_layout::Utcb_ptr_page,
	Vmem_alloc::ZERO_FILL, Vmem_alloc::User))
    panic ("UTCB pointer page allocation failure");

  Cpu::get_gdt()->set_entry_byte (Gdt::gdt_utcb / 8, 
				  Mem_layout::Utcb_ptr_page,
				  sizeof (Address) -1,
				  Gdt_entry::Access_user |
				  Gdt_entry::Access_data_write |
				  Gdt_entry::Accessed,
				  Gdt_entry::Size_32);

  Cpu::set_gs (gs_value());
}


//-----------------------------------------------------------------------------
INTERFACE [ux]:

EXTENSION class Utcb_init
{
public:
  /**
   * Value for GS.
   * @return Value the GS register has to be loaded with when entering user
   *         mode.
   */
  static Unsigned32 gs_value();
};


//-----------------------------------------------------------------------------
IMPLEMENTATION [ux]:

#include "emulation.h"
#include "kip.h"
#include "kmem.h"

IMPLEMENT static inline
Unsigned32
Utcb_init::gs_value() 
{ return 7; }	// RPL=3, TI=LDT, Index=0

IMPLEMENT 
void
Utcb_init::init()
{
  Emulation::modify_ldt (0,					// entry
			 Mem_layout::Utcb_ptr_page_user,	// address
			 sizeof (Address) - 1);			// limit

}

