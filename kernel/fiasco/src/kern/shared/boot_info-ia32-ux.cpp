
INTERFACE:

#include <flux/x86/multiboot.h>
#include "types.h"

EXTENSION class Boot_info
{
public:

  static Address mbi_phys();

  static multiboot_info * const mbi_virt();

  /**
   * @brief Return base address of the kernel memory allocator
   */
  static Address kmem_start(Address mem_max);
};

IMPLEMENTATION[ia32-ux]:

#include "config.h"

IMPLEMENT inline NEEDS ["config.h"]
Address
Boot_info::kmem_start(Address mem_max)
{
  Address end_addr = mbi_virt()->mem_upper << 10;
  if (end_addr > mem_max)
    end_addr = mem_max;
  unsigned size = end_addr / 100 * Config::kernel_mem_per_cent;

  if (size > Config::kernel_mem_max)
    size = Config::kernel_mem_max;

  return end_addr - size & Config::PAGE_MASK;
}
