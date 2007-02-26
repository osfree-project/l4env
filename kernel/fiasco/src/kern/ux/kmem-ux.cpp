/* IA32 specific */

INTERFACE:

#include <flux/x86/multiboot.h> // multiboot_info
#include <flux/x86/paging.h>	// Pd_entry
#include <cstddef>		// size_t
#include "config_gdt.h"
#include "kip.h"		// Kernel_info
#include "linker_syms.h"
#include "types.h"

/* our own implementation of C++ memory management: disallow dynamic
   allocation (except where class-specific new/delete functions exist) */

// more specialized memory allocation/deallocation functions follow
// below in the "Kmem" namespace

struct x86_gate;
struct x86_tss;
struct x86_desc;

/** The system's base facilities for kernel-memory management.
    The kernel memory is a singleton object.  We access it through a
    static class interface. */
EXTENSION class Kmem
{
public:
  enum {
    /*
     * Kernel memory layout. Some other values are defined in kernel.ux.ld
     */
    _mappings_1_addr	  = 0x50000000,	// Multipage Slabs		
    _mappings_end_1_addr  = 0x5f000000,	// XXX old code end-of-region	
    _idt_addr		  = 0x5f001000,
    tbuf_status_page      = 0x5f002000,	// more or less a dummy for tbuf
    tbuf_buffer_area      = 0x5f200000,	// space for tracebuffers

    _iobitmap_1_addr	  = 0x5fc00000,	// assumption: 4MB-aligned
					// (all following entries)

    _unused1_1_addr	  = 0xc0000000,	// v2: space index
					// v4: thread counter 

    _unused2_1_addr	  = 0xc0400000,	// v2: chief index
                                        // v4: utcb area

    _unused3_1_addr	  = 0xc0800000,	// v2: (unused)
					// v4: kip area

    _unused4_1_addr	  = 0xc0c00000, // v2: host pid
					// v4: host pid
  };

  /** @name Kernel-virtual address space.
      These constants are derived from declarations in kernel.ld. */
  //@{

  /// Start of physical-memory region.
  static const Address mem_phys = reinterpret_cast<Address>(&_physmem_1);

  /// Start of region for thread-control blocks (Thread's).
  static const Address mem_tcbs = reinterpret_cast<Address>(&_tcbs_1);

  /// End of user-specific virtual-memory region.
  static const Address mem_user_max = 0xc0000000;

  /// Start of I/O bitmap.
  static const Address io_bitmap = _iobitmap_1_addr;

  //@}
 
  /** Segment numbers. */
  enum { gdt_tss = GDT_TSS, 
    gdt_code_kernel = GDT_CODE_KERNEL, gdt_data_kernel = GDT_DATA_KERNEL,
    gdt_code_user = GDT_CODE_USER, gdt_data_user = GDT_DATA_USER, 
    gdt_max = GDT_MAX };

protected:
  static Pd_entry *kdir;	///< Kernel page directory
  static Pd_entry cpu_global;	///< Page-table flags used for global entries

private:
  friend class kdb;
  friend class jdb;
  friend class profile;
  friend class Vmem_alloc;

  Kmem();			// default constructors are undefined
  Kmem(const Kmem&);

  static Address mem_max, _himem;

  static const Pd_entry flag_global = 0x200; // l4-specific pg dir entry flag

  static x86_tss volatile *tss asm ("KMEM_TSS");

  // ABI specific init methods
  static void setup_kip_abi (Kernel_info *);
};

IMPLEMENTATION[ux]:

#include <flux/x86/tss.h>
#include <flux/x86/seg.h>
#include <flux/x86/proc_reg.h>
#include <flux/x86/gate_init.h>
#include <flux/x86/base_vm.h>

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>

#include "boot_info.h"
#include "config.h"
#include "cpu.h"
#include "emulation.h"
#include "globals.h"
#include "initcalls.h"
#include "regdefs.h"

//
// class Kmem
//

// static class variables

Address     Kmem::mem_max, Kmem::_himem;
Pd_entry *Kmem::kdir;
Pd_entry  Kmem::cpu_global;


x86_tss volatile *Kmem::tss;

extern char _stext, _end;

/** Flush a TLB entry.
    @param addr virtual address of page whose TLB entry should be flushed */
PUBLIC static
inline void Kmem::tlb_flush (Address) // flush tlb at virtual addr
{}

