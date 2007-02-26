INTERFACE [arm]:

class Kern_lib_page
{
public:
  static void init();
};


//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include <panic.h>

#include "kmem_space.h"
#include "pagetable.h"

IMPLEMENT
void Kern_lib_page::init()
{
  extern char kern_lib_start;

  if(Kmem_space::kdir()->insert( &kern_lib_start - Mem_layout::Map_base 
	                         + Mem_layout::Sdram_phys_base, 
                                 (void*)Kmem_space::Kern_lib_base, 4096,
				 Page::USER_RX | Page::CACHEABLE)
     !=Page_table::E_OK) 
    {
      panic("FATAL: Error mapping cpu page to %p\n",
	    (void*)Kmem_space::Kern_lib_base);
    }

}

asm (
    ".p2align(12)                        \n"
    "kern_lib_start:                     \n"

    // atomic add
    // r0: memory reference
    // r1: delta value
    "  ldr r2, [r0]			 \n"
    "  add r2, r2, r1                    \n"
    "  nop                               \n"
    "  str r2, [r0]                      \n"
    // forward point
    "  mov r0, r2                        \n"
    "  mov pc, lr                        \n"



    // compare exchange
    // r0: memory reference
    // r1: cmp value
    // r2: new value
    ".p2align(8)                         \n"
    "  ldr r3, [r0]			 \n"
    "  cmp r3, r1                        \n"
    "  nop                               \n"
    "  streq r2, [r0]                    \n"
    // forward point
    "  moveq r0, #1                      \n"
    "  movne r0, #0                      \n"
    "  mov pc, lr                        \n"

    // compare exchange with old value return
    // r0: memory reference
    // r1: cmp value
    // r2: new value
    ".p2align(8)                         \n"
    "  ldr r3, [r0]			 \n"
    "  cmp r3, r1                        \n"
    "  nop                               \n"
    "  streq r2, [r0]                    \n"
    // forward point
    "  mov r0, r3                        \n"
    "  mov pc, lr                        \n"
    );
