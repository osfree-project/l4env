
INTERFACE:

#include "kip.h"

class Kmem
{
  friend class Jdb;
  friend class Jdb_kern_info_misc;

private:
  static Address stupid_alloc( Address *border );
  static Address alloc_from_page( Address *from, Address size );
  static void setup_gs0_page();
  static void setup_kip_generic (Kernel_info *);

public:
  static void init();
  static Kernel_info *info();
  static const Pd_entry *dir();
  static Pd_entry pde_global();
  static Address himem();
  static Address volatile *kernel_esp();

  static Address virt_to_phys( const void *addr );
  static void *linear_virt_to_phys( const void *addr );
  static void *phys_to_virt( Address addr );

  static Mword is_kmem_page_fault( Mword pfa, Mword error );
  static Mword is_tcb_page_fault( Mword pfa, Mword error );
  static Mword is_ipc_page_fault( Mword pfa, Mword error );
  static Mword is_smas_page_fault( Mword pfa, Mword error );
  static Mword is_io_bitmap_page_fault( Mword pfa, Mword error );
};

typedef Kmem Kmem_space;

IMPLEMENTATION[ia32-ux]:

IMPLEMENT inline                                              
Mword Kmem::is_tcb_page_fault( Address addr, Mword /*error*/ )
{
  return (   addr >= Kmem::mem_tcbs
	  && addr <  Kmem::mem_tcbs
		     + (Config::thread_block_size 
			* Kmem::info()->max_threads()) );
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
const Pd_entry *
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
Pd_entry
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
 *         -1 otherwise.
 */
IMPLEMENT
Address
Kmem::virt_to_phys (const void *addr)
{
  Address a = reinterpret_cast<Address>(addr);

  if ((a & 0xf0000000) == mem_phys)
    return a - mem_phys;

  Pd_entry p = kdir[(a >> PDESHIFT) & PDEMASK];

  if (!(p & INTEL_PDE_VALID))
    return (Address) -1;

  if (p & INTEL_PDE_SUPERPAGE)
    return (p & Config::SUPERPAGE_MASK) | (a & ~Config::SUPERPAGE_MASK);

  Pt_entry t = reinterpret_cast<Pt_entry *>
    ((p & Config::PAGE_MASK) + mem_phys)[(a >> PTESHIFT) & PTEMASK];
 
  return (t & INTEL_PTE_VALID) ?
         (t & Config::PAGE_MASK) | (a & ~Config::PAGE_MASK) : (Address) -1;
}

IMPLEMENT inline
void * 
Kmem::linear_virt_to_phys (const void *addr)
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

/**
 * ABI and ia32/ux generic KIP initialization code
 */
IMPLEMENT FIASCO_INIT
void
Kmem::setup_kip_generic (Kernel_info* kinfo)
{
  kinfo->magic		= L4_KERNEL_INFO_MAGIC;
  kinfo->version	= Config::kernel_version_id;
  kinfo->frequency_cpu  = Cpu::frequency() / 1000;

  kinfo->offset_version_strings = 0x10;
  strcpy(reinterpret_cast<char*>(kinfo) 
	 + (kinfo->offset_version_strings << 4), 
	 Config::kernel_version_string);
}
