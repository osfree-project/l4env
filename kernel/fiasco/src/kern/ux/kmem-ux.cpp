/* IA32 specific */

INTERFACE:

#include <flux/x86/multiboot.h> // multiboot_info
#include <flux/x86/paging.h> // pd_entry_t
#include <cstddef>		// size_t
#include "config_gdt.h"
#include "kip.h"	// kernel_info_t
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
  /** @name Kernel-virtual address space.
      These constants are derived from declarations in kernel.ld. */
  //@{
  /// Start of physical-memory region.
  static const vm_offset_t mem_phys = reinterpret_cast<vm_offset_t>(&_physmem_1);
  /// Start of region for thread-control blocks (thread_t's).
  static const vm_offset_t mem_tcbs = reinterpret_cast<vm_offset_t>(&_tcbs_1);
  /// End of user-specific virtual-memory region.
  static const vm_offset_t mem_user_max = 0xc0000000;
  /// Start of I/O bitmap.
  static const vm_offset_t io_bitmap = 
     reinterpret_cast<vm_offset_t>(&_iobitmap_1);

  /// Service page directory entry (for Local APIC, jdb adapter page)
  static const vm_offset_t service_page
    = reinterpret_cast<vm_offset_t>(&_service);
  /// page for local APIC
  static const vm_offset_t local_apic_page = service_page;
  /// page for jdb adapter page
  static const vm_offset_t jdb_adapter_page = local_apic_page + 0x1000;
  /// status page for trace buffer
  static const vm_offset_t tbuf_status_page = jdb_adapter_page + 0x1000;
  /// area for trace buffer (implemented in jdb_tbuf)
  //  The area has to be aligned in a way that allows to map the buffer as
  //  flexpage between address spaces (current max. size is 2MB -- align 2MB)
  static const vm_offset_t tbuf_buffer_area = service_page + 0x200000;
  /// Start of first IPC window.
  /// Start of second IPC window.
  //@}
 
  /// page table entry in service page of 4k window for jdb adapter space
  static pt_entry_t *jdb_adapter_pt;

  /** Segment numbers. */
  enum { gdt_tss = GDT_TSS, 
    gdt_code_kernel = GDT_CODE_KERNEL, gdt_data_kernel = GDT_DATA_KERNEL,
    gdt_code_user = GDT_CODE_USER, gdt_data_user = GDT_DATA_USER, 
    gdt_max = GDT_MAX };

protected:
  static pd_entry_t *kdir;	///< Kernel page directory
  static pd_entry_t cpu_global;	///< Page-table flags used for global entries

private:
  friend class kdb;
  friend class jdb;
  friend class profile;
  friend class Vmem_alloc;

  Kmem();			// default constructors are undefined
  Kmem(const Kmem&);

  static vm_offset_t mem_max, _himem;

  static const pd_entry_t flag_global = 0x200; // l4-specific pg dir entry flag

  static x86_tss volatile *tss asm ("KMEM_TSS");
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
#include "undef_oskit.h"

//
// class Kmem
//

// static class variables

vm_offset_t Kmem::mem_max, Kmem::_himem;
pd_entry_t *Kmem::kdir;
pd_entry_t  Kmem::cpu_global;
pt_entry_t *Kmem::jdb_adapter_pt;


x86_tss volatile *Kmem::tss;

extern char _stext, _end;

/** Flush a TLB entry.
    @param addr virtual address of page whose TLB entry should be flushed */
PUBLIC static
inline void Kmem::tlb_flush (vm_offset_t) // flush tlb at virtual addr
{}

/** Flush the whole TLB. */
PUBLIC static
inline void Kmem::tlb_flush()
{}

// 
// ACCESSORS
// 

IMPLEMENT inline
bool
Kmem::iobm_fault_addr (Address)
{
  return false;
}

IMPLEMENT inline
bool
Kmem::smas_fault_addr (Address)
{
  return false;
}

IMPLEMENT inline
bool
Kmem::ipcw_fault_addr (Address addr, unsigned error)
{
  return addr < mem_user_max && (error & PF_ERR_REMTADDR);
}

IMPLEMENT inline NEEDS ["regdefs.h"]
bool
Kmem::user_fault_addr (Address addr, unsigned error)
{
  return addr < mem_user_max && (error & PF_ERR_USERADDR);
}

