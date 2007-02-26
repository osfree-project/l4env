/* IA32 specific */

INTERFACE:

#include <cstddef>		// size_t

#include "types.h"

#include <flux/x86/multiboot.h> // multiboot_info
#include <flux/x86/paging.h> 	// Pd_entry
#include <flux/x86/seg.h>

#include "kern_types.h"
#include "kip.h"		// Kernel_info
#include "config_gdt.h"

/* our own implementation of C++ memory management: disallow dynamic
   allocation (except where class-specific new/delete functions exist) */

// more specialized memory allocation/deallocation functions follow
// below in the "Kmem" namespace

// kernel.ld definitions

#include "linker_syms.h"

struct x86_gate;
struct x86_tss;
struct x86_desc;

/** The system's base facilities for kernel-memory management.
    The kernel memory is a singleton object.  We access it through a
    static class interface. */
EXTENSION class Kmem
{
  friend class Vmem_alloc;

public:
  enum {
    /*
     * Kernel memory layout. Some other values are defined in kernel.ia32.ld
     */
    _mappings_1_addr     = 0xe0000000,
    _mappings_end_1_addr = 0xea000000,		/* XXX old code uses _unused1_1 as
						   end-of-region */
    _unused1_1_addr      = _mappings_end_1_addr,/* assumption: 4MB-aligned */
    _unused2_1_addr      = 0xea400000,		/* assumption: 4MB-aligned */
    _unused3_1_addr      = 0xea800000,		/* assumption: 4MB-aligned */
    _service_addr        = 0xeac00000,		/* assumption: 4MB-aligned */
    _smas_start_1_addr   = 0xeb000000,		/* assumption: 4MB-aligned */
    _smas_end_1_addr     = 0xee000000,		/* assumption: 4MB-aligned */
    _ipc_window0_1_addr  = 0xee000000,		/* assumption: 8MB-aligned */
    _ipc_window1_1_addr  = 0xee800000,		/* assumption: imm. after window 0 */
    _utcb_ptr_addr	 = 0xeacfd000,
    _idt_addr            = 0xeacfe000,
    _smas_version_1_addr = 0xef000000,		/* assumption: 4MB-aligned */
    _smas_area_1_addr    = 0xef400000,		/* assumption: 4MB-aligned */
    _kstatic1_1_addr     = 0xef800000,		/* assumption: 4MB-aligned */
    _iobitmap_1_addr     = 0xefc00000,		/* assumption: 4MB-aligned */
    _unused4_io_1_addr   = 0xefc80000,		/* assumption: 2nd level field in
						   page table for IO bitmap */
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

  /// Service page directory entry (for Local APIC, jdb adapter page)
  static const Address service_page = _service_addr;

  /// page for local APIC
  static const Address local_apic_page = service_page;

  /// page for jdb adapter page
  static const Address jdb_adapter_page = local_apic_page + 0x1000;

  /// status page for trace buffer
  static const Address tbuf_status_page = jdb_adapter_page + 0x1000;

  /// trampoline page for small address spaces
  static const Address smas_trampoline = tbuf_status_page + 0x1000;

  /// area for trace buffer (implemented in jdb_tbuf)
  //  The area has to be aligned in a way that allows to map the buffer as
  //  flexpage between address spaces (current max. size is 2MB -- align 2MB)
  static const Address tbuf_buffer_area = service_page + 0x200000;

  /// Start of first IPC window.
  static const Address ipc_window0 = _ipc_window0_1_addr;

  /// Start of second IPC window.
  static const Address ipc_window1 = _ipc_window1_1_addr;

  /// Start of small address space region.
  static const Address smas_start = _smas_start_1_addr;

  /// End of small address space region.
  static const Address smas_end = _smas_end_1_addr;
  //@}
 
  /// page table entry in service page of 4k window for jdb adapter space
  static Pt_entry *jdb_adapter_pt;

   /** Segment numbers. */
  enum {
    gdt_tss		= GDT_TSS,
    gdt_code_kernel	= GDT_CODE_KERNEL,
    gdt_data_kernel	= GDT_DATA_KERNEL,
    gdt_code_user	= GDT_CODE_USER,
    gdt_data_user	= GDT_DATA_USER,
    gdt_tss_dbf		= GDT_TSS_DBF,
    gdt_max		= GDT_MAX
  };

  template< typename _Ty >
  static _Ty *phys_to_virt( P_ptr<_Ty> ); // physical to kernel-virtual

protected:
  static Pd_entry *kdir asm ("KMEM_KDIR");	///< Kernel page directory
  static Pd_entry cpu_global;	///< Page-table flags used for global entries

private:
  friend class kdb;
  friend class profile;

  Kmem();			// default constructors are undefined
  Kmem(const Kmem&);

  static Address mem_max, _himem;

  static const Pd_entry flag_global = 0x200; // l4-specific pg dir entry flag

  static x86_tss volatile *tss asm ("KMEM_TSS");
  static x86_tss volatile *tss_dbf; ///< task segment for double fault handler
  static x86_desc *gdt asm ("KMEM_GDT");

  static Unsigned8 *io_bitmap_delimiter;

  // ABI specific init methods
  static void setup_kip_abi (Kernel_info *);
};

IMPLEMENTATION[ia32]:

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>

#include <flux/x86/tss.h>
#include <flux/x86/seg.h>
#include <flux/x86/proc_reg.h>
#include <flux/x86/gate_init.h>
#include <flux/x86/base_vm.h>

#include "boot_info.h"
#include "cmdline.h"
#include "config.h"
#include "cpu.h"
#include "globals.h"
#include "l4_types.h"
#include "pic.h"
#include "regdefs.h"

//
// class Kmem
//

// static class variables

Address     Kmem::mem_max, Kmem::_himem;
Pd_entry *Kmem::kdir;
Pd_entry  Kmem::cpu_global;
Pt_entry *Kmem::jdb_adapter_pt;


x86_tss volatile *Kmem::tss;
x86_tss volatile *Kmem::tss_dbf;
x86_desc *Kmem::gdt;

Unsigned8 *Kmem::io_bitmap_delimiter;

extern "C" unsigned dbf_stack_top;
extern "C" void dbf_entry(void);


/** Initialize a new task's page directory from the current master copy
    of kernel page tables (the global page directory).
    @param d pointer to first entry of new page directory */
PUBLIC static
void Kmem::dir_init(Pd_entry *d)
{
  std::memcpy(d + ((mem_user_max >> PDESHIFT) & PDEMASK),
	 kdir + ((mem_user_max >> PDESHIFT) & PDEMASK),
	 ((~0 - mem_user_max + 1) >> PDESHIFT) * sizeof(Pd_entry));
}

/** Flush a TLB entry.
    @param addr virtual address of page whose TLB entry should be flushed */
PUBLIC static
inline void Kmem::tlb_flush(Address addr) // flush tlb at virtual addr
{
  asm volatile
    ("invlpg %0" : : "m" (*(char*)addr) : "memory");
}

/** Flush the whole TLB. */
PUBLIC static
inline void Kmem::tlb_flush()
{
  // x86 way of flushing the whole tlb
  unsigned dummy;
  asm volatile (" mov %%cr3,%0; mov %0,%%cr3 " : "=r"(dummy) );
}

/** Deliver the address of the main task segment. Needed by the double fault
 * handler to see the last task state */
PUBLIC static
inline volatile x86_tss* Kmem::main_tss()
{
  return tss;
}

// 
// ACCESSORS
// 

/** Return virtual address of an IPC window.
    @param win number of IPC window (0 or 1)
    @return IPC window's virtual address */
PUBLIC static inline
Address 
Kmem::ipc_window(unsigned win)
{
  if (win == 0)
    return ipc_window0;

  return ipc_window1;
}

IMPLEMENT inline
Mword
Kmem::is_io_bitmap_page_fault( Address addr, Mword /*error*/ )
{
  return (addr >= Kmem::io_bitmap && addr <= Kmem::io_bitmap +
	  L4_fpage::IO_PORT_MAX / 8);
}

IMPLEMENT inline
Mword
Kmem::is_smas_page_fault( Address addr, Mword /*error*/ )
{
  return (addr >= Kmem::smas_start && addr < Kmem::smas_end);
}

IMPLEMENT inline
Mword
Kmem::is_ipc_page_fault( Address addr, Mword /*error*/ )
{
  return (addr >= Kmem::ipc_window(0) && addr < Kmem::ipc_window(1) +
	  (Config::SUPERPAGE_SIZE << 1));
}

IMPLEMENT inline
Mword
Kmem::is_kmem_page_fault( Address addr, Mword /*error*/ )
{
  return (addr >= mem_user_max);
}

PUBLIC static inline
Address
Kmem::io_bitmap_delimiter_page()
{
  return reinterpret_cast<Address>(io_bitmap_delimiter);
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
  
  return reinterpret_cast<Address>
         (memset(reinterpret_cast<void *>(*border), 0, Config::PAGE_SIZE));
}

// set CS (missing from OSKIT)
#define set_cs(cs) 				\
  asm volatile					\
    ("ljmp %0,$1f \n1:"				\
     : : "i" (cs));

// for startup initialization of Kmem
//STATIC_INITIALIZE_P(Kmem, KMEM_INIT_PRIO);

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

  // address of physical memory in our address space; this variable is
  // shared with the OSKIT
  phys_mem_va = mem_phys;

  // find the highest memory address
  mem_max = 1024 * ( 1024 + Boot_info::mbi_virt()->mem_upper );

  // XXX cannot handle more physical memory than the space we have for
  // mapping it.
  if (mem_max > (1LL << 32) - mem_phys)
    mem_max = (1LL << 32) - mem_phys;

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

  if (Cpu::features() & FEAT_PGE) {
    cpu_global |= INTEL_PDE_GLOBAL;
    set_cr4 (get_cr4() | CR4_PGE);
  }

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
       address += Config::SUPERPAGE_SIZE, kd_entry++)
    {
      if (Cpu::features() & FEAT_PSE)
	{
	  kdir[kd_entry] = address | INTEL_PDE_SUPERPAGE 
	    | INTEL_PDE_VALID | INTEL_PDE_WRITE | INTEL_PDE_REF
	    | INTEL_PDE_MOD | cpu_global;
	}
      else
	{
	  Pt_entry *t = 
	    reinterpret_cast<Pt_entry *>(stupid_alloc(& _himem));

	  kdir[kd_entry] = reinterpret_cast<Address>(t)
	    | INTEL_PDE_VALID | INTEL_PDE_WRITE | INTEL_PDE_REF | cpu_global;
	  for (Address a = address; a < address + Config::SUPERPAGE_SIZE;
	       a += Config::PAGE_SIZE)
	    {
	      t[(a >> PTESHIFT) & PTEMASK] = a | INTEL_PTE_VALID 
		| INTEL_PTE_WRITE | INTEL_PTE_REF
		| INTEL_PTE_MOD | cpu_global;
	    }
	}

      // add a one-to-one mapping
      kdir[(address >> PDESHIFT) & PDEMASK] = kdir[kd_entry];
    }

  // The service page directory entry points to an universal usable
  // page table which is currently used for the Local APIC and the 
  // jdb adapter page.
  assert((service_page & ~Config::SUPERPAGE_MASK) == 0);
  
  Pt_entry * t;

  t = reinterpret_cast<Pt_entry *>(stupid_alloc(&_himem));
  kdir[(service_page >> PDESHIFT) & PDEMASK] = reinterpret_cast<Address>(t)
    | INTEL_PDE_VALID | INTEL_PDE_WRITE | INTEL_PDE_REF | INTEL_PDE_USER
    | cpu_global;

  //Set small space variables. Serves as default for all address spaces.
  //(default being big address space and version 0)
  //Sorry about the bare numbers.
  //See corresponding code in space_context.
  if (Config::USE_SMALL_SPACES) 
  {
    kdir[(_smas_area_1_addr >> PDESHIFT) & PDEMASK]  
             = ((mem_user_max - 1) >> 12) & 0xFFF00;
    kdir[(_smas_version_1_addr >> PDESHIFT) &        PDEMASK] = 0;
  }

  // set page table entry for local APIC register page
  t[(local_apic_page >> PTESHIFT) & PTEMASK] = local_apic_page
    | INTEL_PTE_VALID | INTEL_PTE_WRITE |  INTEL_PTE_WTHRU | INTEL_PTE_NCACHE 
    | INTEL_PTE_REF | INTEL_PTE_MOD | cpu_global;

  // jdb needs an universal page table entry for accesses to arbitrary 
  // physcial addresses
  jdb_adapter_pt = reinterpret_cast<Pt_entry*>
                     (phys_to_virt(reinterpret_cast<Address>
		       (t + ((jdb_adapter_page >> PTESHIFT) & PTEMASK))));

  // kernel mode should acknowledge write-protected page table entries
  set_cr0(get_cr0() | CR0_WP);

  // now switch to our new page table
  set_pdbr(virt_to_phys(kdir));

  // map the cpu_page we allocated earlier just before io_bitmap
  Address cpu_page_vm;

  assert((io_bitmap & ~Config::SUPERPAGE_MASK) == 0);

  if ((Cpu::features() & FEAT_PSE)
      && Config::SUPERPAGE_SIZE - (cpu_page&~Config::SUPERPAGE_MASK) < 0x10000)
    {
      // can map as 4MB page because the cpu_page will land within a
      // 16-bit range from io_bitmap
      kdir[((io_bitmap >> PDESHIFT) & PDEMASK) - 1]
	= (cpu_page & Config::SUPERPAGE_MASK) | INTEL_PDE_SUPERPAGE 
	  | INTEL_PDE_VALID | INTEL_PDE_WRITE | INTEL_PDE_REF
	  | INTEL_PDE_MOD | cpu_global;

      cpu_page_vm = (cpu_page & ~Config::SUPERPAGE_MASK) 
	+ (io_bitmap - Config::SUPERPAGE_SIZE);
    }
  else
    {
      Pt_entry *t = reinterpret_cast<Pt_entry *>(stupid_alloc(& _himem));
      kdir[((io_bitmap >> PDESHIFT) & PDEMASK) - 1]
	= reinterpret_cast<Address>(t)
	  | INTEL_PDE_VALID | INTEL_PDE_WRITE | INTEL_PDE_REF | cpu_global;
      
      t[PTEMASK] = cpu_page | INTEL_PTE_VALID 
	| INTEL_PTE_WRITE | INTEL_PTE_REF
	| INTEL_PTE_MOD | cpu_global;

      cpu_page_vm = io_bitmap - Config::PAGE_SIZE;
    }

  // set up the x86 CPU's memory model
  pseudo_descriptor desc;

  if(Config::enable_io_protection)
    {
      // the IO bitmap must be followed by one byte containing 0xff
      // if this byte is not present, then one gets page faults 
      // (or general protection) when accessing the last port
      // at least on a Pentium 133.
      //
      // Therefore we write 0xff in the first byte of the cpu_page 
      // and map this page behind every IO bitmap
      io_bitmap_delimiter = 
	reinterpret_cast<Unsigned8 *>(alloc_from_page(& cpu_page_vm, 1));

      // did we really get the first byte ??
      assert((reinterpret_cast<Address>(io_bitmap_delimiter) 
	       & ~Config::PAGE_MASK) == 0);
      *io_bitmap_delimiter = 0xff;
    }

  // now initialize the global descriptor table
  gdt = reinterpret_cast<x86_desc *>(alloc_from_page(& cpu_page_vm, gdt_max));

  // allocate the task segment for the double fault handler
  tss_dbf = reinterpret_cast<x86_tss volatile*>
    (alloc_from_page(& cpu_page_vm, sizeof(x86_tss)));

  // allocate the task segment as the last thing from cpu_page_vm
  // because with IO protection enabled the task segment includes the 
  // rest of the page and the following IO bitmat (2 pages).
  // 
  // Allocate additional 256 bytes for emergency stack right beneath
  // the tss. It is needed if we get an NMI or debug exception at
  // do_sysenter/do_sysenter_c/do_sysenter_log.
  tss = reinterpret_cast<x86_tss volatile*>
    (alloc_from_page(& cpu_page_vm, sizeof(x86_tss) + 256) + 256);

  // XXX Just before the IO bitmap there are 32 bytes interrupt redirection 
  // XXX map for V86 mode.
  // XXX do something about it

  cpu_page_vm = 0;		// disable allocation from cpu_page_vm
  
  // make sure kernel cs/ds and user cs/ds are placed in the same
  // cache line, respectively; pre-set all "accessed" flags so that
  // the CPU doesn't need to do this later
  if (Config::enable_io_protection)
    {
      Address tss_size = io_bitmap + (L4_fpage::IO_PORT_MAX / 8) 
		       - reinterpret_cast<Address>(tss);
      assert(tss_size < 0x100000); // must fit in 20 Bits
      fill_descriptor(gdt + gdt_tss/8, reinterpret_cast<Address>(tss), 
		      // this is actually tss_size +1, including the
		      // io_bitmap_delimiter byte 
		      tss_size, ACC_PL_K | ACC_TSS, 0);
    }
  else
    {
      fill_descriptor(gdt + gdt_tss/8, reinterpret_cast<Address>(tss), 
		      sizeof(x86_tss) - 1, ACC_PL_K | ACC_TSS, 0);
    }

  create_gdt_entries(gdt);

  // gdt entry for double fault handler
  fill_descriptor(gdt + gdt_tss_dbf/8, reinterpret_cast<Address>(tss_dbf),
		  sizeof(x86_tss) - 1, ACC_PL_K | ACC_TSS | ACC_A, 0);

  desc.limit       = gdt_max - 1;
  desc.linear_base = reinterpret_cast<Address>(gdt);
  set_gdt(&desc);
  set_ldt(0);

  set_ds(data_segment(true));
  set_es(data_segment(true));
  set_fs(gdt_data_user | SEL_PL_U);
  set_ss(gdt_data_kernel);
  set_cs(gdt_code_kernel);
  set_gs(gdt_data_user | SEL_PL_U);

  // and finally initialize the TSS
  tss->ss0 = gdt_data_kernel;
  tss->io_bit_map_offset = io_bitmap - reinterpret_cast<Address>(tss);
  set_tr(gdt_tss);

  // initialize double fault handler task segment
  tss_dbf->cs     = gdt_code_kernel;
  tss_dbf->ss     = gdt_data_kernel;
  tss_dbf->ds     = gdt_data_kernel;
  tss_dbf->es     = gdt_data_kernel;
  tss_dbf->eip    = (unsigned)dbf_entry;
  tss_dbf->ss0    = gdt_data_kernel;
  tss_dbf->esp0   = (unsigned)&dbf_stack_top;
  tss_dbf->ldt    = 0;
  tss_dbf->eflags = 0x00000082;
  tss_dbf->cr3    = virt_to_phys(kdir);
  tss_dbf->io_bit_map_offset = 0x8000;

  Cpu::init_sysenter(reinterpret_cast<Address>(kernel_esp()));

  // CPU initialization done

  // allocate the kernel info page
  Kernel_info *kinfo = static_cast<Kernel_info*>
    (phys_to_virt (stupid_alloc (& _himem)));

  Kernel_info::init_kip (kinfo);

  // initialize kernel info page from prototype
  char *sub = strstr (Cmdline::cmdline(), " proto=");
  if (sub)
    {
      Address proto;
      proto = strtoul(sub + 7, 0, 0);
      if (proto)
	{
	  std::memcpy(kinfo, phys_to_virt(proto), Config::PAGE_SIZE);
	}
    }

  // call ia32-ux-v2-x0-v4 generic KIP setup function
  setup_kip_generic (kinfo);

  // call ABI specific function to finish KIP setup
  setup_kip_abi (kinfo);
}

PUBLIC static inline NEEDS [<flux/x86/seg.h>]
x86_desc * Kmem::get_gdt()
{ return gdt; }