/** Flush the whole TLB. */
PUBLIC static
inline void Kmem::tlb_flush()
{}

// 
// ACCESSORS
// 

IMPLEMENT inline
Mword Kmem::is_io_bitmap_page_fault( Address, Mword )
{
  return false;
}

IMPLEMENT inline
Mword Kmem::is_smas_page_fault( Address, Mword )
{
  return false;
}

IMPLEMENT inline
Mword Kmem::is_ipc_page_fault( Address addr, Mword error )
{
  return addr < mem_user_max && (error & PF_ERR_REMTADDR);
}

IMPLEMENT inline NEEDS ["regdefs.h"]
Mword Kmem::is_kmem_page_fault( Address addr, Mword error )
{
  return !(addr < mem_user_max && (error & PF_ERR_USERADDR));
}


//
// helper functions
//

// allocate a page from the end of the physical memory
IMPLEMENT inline NEEDS [<cstring>, "config.h"]
Address
Kmem::stupid_alloc(Address *border)
{
  *border -= Config::PAGE_SIZE;
  
  memset (reinterpret_cast<void *>(Kmem::phys_to_virt (*border)), 
	  0, Config::PAGE_SIZE);

  return *border;
}

IMPLEMENT FIASCO_INIT
void
Kmem::init()
{
  // there are several things to note here: first, we run before
  // main() has been started, i.e., before any constructors for static
  // objects have been run.  second, we can assume here that we still
  // have the physical memory mapped in one-to-one from bootstrap.c.
  // third, we can't allocate memory dynamically before we have set up
  // the kernel memory and init page table.
  // we also can assume that the "cpu" global variable has already
  // been initialized.

  // find the highest memory address
  mem_max = Boot_info::mbi_virt()->mem_upper << 10;

  _himem = mem_max;

  // allocate a page for processor data structures.  this page is
  // filled in later; however, because we start allocating from the
  // end of memory, we allocate this page early to increase the chance
  // that it is on the end of a 4MB page.  if this happens, we can map
  // it in as an 4MB page later; otherwise, we need to allocate a page
  // table just for this page.  the reason we need this page near the
  // end is that it must refer to the io_bitmap on the next 4MB-page
  // with a 16-bit pointer.
  Address cpu_page = stupid_alloc(& _himem);

  kdir = static_cast<Pd_entry *>(phys_to_virt(stupid_alloc(& _himem)));
  unsigned kd_entry = mem_phys >> 22;

  cpu_global = flag_global;

  if (Cpu::features() & FEAT_PGE)
    cpu_global |= INTEL_PDE_GLOBAL;

  // set up the kernel mapping for physical memory.  mark all pages as
  // referenced and modified (so when touching the respective pages
  // later, we save the CPU overhead of marking the pd/pt entries like
  // this)

  // we also set up a one-to-one virt-to-phys mapping for two reasons:
  // (1) so that we switch to the new page table early and re-use the
  // segment descriptors set up by bootstrap.c.  (we'll set up our own
  // descriptors later.)  (2) a one-to-one phys-to-virt mapping in the
  // kernel's page directory sometimes comes in handy

  for (Address address = 0; address < mem_max; 
       address += Config::SUPERPAGE_SIZE, kd_entry++) {
    
    if (Cpu::features() & FEAT_PSE)
      kdir[kd_entry] = address | INTEL_PDE_SUPERPAGE | INTEL_PDE_VALID 
	| INTEL_PDE_WRITE | INTEL_PDE_REF | INTEL_PDE_MOD | cpu_global;

    else {
      Pt_entry *t =  reinterpret_cast<Pt_entry *>(stupid_alloc(& _himem));

      kdir[kd_entry] = reinterpret_cast<Address>(t) | INTEL_PDE_VALID 
	| INTEL_PDE_WRITE | INTEL_PDE_REF | cpu_global;

      for (Address a = address; a < address + Config::SUPERPAGE_SIZE; 
	   a += Config::PAGE_SIZE)
        reinterpret_cast<Pt_entry *>
	  (phys_to_virt (reinterpret_cast<Address>(t)))
	  [(a >> PTESHIFT) & PTEMASK] 
	  = a | INTEL_PTE_VALID | INTEL_PTE_WRITE | INTEL_PTE_REF 
	  | INTEL_PTE_MOD | cpu_global;
    }
  }

  // now switch to our new page table
  Emulation::set_pdir_addr (virt_to_phys (kdir));

  // map the cpu_page we allocated earlier just before io_bitmap
  Address cpu_page_vm;

  assert((io_bitmap & ~Config::SUPERPAGE_MASK) == 0);

  if ((Cpu::features() & FEAT_PSE) && Config::SUPERPAGE_SIZE 
      - (cpu_page & ~Config::SUPERPAGE_MASK) < 0x10000) {
    // can map as 4MB page because the cpu_page will land within a
    // 16-bit range from io_bitmap
    kdir[((reinterpret_cast<Address>(io_bitmap) >> PDESHIFT) & PDEMASK) - 1] =
      (cpu_page & Config::SUPERPAGE_MASK) | INTEL_PDE_SUPERPAGE 
      | INTEL_PDE_VALID | INTEL_PDE_WRITE | INTEL_PDE_REF | INTEL_PDE_MOD 
      | cpu_global;

    cpu_page_vm = (cpu_page & ~Config::SUPERPAGE_MASK) 
      + (io_bitmap - Config::SUPERPAGE_SIZE);

  } else {

    Pt_entry *t = reinterpret_cast<Pt_entry *>(stupid_alloc(& _himem));

    kdir[((io_bitmap >> PDESHIFT) & PDEMASK) - 1] 
      = reinterpret_cast<Address>(t) | INTEL_PDE_VALID | INTEL_PDE_WRITE 
      | INTEL_PDE_REF | cpu_global;
    
    reinterpret_cast<Pt_entry *>
      (phys_to_virt (reinterpret_cast<Address>(t)))[PTEMASK] 
      = cpu_page | INTEL_PDE_VALID | INTEL_PDE_WRITE | INTEL_PDE_REF 
      | INTEL_PDE_MOD | cpu_global;
      
    cpu_page_vm = io_bitmap - Config::PAGE_SIZE;
  }

  if (mmap ((void *) cpu_page_vm, Config::PAGE_SIZE, PROT_READ 
	    | PROT_WRITE, MAP_SHARED | MAP_FIXED, Boot_info::fd(), cpu_page) 
      == MAP_FAILED)
    printf ("CPU page mapping failed: %s\n", strerror (errno));

  // allocate the task segment as the last thing from cpu_page_vm
  // because with IO protection enabled the task segment includes the 
  // rest of the page and the following IO bitmat (2 pages).
  tss = reinterpret_cast<x86_tss *>
    (alloc_from_page(& cpu_page_vm, sizeof(x86_tss)));

  // and finally initialize the TSS
  tss->ss0 = gdt_data_kernel;
  //  tss->io_bit_map_offset = io_bitmap - reinterpret_cast<Address>(tss);

  // CPU initialization done

  // allocate the kernel info page
  Kernel_info *kinfo = static_cast<Kernel_info*>
    (phys_to_virt (stupid_alloc (& _himem)));

  Kernel_info::init_kip (kinfo);

  struct multiboot_module *mbm  
    = reinterpret_cast<multiboot_module*>
    (Kmem::phys_to_virt (Boot_info::mbi_virt()->mods_addr));

  kinfo->sigma0_eip
    = (reinterpret_cast<multiboot_module *>(mbm + 0))->reserved;

  kinfo->sigma0_memory.low
    =  (reinterpret_cast<multiboot_module *>(mbm + 0))->mod_start 
    & Config::PAGE_MASK;

  kinfo->sigma0_memory.high
    = ((reinterpret_cast<multiboot_module *>(mbm + 0))->mod_end 
       + (Config::PAGE_SIZE-1)) & Config::PAGE_MASK;

  kinfo->root_eip 
    = (reinterpret_cast<multiboot_module *>(mbm + 1))->reserved;

  kinfo->root_memory.low 
    = (reinterpret_cast<multiboot_module *>(mbm + 1))->mod_start 
    & Config::PAGE_MASK;

  kinfo->root_memory.high 
    = ((reinterpret_cast<multiboot_module *>(mbm + 1))->mod_end 
       + (Config::PAGE_SIZE-1)) & Config::PAGE_MASK;

  // call ia32-ux-v2-x0-v4 generic KIP setup function
  setup_kip_generic (kinfo);

  // call ABI specific function to finish KIP setup
  setup_kip_abi (kinfo);
}

