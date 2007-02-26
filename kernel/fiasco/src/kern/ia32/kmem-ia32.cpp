/* IA32 specific */

INTERFACE:

#include <cstddef>		// size_t

#include "types.h"

#include <flux/x86/multiboot.h> // multiboot_info
#include <flux/x86/paging.h> // pd_entry_t

#include "kern_types.h"

#include "kip.h"	// kernel_info_t

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
  static const vm_offset_t ipc_window0 
    = reinterpret_cast<vm_offset_t>(&_ipc_window0_1);
  /// Start of second IPC window.
  static const vm_offset_t ipc_window1 
    = reinterpret_cast<vm_offset_t>(&_ipc_window1_1);
  /// Start of small address space region.
  static const vm_offset_t smas_start
    = reinterpret_cast<vm_offset_t>(&_smas_start_1);
  /// End of small address space region.
  static const vm_offset_t smas_end
    = reinterpret_cast<vm_offset_t>(&_smas_end_1);
  //@}
 
  /// page table entry in service page of 4k window for jdb adapter space
  static pt_entry_t *jdb_adapter_pt;

  /** Segment numbers. */
  enum { gdt_tss = GDT_TSS, 
    gdt_code_kernel = GDT_CODE_KERNEL, gdt_data_kernel = GDT_DATA_KERNEL,
    gdt_code_user = GDT_CODE_USER, gdt_data_user = GDT_DATA_USER, 
    gdt_max = GDT_MAX };


  template< typename _Ty >
  static _Ty *phys_to_virt( P_ptr<_Ty> ); // physical to kernel-virtual


protected:
  static pd_entry_t *kdir;	///< Kernel page directory
  static pd_entry_t cpu_global;	///< Page-table flags used for global entries

private:
  friend class kdb;
  friend class profile;

  Kmem();			// default constructors are undefined
  Kmem(const Kmem&);

  static vm_offset_t mem_max, _himem;

  static const pd_entry_t flag_global = 0x200; // l4-specific pg dir entry flag

  static x86_tss volatile *tss asm ("KMEM_TSS");
  static x86_desc *gdt;

  static Unsigned8 *io_bitmap_delimiter;
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
#include "config.h"
#include "cpu.h"
#include "globals.h"
#include "l4_types.h"
#include "pic.h"
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
x86_desc *Kmem::gdt;


Unsigned8 *Kmem::io_bitmap_delimiter;

/** Initialize a new task's page directory from the current master copy
    of kernel page tables (the global page directory).
    @param d pointer to first entry of new page directory */
PUBLIC static
void Kmem::dir_init(pd_entry_t *d)
{
  std::memcpy(d + ((mem_user_max >> PDESHIFT) & PDEMASK),
	 kdir + ((mem_user_max >> PDESHIFT) & PDEMASK),
	 ((~0 - mem_user_max + 1) >> PDESHIFT) * sizeof(pd_entry_t));
}

/** Flush a TLB entry.
    @param addr virtual address of page whose TLB entry should be flushed */
PUBLIC static
inline void Kmem::tlb_flush(vm_offset_t addr) // flush tlb at virtual addr
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

// 
// ACCESSORS
// 

/** Return virtual address of an IPC window.
    @param win number of IPC window (0 or 1)
    @return IPC window's virtual address */
PUBLIC static
inline vm_offset_t 
Kmem::ipc_window(unsigned win)
{
  if (win == 0)
    return ipc_window0;

  return ipc_window1;
}

IMPLEMENT inline
bool
Kmem::iobm_fault_addr (Address addr)
{
  return (addr >= Kmem::io_bitmap && addr <= Kmem::io_bitmap +
                                             L4_fpage::IO_PORT_MAX / 8);
}

IMPLEMENT inline
bool
Kmem::smas_fault_addr (Address addr)
{
  return (addr >= Kmem::smas_start && addr < Kmem::smas_end);
}

IMPLEMENT inline
bool
Kmem::ipcw_fault_addr (Address addr, unsigned)
{
  return (addr >= Kmem::ipc_window(0) && addr < Kmem::ipc_window(1) +
                                         (Config::SUPERPAGE_SIZE << 1));
}

IMPLEMENT inline
bool
Kmem::user_fault_addr (Address addr, unsigned)
{
  return (addr < mem_user_max);
}

IMPLEMENT inline
bool
Kmem::pagein_tcb_request (Address eip)
{
#ifdef CONFIG_ASSEMBLER_IPC_SHORTCUT
  extern Mword pagein_tcb_request1;
  extern Mword pagein_tcb_request2;
#endif
  extern Mword pagein_tcb_request3;

  return (
#ifdef CONFIG_ASSEMBLER_IPC_SHORTCUT
           (eip == (Address)&pagein_tcb_request1) ||
           (eip == (Address)&pagein_tcb_request2) ||
#endif
	   (eip == (Address)&pagein_tcb_request3)
	  );
}