IMPLEMENT inline
bool
Kmem::pagein_tcb_request (Address eip)
{
#ifdef CONFIG_ASSEMBLER_IPC_SHORTCUT
  extern Mword pagein_tcb_request1;
#endif
  extern Mword pagein_tcb_request3;

  return (
#ifdef CONFIG_ASSEMBLER_IPC_SHORTCUT
          eip == (Address) &pagein_tcb_request1 || 
#endif
          eip == (Address) &pagein_tcb_request3);
}

//
// helper functions
//

// allocate a page from the end of the physical memory
IMPLEMENT inline NEEDS [<cstring>, "config.h", "undef_oskit.h"]
vm_offset_t
Kmem::stupid_alloc(vm_offset_t *border)
{
  *border -= Config::PAGE_SIZE;
  
  memset (reinterpret_cast<void *>(Kmem::phys_to_virt (*border)), 0, Config::PAGE_SIZE);

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
  vm_offset_t cpu_page = stupid_alloc(& _himem);

  kdir = static_cast<pd_entry_t *>(phys_to_virt(stupid_alloc(& _himem)));
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

  for (vm_offset_t address = 0; address < mem_max; address += Config::SUPERPAGE_SIZE, kd_entry++) {

    if (Cpu::features() & FEAT_PSE)
      kdir[kd_entry] = address | INTEL_PDE_SUPERPAGE | INTEL_PDE_VALID | INTEL_PDE_WRITE | INTEL_PDE_REF | INTEL_PDE_MOD | cpu_global;

    else {
      pt_entry_t *t =  reinterpret_cast<pt_entry_t *>(stupid_alloc(& _himem));

      kdir[kd_entry] = reinterpret_cast<vm_offset_t>(t) | INTEL_PDE_VALID | INTEL_PDE_WRITE | INTEL_PDE_REF | cpu_global;

      for (vm_offset_t a = address; a < address + Config::SUPERPAGE_SIZE; a += Config::PAGE_SIZE)
        reinterpret_cast<pt_entry_t *>(phys_to_virt (reinterpret_cast<vm_offset_t>(t)))[(a >> PTESHIFT) & PTEMASK] =
	a | INTEL_PTE_VALID | INTEL_PTE_WRITE | INTEL_PTE_REF | INTEL_PTE_MOD | cpu_global;
    }
  }

#if 0
  // The service page directory entry points to an universal usable
  // page table which is currently used for the Local APIC and the 
  // jdb adapter page.
  assert((service_page & ~Config::SUPERPAGE_MASK) == 0);
  
  pt_entry_t * t;

  t = reinterpret_cast<pt_entry_t *>(stupid_alloc(&_himem));
  kdir[(service_page >> PDESHIFT) & PDEMASK] = reinterpret_cast<vm_offset_t>(t)
    | INTEL_PDE_VALID | INTEL_PDE_WRITE | INTEL_PDE_REF | INTEL_PDE_USER
    | cpu_global;

  // set page table entry for local APIC register page
  t[(local_apic_page >> PTESHIFT) & PTEMASK] = local_apic_page
    | INTEL_PTE_VALID | INTEL_PTE_WRITE |  INTEL_PTE_WTHRU | INTEL_PTE_NCACHE 
    | INTEL_PTE_REF | INTEL_PTE_MOD | cpu_global;

  // jdb needs an universal page table entry for accesses to arbitrary 
  // physcial addresses
  jdb_adapter_pt = reinterpret_cast<pt_entry_t*>
                     (phys_to_virt(reinterpret_cast<vm_offset_t>
		       (t + ((jdb_adapter_page >> PTESHIFT) & PTEMASK))));

  // kernel mode should acknowledge write-protected page table entries
  set_cr0(get_cr0() | CR0_WP);
#endif

  // now switch to our new page table
  Emulation::set_pdir_addr (virt_to_phys (kdir));

  // map the cpu_page we allocated earlier just before io_bitmap
  vm_offset_t cpu_page_vm;

  assert((io_bitmap & ~Config::SUPERPAGE_MASK) == 0);

  if ((Cpu::features() & FEAT_PSE) && Config::SUPERPAGE_SIZE - (cpu_page & ~Config::SUPERPAGE_MASK) < 0x10000) {
    // can map as 4MB page because the cpu_page will land within a
    // 16-bit range from io_bitmap
    kdir[((reinterpret_cast<vm_offset_t>(io_bitmap) >> PDESHIFT) & PDEMASK) - 1] =
   (cpu_page & Config::SUPERPAGE_MASK) | INTEL_PDE_SUPERPAGE | INTEL_PDE_VALID | INTEL_PDE_WRITE | INTEL_PDE_REF | INTEL_PDE_MOD | cpu_global;

    cpu_page_vm = (cpu_page & ~Config::SUPERPAGE_MASK) + (io_bitmap - Config::SUPERPAGE_SIZE);

  } else {

    pt_entry_t *t = reinterpret_cast<pt_entry_t *>(stupid_alloc(& _himem));

    kdir[((io_bitmap >> PDESHIFT) & PDEMASK) - 1] =
    reinterpret_cast<vm_offset_t>(t) | INTEL_PDE_VALID | INTEL_PDE_WRITE | INTEL_PDE_REF | cpu_global;

    reinterpret_cast<pt_entry_t *>(phys_to_virt (reinterpret_cast<vm_offset_t>(t)))[PTEMASK] =        
    cpu_page | INTEL_PDE_VALID | INTEL_PDE_WRITE | INTEL_PDE_REF | INTEL_PDE_MOD | cpu_global;
      
    cpu_page_vm = io_bitmap - Config::PAGE_SIZE;
  }

  if (mmap ((void *) cpu_page_vm, Config::PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, Boot_info::fd(), cpu_page) == MAP_FAILED)
    printf ("CPU page mapping failed: %s\n", strerror (errno));

  // allocate the task segment as the last thing from cpu_page_vm
  // because with IO protection enabled the task segment includes the 
  // rest of the page and the following IO bitmat (2 pages).
  tss = reinterpret_cast<x86_tss *>(alloc_from_page(& cpu_page_vm, sizeof(x86_tss)));

  // and finally initialize the TSS
  tss->ss0 = gdt_data_kernel;
  //  tss->io_bit_map_offset = io_bitmap - reinterpret_cast<vm_offset_t>(tss);

  // CPU initialization done

  // allocate the kernel info page

  Kernel_info *kinfo = static_cast<Kernel_info*>
                       (phys_to_virt (stupid_alloc (& _himem)));

  Kernel_info::init_kip (kinfo);

  // now define space for the kernel memory allocator
  void *kmem_base = reinterpret_cast<void*> ((Mword)(phys_to_virt (mem_max - mem_max / 100 * Config::kernel_mem_per_cent)) & Config::PAGE_MASK);

  kinfo->magic			= L4_KERNEL_INFO_MAGIC;
  kinfo->version		= Config::kernel_version_id;
  kinfo->main_memory.low	= 0;
  kinfo->main_memory.high	= mem_max;
  kinfo->reserved0.low		= 0;				/* Kernel Reserved (3 pages) */
  kinfo->reserved0.high		= 0x3000;			/* Multiboot, Trampoline, Sigstack */
  kinfo->reserved1.low		= virt_to_phys(kmem_base);	/* Kernel Allocator Space */
  kinfo->reserved1.high		= mem_max;
  kinfo->semi_reserved.low	= 1024 * Boot_info::mbi_virt()->mem_lower;
  kinfo->semi_reserved.high	= 1024 * 1024;
  kinfo->offset_version_strings = 0x10;
  strcpy(reinterpret_cast<char*>(kinfo) + (kinfo->offset_version_strings << 4), Config::kernel_version_string);
  kinfo->clock			= 0;

  struct multiboot_module *mbm  = reinterpret_cast<multiboot_module*>(Kmem::phys_to_virt (Boot_info::mbi_virt()->mods_addr));

  kinfo->sigma0_eip             =  (reinterpret_cast<multiboot_module *>(mbm + 0))->reserved;
  kinfo->sigma0_memory.low      =  (reinterpret_cast<multiboot_module *>(mbm + 0))->mod_start & Config::PAGE_MASK;
  kinfo->sigma0_memory.high     = ((reinterpret_cast<multiboot_module *>(mbm + 0))->mod_end + (Config::PAGE_SIZE-1)) & Config::PAGE_MASK;
  kinfo->root_eip               =  (reinterpret_cast<multiboot_module *>(mbm + 1))->reserved;
  kinfo->root_memory.low        =  (reinterpret_cast<multiboot_module *>(mbm + 1))->mod_start & Config::PAGE_MASK;
  kinfo->root_memory.high       = ((reinterpret_cast<multiboot_module *>(mbm + 1))->mod_end + (Config::PAGE_SIZE-1)) & Config::PAGE_MASK;
  kinfo->dedicated[0].low       = kinfo->root_memory.low;
  kinfo->dedicated[0].high      = kinfo->root_memory.high;

  for (int i = 1; i < 4; i++)
    kinfo->dedicated[i].low = kinfo->dedicated[i].high = 0;         
}
