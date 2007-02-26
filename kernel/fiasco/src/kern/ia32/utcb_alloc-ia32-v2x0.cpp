INTERFACE:

#include "types.h"

IMPLEMENTATION[ia32-v2x0]:

#include <flux/x86/seg.h>
#include "kmem.h"

/// dummy
IMPLEMENT 
void Utcb_alloc::init() {}

/** Value for gs
 * @return Value the GS register is to be loaded with.
 */
PUBLIC static inline NEEDS [<flux/x86/seg.h>, "kmem.h"]
Mword Utcb_alloc::gs_value() { return (Kmem::gdt_data_user | SEL_PL_U); }

