INTERFACE [ia32]:

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
IMPLEMENTATION [ia32-utcb]:

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
  global_utcb_ptr = (Address*) Mem_layout::Utcb_ptr_page;

  if (!Vmem_alloc::page_alloc ((void *) global_utcb_ptr,
			       Vmem_alloc::ZERO_FILL,
			       (Space::Page_user_accessible
				| Space::Page_writable
				| Pd_entry::global())))
    panic ("UTCB pointer page allocation failure");

  Cpu::get_gdt()->set_entry_byte (Gdt::gdt_utcb / 8, 
				  (Address) global_utcb_ptr,
				  sizeof (Address) -1,
				  Gdt_entry::Access_user |
				  Gdt_entry::Access_data_write |
				  Gdt_entry::Accessed,
				  Gdt_entry::Size_32);

  Cpu::set_gs (gs_value());
  init_lipc_kip();
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [ia32-!utcb]:

#include "gdt.h"

IMPLEMENT static inline NEEDS ["gdt.h"]
Unsigned32
Utcb_init::gs_value() 
{ return Gdt::gdt_data_user | Gdt::Selector_user; }


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
IMPLEMENTATION [ux-utcb]:

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
  global_utcb_ptr = (Address *) Kmem::phys_to_virt	// fill public variable
                    (Mem_layout::Utcb_ptr_frame);

  Emulation::modify_ldt (0,				// entry
			 Mem_layout::Utcb_ptr_page,	// address
			 sizeof (Address) - 1);		// limit

  init_lipc_kip();
}


//-----------------------------------------------------------------------------
IMPLEMENTATION [ux-!utcb]:

IMPLEMENT static inline
Unsigned32
Utcb_init::gs_value() 
{ return 0; }


//-----------------------------------------------------------------------------
IMPLEMENTATION[{ia32,ux}-!lipc]:

PRIVATE static inline
void
Utcb_init::init_lipc_kip()
{}


//-----------------------------------------------------------------------------
IMPLEMENTATION[{ia32,ux}-lipc]:

PRIVATE static
void
Utcb_init::init_lipc_kip()
{
  /* sizeof(Utcb) should be a power of 2 */
  assert((sizeof(Utcb) & (sizeof(Utcb) - 1)) == 0);

  // copy lipc code in the kip
  Kip *ki = Kip::k();

  // check if the lipc code fits in the kip
  assert((unsigned int) (&Mem_layout::asm_lipc_code_end 
			 - &Mem_layout::asm_lipc_code_start) 
         <= sizeof(ki->lipc_code));

  memcpy (ki->lipc_code, &Mem_layout::asm_lipc_code_start,
          &Mem_layout::asm_lipc_code_end - &Mem_layout::asm_lipc_code_start);
  printf("Enabling KIP-LIPC\n");
}