PUBLIC static
inline
vm_offset_t Kmem::io_bitmap_delimiter_page()
{
  return reinterpret_cast<vm_offset_t>(io_bitmap_delimiter);
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
  
  return reinterpret_cast<vm_offset_t>
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

  // make a copy of the multiboot parameters
 

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
  vm_offset_t cpu_page = stupid_alloc(& _himem);

  kdir = static_cast<pd_entry_t *>(phys_to_virt(stupid_alloc(& _himem)));
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
  for (vm_offset_t address = 0; address < mem_max; 
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
	  pt_entry_t *t = 
	    reinterpret_cast<pt_entry_t *>(stupid_alloc(& _himem));

	  kdir[kd_entry] = reinterpret_cast<vm_offset_t>(t)
	    | INTEL_PDE_VALID | INTEL_PDE_WRITE | INTEL_PDE_REF | cpu_global;
	  for (vm_offset_t a = address; a < address + Config::SUPERPAGE_SIZE;
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
  
  pt_entry_t * t;

  t = reinterpret_cast<pt_entry_t *>(stupid_alloc(&_himem));
  kdir[(service_page >> PDESHIFT) & PDEMASK] = reinterpret_cast<vm_offset_t>(t)
    | INTEL_PDE_VALID | INTEL_PDE_WRITE | INTEL_PDE_REF | INTEL_PDE_USER
    | cpu_global;

  //Set small space variables. Serves as default for all address spaces.
  //(default being big address space and version 0)
  //Sorry about the bare numbers.
  //See corresponding code in space_context.
  if (Config::USE_SMALL_SPACES) 
  {
    kdir[(reinterpret_cast<unsigned long>(&_smas_area_1) >> PDESHIFT) & PDEMASK]  
             = ((mem_user_max - 1) >> 12) & 0xFFF00;
    kdir[(reinterpret_cast<unsigned long>(&_smas_version_1) >> PDESHIFT) &        PDEMASK] = 0;
  }

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

  // now switch to our new page table
  set_pdbr(virt_to_phys(kdir));

  // map the cpu_page we allocated earlier just before io_bitmap
  vm_offset_t cpu_page_vm;

  assert((io_bitmap & ~Config::SUPERPAGE_MASK) == 0);

  if ((Cpu::features() & FEAT_PSE)
      && Config::SUPERPAGE_SIZE - (cpu_page & ~Config::SUPERPAGE_MASK) < 0x10000)
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
      pt_entry_t *t = reinterpret_cast<pt_entry_t *>(stupid_alloc(& _himem));
      kdir[((io_bitmap >> PDESHIFT) & PDEMASK) - 1]
	= reinterpret_cast<vm_offset_t>(t)
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
      io_bitmap_delimiter = reinterpret_cast<Unsigned8 *>
	(alloc_from_page(& cpu_page_vm, 1));

      // did we really get the first byte ??
      assert((reinterpret_cast<vm_offset_t>(io_bitmap_delimiter) & ~Config::PAGE_MASK)
	     == 0);
      *io_bitmap_delimiter = 0xff;
    }

  // now initialize the global descriptor table
  gdt = reinterpret_cast<x86_desc *>(alloc_from_page(& cpu_page_vm, gdt_max));

  // allocate the task segment as the last thing from cpu_page_vm
  // because with IO protection enabled the task segment includes the 
  // rest of the page and the following IO bitmat (2 pages).
  tss = reinterpret_cast<x86_tss volatile*>(alloc_from_page(& cpu_page_vm, 
							    sizeof(x86_tss)));

  // XXX Just before the IO bitmap there are 32 bytes interrupt redirection 
  // XXX map for V86 mode.
  // XXX do something about it

  cpu_page_vm = 0;		// disable allocation from cpu_page_vm
  
  // make sure kernel cs/ds and user cs/ds are placed in the same
  // cache line, respectively; pre-set all "accessed" flags so that
  // the CPU doesn't need to do this later
  if(Config::enable_io_protection)
    {
      vm_offset_t tss_size = 
	io_bitmap + (L4_fpage::IO_PORT_MAX / 8) 
		  - reinterpret_cast<vm_offset_t>(tss);
      assert(tss_size < 0x100000); // must fit in 20 Bits
      fill_descriptor(gdt + gdt_tss/8, reinterpret_cast<vm_offset_t>(tss), 
		      // this is actually tss_size +1, including the
		      // io_bitmap_delimiter byte 
		      tss_size, 
		      ACC_PL_K | ACC_TSS, 0);
    }
  else
    {
      fill_descriptor(gdt + gdt_tss/8, reinterpret_cast<vm_offset_t>(tss), 
		      sizeof(x86_tss) - 1, 
		      ACC_PL_K | ACC_TSS, 0);
    }
  fill_descriptor(gdt + gdt_code_kernel/8, 0, 0xffffffff,
		  ACC_PL_K | ACC_CODE_R | ACC_A, SZ_32);
  fill_descriptor(gdt + gdt_data_kernel/8, 0, 0xffffffff,
		  ACC_PL_K | ACC_DATA_W | ACC_A, SZ_32);
  if (Config::USE_SMALL_SPACES)
  {
    fill_descriptor(gdt + gdt_code_user/8, 0, 0xbfffffff,
                  ACC_PL_U | ACC_CODE_R | ACC_A, SZ_32);
    fill_descriptor(gdt + gdt_data_user/8, 0, 0xbfffffff,
                  ACC_PL_U | ACC_DATA_W | ACC_A, SZ_32);
  } else
  {
    fill_descriptor(gdt + gdt_code_user/8, 0, 0xffffffff,
                  ACC_PL_U | ACC_CODE_R | ACC_A, SZ_32);
    fill_descriptor(gdt + gdt_data_user/8, 0, 0xffffffff,
                  ACC_PL_U | ACC_DATA_W | ACC_A, SZ_32);
  }

  desc.limit = gdt_max - 1;
  desc.linear_base = reinterpret_cast<vm_offset_t>(gdt);
  set_gdt(&desc);
  set_ldt(0);

  if (Config::USE_SMALL_SPACES)
  {
    set_ds(gdt_data_kernel);
    set_es(gdt_data_kernel);
  } else
  {
    set_ds(gdt_data_user | SEL_PL_U);
    set_es(gdt_data_user | SEL_PL_U);
  }

  set_fs(gdt_data_user | SEL_PL_U);
  set_gs(gdt_data_user | SEL_PL_U);
  set_ss(gdt_data_kernel);
  set_cs(gdt_code_kernel);

  // and finally initialize the TSS
  tss->ss0 = gdt_data_kernel;
  tss->io_bit_map_offset = io_bitmap - reinterpret_cast<vm_offset_t>(tss);
  set_tr(gdt_tss);

  Cpu::init_sysenter(reinterpret_cast<Address>(kernel_esp()));

  // CPU initialization done

  // allocate the kernel info page
  extern char __crt_dummy__, _end; // defined by linker and in crt0.S

  Kernel_info *kinfo 
    = static_cast<Kernel_info*>(phys_to_virt(stupid_alloc(& _himem)));
  Kernel_info::init_kip(kinfo);

  // initialize kernel info page from prototype
  char *sub = strstr(Boot_info::cmdline(), " proto=");
  if (sub)
    {
      vm_offset_t proto;
      proto = strtoul(sub + 7, 0, 0);
      if (proto)
	{
	  std::memcpy(kinfo, phys_to_virt(proto), Config::PAGE_SIZE);
	}
    }

  kinfo->magic = L4_KERNEL_INFO_MAGIC;
  kinfo->version = Config::kernel_version_id;
  kinfo->main_memory.low = 0;
  kinfo->main_memory.high = mem_max;
  kinfo->reserved0.low = virt_to_phys(&__crt_dummy__) & Config::PAGE_MASK;
  kinfo->reserved0.high = (virt_to_phys(&_end) + Config::PAGE_SIZE -1) 
    & Config::PAGE_MASK;
  kinfo->semi_reserved.low = 1024 * Boot_info::mbi_virt()->mem_lower;
  kinfo->semi_reserved.high = 1024 * 1024;

  kinfo->offset_version_strings = ((Mword)(((Kernel_info*)0x10)->version_strings) - 0x10) >> 4;
  std::strcpy(reinterpret_cast<char*>(kinfo) + 
	      (kinfo->offset_version_strings << 4),
	      Config::kernel_version_string);
  
  kinfo->clock = 0;

  // now define space for the kernel memory allocator

  void *kmem_base = reinterpret_cast<void*>
    ((Mword)(phys_to_virt(mem_max - mem_max * Config::kernel_mem_per_cent / 100))
     & Config::PAGE_MASK);
  
  kinfo->reserved1.low = virt_to_phys(kmem_base);
  kinfo->reserved1.high = mem_max;
}
