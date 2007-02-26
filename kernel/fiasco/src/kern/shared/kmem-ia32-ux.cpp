
INTERFACE:

#include "kip.h"

class Kmem
{

  friend class Jdb;
  friend class Jdb_kern_info;

private:
  
  static Address stupid_alloc( Address *border );
  static Address alloc_from_page( Address *from, Address size );

public:
  static void init();
  static Kernel_info *info();
  static const pd_entry_t *dir();
  static pd_entry_t pde_global();
  static Address himem();
  static Address volatile *kernel_esp();

  static Address virt_to_phys( const void *addr );
  static void *linear_virt_to_phys( const void *addr );
  static void *phys_to_virt( Address addr );

  static bool tcbs_fault_addr( Address addr);
  static bool iobm_fault_addr( Address addr);
  static bool smas_fault_addr( Address addr);
  static bool ipcw_fault_addr( Address addr, Mword error );
  static bool user_fault_addr( Address addr, Mword error );
  static bool pagein_tcb_request( Address eip );
};

typedef Kmem Kmem_space;

IMPLEMENTATION[ia32-ux]:

IMPLEMENT inline                                              
bool
Kmem::tcbs_fault_addr (Address addr)
{
  return (addr >= Kmem::mem_tcbs && addr < Kmem::mem_tcbs +
                                           (Config::thread_block_size << 18));
}

/**
 * Return Kernel info page.
 * @return kernel info page
 */
IMPLEMENT inline
Kernel_info *
Kmem::info()
{  
  return Kernel_info::kip();
}

/**
 * Return Global page directory.
 * This is the master copy of the kernel's page directory. Kernel-memory
 * allocations are kept here and copied to task page directories lazily
 * upon page fault.
 * @return kernel's global page directory
 */
IMPLEMENT inline
const pd_entry_t *
Kmem::dir()
{
  return kdir;
}

/**
 * Flags for global page-table and page-directory entries.
 * Global entries are entries that are not automatically flushed when
 * the page-table base register is reloaded. They are intended
 * for kernel data that is shared between all tasks.
 * @return global page-table--entry flags
 */    
IMPLEMENT inline
pd_entry_t
Kmem::pde_global()
{
  return cpu_global;
}
 
IMPLEMENT inline
Address
Kmem::himem()
{
  return _himem;
}

/**
 * Get a pointer to the CPUs kernel stack pointer
 */
IMPLEMENT inline NEEDS [<flux/x86/tss.h>]
Address volatile *
Kmem::kernel_esp()
{
  return reinterpret_cast<Address volatile *>(&tss->esp0);
}

/**
 * Compute physical address from a kernel-virtual address.
 * @param addr a virtual address
 * @return corresponding physical address if a mappings exists.
 *         0xffffffff otherwise.
 */
IMPLEMENT
Address
Kmem::virt_to_phys(const void *addr)
{
  Address a = reinterpret_cast<Address>(addr);

  if ((a & 0xf0000000) == mem_phys)
    return a - mem_phys;

  pd_entry_t p = kdir[(a >> PDESHIFT) & PDEMASK];

  if (!(p & INTEL_PDE_VALID))
    return 0xffffffff;

  if (p & INTEL_PDE_SUPERPAGE)
    return (p & Config::SUPERPAGE_MASK) | (a & ~Config::SUPERPAGE_MASK);

  pt_entry_t t = reinterpret_cast<pt_entry_t *>
    ((p & Config::PAGE_MASK) + mem_phys)[(a >> PTESHIFT) & PTEMASK];
 
  return (t & INTEL_PTE_VALID) ?
         (t & Config::PAGE_MASK) | (a & ~Config::PAGE_MASK) : 0xffffffff;
}

IMPLEMENT inline
void * 
Kmem::linear_virt_to_phys(const void *addr)
{
  Address a = reinterpret_cast<Address>(addr);

  return (void*)(a - mem_phys);
}

/**
 * Compute a kernel-virtual address for a physical address.
 * This function always returns virtual addresses within the
 * physical-memory region.
 * @pre addr <= highest kernel-accessible RAM address
 * @param addr a physical address
 * @return physical address.
 */
IMPLEMENT inline
void *
Kmem::phys_to_virt(Address addr)
{
  return reinterpret_cast<void *>(addr + mem_phys);
}

/**
 * Allocate some bytes from a memory page
 */
IMPLEMENT inline
Address
Kmem::alloc_from_page(Address *from, Address size)
{
  Address ret = *from;

  *from += (size + 0xf) & ~0xf;

  return ret;
}
