INTERFACE:

#include "initcalls.h"
#include "types.h"

class Utcb_init
{
public:
  /**
   * UTCB access initialization.
   *
   * Allocates the UTCB pointer page and maps it to Kmem::utcb_ptr_page.
   * Setup both segment selector and gs register to allow gs:0 access.
   *
   * @post user can access the utcb pointer via gs:0.
   */
  static void init() FIASCO_INIT;
};

IMPLEMENTATION:

#include <feature.h>
#include "cpu.h"

KIP_KERNEL_FEATURE("utcb");

//-----------------------------------------------------------------------------
IMPLEMENTATION [!ux && !ia32]:

#include "mem_layout.h"
#include "paging.h"
#include "panic.h"
#include "vmem_alloc.h"

IMPLEMENT 
void
Utcb_init::init()
{
  if (!Vmem_alloc::page_alloc ((void *) Mem_layout::Utcb_ptr_page,
	Vmem_alloc::ZERO_FILL, Vmem_alloc::User))
    panic ("UTCB pointer page allocation failure");
}

