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
   * @post kernel can access the utcb pointer via *global_utcb_ptr.
   * @post user can access the utcb pointer via gs:0.
   */
  static void init() FIASCO_INIT;
};

IMPLEMENTATION [utcb]:

#include "cpu.h"
#include "globals.h"
#include "mem_layout.h"

IMPLEMENTATION [!utcb]:

IMPLEMENT void Utcb_init::init() {}

