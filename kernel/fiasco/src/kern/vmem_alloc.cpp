INTERFACE:

#include "paging.h"
#include "kern_types.h"


class Vmem_alloc
{
public:

  enum Zero_fill {
    NO_ZERO_FILL = 0,
    ZERO_FILL,
    ZERO_MAP
  };
  
  static void init();

  static void *page_alloc( void *address, int order = 0, Zero_fill zf = NO_ZERO_FILL,
			   Page::Attribs pa = Page::USER_NO );

  static void page_free( void *page, int order = 0 );

  static void page_map (void *address, int order, Zero_fill zf,
                        vm_offset_t phys);

  static void page_unmap (void *address, int order);
};

IMPLEMENTATION:
//#include "static_init.h"

//STATIC_INITIALIZE_P(Vmem_alloc,VMEM_ALLOC_INIT_PRIO);

//-
