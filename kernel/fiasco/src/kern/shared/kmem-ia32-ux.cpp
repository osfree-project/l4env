// our own implementation of C++ memory management: disallow dynamic
// allocation (except where class-specific new/delete functions exist)
//
// more specialized memory allocation/deallocation functions follow
// below in the "Kmem" namespace

INTERFACE [ia32,ux]:

#include "initcalls.h"
#include "kern_types.h"
#include "kip.h"
#include "mem_layout.h"

class Pdir;
class Tss;

/**
 * The system's base facilities for kernel-memory management.
 * The kernel memory is a singleton object.  We access it through a
 * static class interface.
 */
class Kmem
{
  friend class Jdb;
  friend class Jdb_dbinfo;
  friend class Jdb_kern_info_misc;
  friend class Kdb;
  friend class Profile;
  friend class Vmem_alloc;

private:
  Kmem();			// default constructors are undefined
  Kmem (const Kmem&);
  static Address mem_max, _himem;

public:
  enum 
  {
    mem_tcbs     = Mem_layout::Tcbs,
    mem_user_max = Mem_layout::User_max,
  };

  static void init();
  static Mword is_kmem_page_fault(Mword pfa, Mword error);
  static Mword is_ipc_page_fault(Mword pfa, Mword error);
  static Mword is_smas_page_fault(Mword pfa);
  static Mword is_io_bitmap_page_fault(Mword pfa);
  static Address kcode_start();
  static Address kcode_end();
  static Address kmem_base();
  static Address virt_to_phys(const void *addr);
};

typedef Kmem Kmem_space;

IMPLEMENTATION [ia32,ux]:

#include <cstdio>
#include <cstdlib>
#include <cstddef>		// size_t
#include <cstring>		// memset

#include "boot_info.h"
#include "config.h"
#include "cpu.h"
#include "gdt.h"
#include "globals.h"
#include "paging.h"
#include "regdefs.h"
#include "std_macros.h"
#include "tss.h"

// static class variables
Address       Kmem::mem_max, Kmem::_himem;
Pdir         *Kmem::kdir;

PUBLIC static inline                                              
Mword
Kmem::is_tcb_page_fault( Address addr, Mword /*error*/ )
{
  return (   addr >= Mem_layout::Tcbs
	  && addr <  Mem_layout::Tcbs
		     + (Config::thread_block_size 
			* Config::max_threads()) );
}

/**
 * Return Global page directory.
 * This is the master copy of the kernel's page directory. Kernel-memory
 * allocations are kept here and copied to task page directories lazily
 * upon page fault.
 * @return kernel's global page directory
 */
PUBLIC static inline const Pdir* Kmem::dir() { return kdir; }

PUBLIC static inline Address Kmem::get_mem_max() { return mem_max; }
PUBLIC static inline Address Kmem::himem() { return _himem; }

/**
 * Get a pointer to the CPUs kernel stack pointer
 */
PUBLIC static inline NEEDS ["cpu.h", "tss.h"]
Address volatile *
Kmem::kernel_esp()
{
  return reinterpret_cast<Address volatile *>(&Cpu::get_tss()->esp0);
}

/**
 * Compute a kernel-virtual address for a physical address.
 * This function always returns virtual addresses within the
 * physical-memory region.
 * @pre addr <= highest kernel-accessible RAM address
 * @param addr a physical address
 * @return kernel-virtual address.
 */
PUBLIC static inline
void *
Kmem::phys_to_virt(Address addr)
{
  return reinterpret_cast<void *>(Mem_layout::phys_to_pmem(addr));
}

/** Allocate some bytes from a memory page */
PRIVATE static inline
Address
Kmem::alloc_from_page(Address *from, Address size)
{
  Address ret = *from;
  *from += (size + 0xf) & ~0xf;

  return ret;
}

PRIVATE static FIASCO_INIT
Address
Kmem::himem_alloc()
{
  _himem -= Config::PAGE_SIZE;
  memset (Kmem::phys_to_virt (_himem), 0, Config::PAGE_SIZE);

  return _himem;
}

