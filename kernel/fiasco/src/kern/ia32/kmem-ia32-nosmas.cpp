INTERFACE:

IMPLEMENTATION[ia32-nosmas]:

#include <flux/x86/seg.h>

/** Create GDT entries
 */
PUBLIC static inline NEEDS[<flux/x86/seg.h>]
void
Kmem::create_gdt_entries(x86_desc *gdt)
{
  fill_descriptor(gdt + gdt_code_kernel/8, 0, 0xffffffff,
        ACC_PL_K | ACC_CODE_R | ACC_A, SZ_32); 
  fill_descriptor(gdt + gdt_data_kernel/8, 0, 0xffffffff,
        ACC_PL_K | ACC_DATA_W | ACC_A, SZ_32);
  fill_descriptor(gdt + gdt_code_user/8, 0, 0xffffffff,
        ACC_PL_U | ACC_CODE_R | ACC_A, SZ_32);
  fill_descriptor(gdt + gdt_data_user/8, 0, 0xffffffff,
        ACC_PL_U | ACC_DATA_W | ACC_A, SZ_32);
}

/** Returns the data segment to be used.
 */
PUBLIC static inline NEEDS[<flux/x86/seg.h>]
int
Kmem::data_segment (bool /*kernel_mode*/)
{
  return gdt_data_user | SEL_PL_U;
}

