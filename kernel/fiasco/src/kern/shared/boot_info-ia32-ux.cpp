
INTERFACE[ia32,ux]:

#include "multiboot.h"
#include "types.h"


EXTENSION class Boot_info
{
public:

  static Address mbi_phys();

  static Multiboot_info * const mbi_virt();
};

IMPLEMENTATION[ia32,ux]:

#include "config.h"
#include "mem_layout.h"

PUBLIC static inline NEEDS ["config.h","mem_layout.h"]
Address
Boot_info::kmem_start(Address mem_max)
{
  Address end_addr = (mbi_virt()->mem_upper + 1024) << 10;
  Address size, base;

  if (end_addr > mem_max)
    end_addr = mem_max;

  size = kmemsize();
  if (!size)
    {
      size = end_addr / 100 * Config::kernel_mem_per_cent;
      if (size > Config::kernel_mem_max)
	size = Config::kernel_mem_max;
    }

  base = end_addr - size & Config::PAGE_MASK;
  if (Mem_layout::phys_to_pmem(base) < Mem_layout::Physmem)
    base = Mem_layout::pmem_to_phys(Mem_layout::Physmem);

  return base;
}
