INTERFACE [ia32]:

EXTENSION class Kmem
{
protected:
  static Pdir *kdir asm ("KMEM_KDIR"); ///< Kernel page directory

private:
  static Address user_max();
  static Unsigned8 *io_bitmap_delimiter;
};

INTERFACE [ia32-smas]:

EXTENSION class Kmem
{
private:
  /** Current size of user gdt entry. */
  static Unsigned32 volatile current_user_gdt asm ("KMEM_CURRENT_GDT");    
};

//---------------------------------------------------------------------------
IMPLEMENTATION [ia32]:

#include "cmdline.h"
#include "cpu.h"
#include "l4_types.h"
#include "mem_unit.h"
#include "panic.h"
#include "paging.h"
#include "pic.h"
#include "std_macros.h"

// static class variables
Unsigned8    *Kmem::io_bitmap_delimiter;

/** Initialize a new task's page directory from the current master copy
    of kernel page tables (the global page directory).
    @param d pointer to first entry of new page directory */
PUBLIC static
void
Kmem::dir_init(Pdir *d)
{
  Cpu::memcpy_mwords (d->lookup(mem_user_max), kdir->lookup(mem_user_max),
	              (((~0 - mem_user_max + 1) >> Pd_entry::Shift) 
		        * sizeof(Pd_entry)) / sizeof(Mword));
}

/** Return virtual address of an IPC window.
    @param win number of IPC window (0 or 1)
    @return IPC window's virtual address */
PUBLIC static inline NEEDS["mem_layout.h"]
Address 
Kmem::ipc_window(unsigned win)
{
  return win == 0 
    ? Mem_layout::Ipc_window0
    : Mem_layout::Ipc_window1;
}

IMPLEMENT inline NEEDS["mem_layout.h"]
Mword
Kmem::is_io_bitmap_page_fault(Address addr)
{
  return (addr >= Mem_layout::Io_bitmap && 
	  addr <= Mem_layout::Io_bitmap + L4_fpage::Io_port_max / 8);
}

IMPLEMENT inline NEEDS["mem_layout.h"]
Mword
Kmem::is_smas_page_fault(Address addr)
{
  return (addr >= Mem_layout::Smas_start && addr < Mem_layout::Smas_end);
}

IMPLEMENT inline NEEDS["mem_layout.h"]
Mword
Kmem::is_ipc_page_fault(Address addr, Mword /*error*/)
{
  return (addr >= ipc_window(0) && addr < ipc_window(1) +
	  (Config::SUPERPAGE_SIZE << 1));
}

IMPLEMENT inline NEEDS["mem_layout.h"]
Mword
Kmem::is_kmem_page_fault(Address addr, Mword /*error*/)
{
  return (addr >= mem_user_max);
}

PUBLIC static inline
Address
Kmem::io_bitmap_delimiter_page()
{
  return reinterpret_cast<Address>(io_bitmap_delimiter);
}

IMPLEMENT
Address
Kmem::kmem_base()
{
  Address base = Boot_info::kmem_start (virt_to_phys ((void*)0xffffffff));
  if ((Address)phys_to_virt(base) < Mem_layout::Physmem)
    panic("Too much kernel memory reserved");

  return base;
}

/**
 * Compute physical address from a kernel-virtual address.
 * @param addr a virtual address
 * @return corresponding physical address if a mappings exists.
 *         -1 otherwise.
 */
IMPLEMENT inline NEEDS["paging.h","std_macros.h","mem_layout.h"]
Address
Kmem::virt_to_phys (const void *addr)
{
  Address a = reinterpret_cast<Address>(addr);

  if (EXPECT_TRUE (Mem_layout::in_pmem(a)))
    return Mem_layout::pmem_to_phys(a);

  if (EXPECT_TRUE (Mem_layout::in_boot_state(a)))
    return a - Mem_layout::Boot_state_start;

  return kdir->virt_to_phys(a);
}

//
// helper functions
//

// set a 4k-mapping, not-cached/write-through
PUBLIC static
void
Kmem::map_devpage_4k(Address phys, Address virt,
    		     bool cached, bool global, Address *offs=0)
{
  Pt_entry *e  = kdir->lookup(virt)->ptab()->lookup(virt);
  Unsigned32 pte = phys & Config::PAGE_MASK;

  *e = pte | Pt_entry::Valid | Pt_entry::Writable 
	   | Pt_entry::Referenced | Pt_entry::Dirty 
	   | (cached ? 0 : (Pt_entry::Write_through | Pt_entry::Noncacheable))
	   | (global ? Pt_entry::global() : 0);
  Mem_unit::tlb_flush(virt);

  if (offs)
    *offs = phys - pte;
}

IMPLEMENT inline Address Kmem::kcode_start()
{ return virt_to_phys (&Mem_layout::start) & Config::PAGE_MASK; }

IMPLEMENT inline Address Kmem::kcode_end()
{
  return (virt_to_phys (&Mem_layout::end) + Config::PAGE_SIZE)
    & Config::PAGE_MASK;
}

IMPLEMENT FIASCO_INIT
void
Kmem::init()
{
  Address kphys_start, kphys_end;
  const Address kphys_size = 0 - Mem_layout::Physmem;
  // There are several things to note here: first, we run before main()
  // has been started, i.e., before any constructors for static objects
  // have been run.  Second, we can assume here that we still have parts
  // of the physical memory mapped in one-to-one from ia32/boot/boot_cpu.cc:
  // the first 4MB and the least 64MB. Third, we can't allocate memory
  // dynamically before we have set up the kernel memory and init page table.
  // We also can assume that Cpu has already been initialized.

  // find the highest memory address
  mem_max  = (Boot_info::mbi_virt()->mem_upper + 1024) << 10;
  mem_max &= Config::PAGE_MASK;

  // limit to 1GB
  if (mem_max > 1<<30)
    mem_max = 1<<30;

  // startup for himem_alloc
  _himem = mem_max;

  // determine the addresses of physical memory we map as kernel memory
  kphys_end   = (mem_max + ~Config::SUPERPAGE_MASK) & Config::SUPERPAGE_MASK;
  kphys_start = kphys_end > kphys_size ? kphys_end - kphys_size : 0;

  // needed for faster translation between kernel memory and physical memory
  Mem_layout::kphys_base(kphys_start);

  Pdir::have_superpages(Cpu::have_superpages());

  // allocate a page for processor data structures.  this page is
  // filled in later; however, because we start allocating from the
  // end of memory, we allocate this page early to increase the chance
  // that it is on the end of a 4MB page.  if this happens, we can map
  // it in as an 4MB page later; otherwise, we need to allocate a page
  // table just for this page.  the reason we need this page near the
  // end is that it must refer to the io_bitmap on the next 4MB-page
  // with a 16-bit pointer.
  Address cpu_page = himem_alloc();

  kdir = static_cast<Pdir*>(phys_to_virt(himem_alloc()));

  if (Cpu::features() & FEAT_PGE)
    {
      Pd_entry::enable_global();
      Cpu::set_cr4 (Cpu::get_cr4() | CR4_PGE);
    }

  // set up the kernel mapping for physical memory.  mark all pages as
  // referenced and modified (so when touching the respective pages
  // later, we save the CPU overhead of marking the pd/pt entries like
  // this)

  // we also set up a one-to-one virt-to-phys mapping for two reasons:
  // (1) so that we switch to the new page table early and re-use the
  //     segment descriptors set up by boot_cpu.cc.  (we'll set up our
  //     own descriptors later.) we only need the first 4MB for that.
  // (2) a one-to-one phys-to-virt mapping in the kernel's page directory
  //     sometimes comes in handy (mostly useful for debugging)
  Ptab    *pt;
  Address pt_phys;

  // first 4MB page
  kdir->map_superpage(0, Mem_layout::Boot_state_start, himem_alloc,
		      Pd_entry::Valid | Pd_entry::Writable |
	      	      Pd_entry::Referenced | Pd_entry::global());

  // map the last 64MB of physical memory as kernel memory
  for (Address phys = kphys_start, virt = Mem_layout::Physmem; 
       phys < kphys_end;
       phys += Config::SUPERPAGE_SIZE, virt += Config::SUPERPAGE_SIZE)
    kdir->map_superpage(phys, virt, himem_alloc,
			Pd_entry::Valid | Pd_entry::Writable |
		      	Pd_entry::Referenced | Pd_entry::global());

  // map the whole physical memory one-to-one
  for (Address phys = 0, virt = 0; phys < mem_max;
       phys += Config::SUPERPAGE_SIZE, virt += Config::SUPERPAGE_SIZE)
    kdir->map_superpage(phys, virt, himem_alloc,
			Pd_entry::Valid | Pd_entry::Writable |
		      	Pd_entry::Referenced | Pd_entry::global());

  // The service page directory entry points to an universal usable
  // page table which is currently used for the Local APIC and the 
  // jdb adapter page.
  assert((Mem_layout::Service_page & ~Config::SUPERPAGE_MASK) == 0);

  pt_phys = himem_alloc();
  *(kdir->lookup(Mem_layout::Service_page)) = pt_phys 
					| Pd_entry::Valid | Pd_entry::Writable
					| Pd_entry::Referenced | Pd_entry::User
					| Pd_entry::global();

  // Set small space variables. Serves as default for all address spaces.
  // (default being big address space and version 0)
  // Sorry about the bare numbers. See corresponding code in space_context.
  if (Config::Small_spaces)
    {
      *(kdir->lookup(Mem_layout::Smas_area))    = 
	    ((mem_user_max-1) >> 12) & 0xFFF00;
      *(kdir->lookup(Mem_layout::Smas_version)) = 0;
    }

  // kernel mode should acknowledge write-protected page table entries
  Cpu::set_cr0(Cpu::get_cr0() | CR0_WP);

  // now switch to our new page table
  Cpu::set_pdbr(virt_to_phys(kdir));

  // map the cpu_page we allocated earlier just before io_bitmap
  Address cpu_page_vm;

  assert((Mem_layout::Io_bitmap & ~Config::SUPERPAGE_MASK) == 0);

  if (Cpu::have_superpages()
      && Config::SUPERPAGE_SIZE - (cpu_page&~Config::SUPERPAGE_MASK) < 0x10000)
    {
      // can map as 4MB page because the cpu_page will land within a
      // 16-bit range from io_bitmap
      *(kdir->lookup(Mem_layout::Io_bitmap - Config::SUPERPAGE_SIZE))
	= (cpu_page & Config::SUPERPAGE_MASK)
	| Pd_entry::Superpage | Pd_entry::Valid 
	| Pd_entry::Writable | Pd_entry::Referenced 
	| Pd_entry::Dirty | Pd_entry::global();

      cpu_page_vm = (cpu_page & ~Config::SUPERPAGE_MASK)
		  + (Mem_layout::Io_bitmap - Config::SUPERPAGE_SIZE);
    }
  else
    {
      pt_phys = himem_alloc();
      pt      = reinterpret_cast<Ptab*>(phys_to_virt(pt_phys));

      *(kdir->lookup(Mem_layout::Io_bitmap - Config::SUPERPAGE_SIZE))
	= pt_phys  | Pd_entry::Valid | Pd_entry::Writable
		   | Pd_entry::Referenced | Pd_entry::global();

      *(pt->lookup(Config::SUPERPAGE_SIZE - Config::PAGE_SIZE))
	= cpu_page | Pt_entry::Valid | Pt_entry::Writable
		   | Pt_entry::Referenced | Pt_entry::Dirty
		   | Pt_entry::global();

      cpu_page_vm = Mem_layout::Io_bitmap - Config::PAGE_SIZE;
    }

  if (Config::enable_io_protection)
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
  Cpu::init_gdt (alloc_from_page(&cpu_page_vm, Gdt::gdt_max), user_max());

  // allocate the task segment for the double fault handler
  Cpu::init_tss_dbf (alloc_from_page(& cpu_page_vm, sizeof(Tss)), 
		     virt_to_phys(kdir));

  // Allocate the task segment as the last thing from cpu_page_vm
  // because with IO protection enabled the task segment includes the 
  // rest of the page and the following IO bitmat (2 pages).
  //
  // Allocate additional 256 bytes for emergency stack right beneath
  // the tss. It is needed if we get an NMI or debug exception at
  // entry_sysenter/entry_sysenter_c/entry_sysenter_log.
  Address tss_mem  = alloc_from_page (&cpu_page_vm, sizeof(Tss) + 256);
  size_t  tss_size;
  
  if (Config::enable_io_protection)
    // this is actually tss_size +1, including the io_bitmap_delimiter byte
    tss_size = Mem_layout::Io_bitmap + (L4_fpage::Io_port_max / 8) - tss_mem;
  else
    tss_size = sizeof(Tss) - 1;

  assert(tss_size < 0x100000); // must fit into 20 Bits

  Cpu::init_tss (tss_mem, tss_size);

  // disable allocation from cpu_page_vm
  cpu_page_vm = 0;

  // set up the x86 CPU's memory model
  Cpu::set_gdt();
  Cpu::set_ldt(0);

  Cpu::set_ds (Gdt::data_segment(true));
  Cpu::set_es (Gdt::data_segment(true));
  Cpu::set_ss (Gdt::gdt_data_kernel | Gdt::Selector_kernel);
  Cpu::set_fs (Gdt::gdt_data_user   | Gdt::Selector_user);
  Cpu::set_gs (Gdt::gdt_data_user   | Gdt::Selector_user);
  Cpu::set_cs ();

  // and finally initialize the TSS
  Cpu::set_tss();

  Cpu::init_sysenter(reinterpret_cast<Address>(kernel_esp()));

  // CPU initialization done

  // allocate the kernel info page
  Kip *kinfo = static_cast<Kip*>(phys_to_virt (himem_alloc()));

  // initialize global pointer to the KIP
  Kip::init_global_kip (kinfo);

  // initialize kernel info page from prototype
  char *sub = strstr (Cmdline::cmdline(), " proto=");
  if (sub)
    {
      Address proto = strtoul(sub + 7, 0, 0);
      if (proto)
	Cpu::memcpy_mwords (kinfo, Mem_layout::boot_data((void*)proto),
			    Config::PAGE_SIZE / sizeof(Mword));
    }
}

//---------------------------------------------------------------------------
IMPLEMENTATION [ia32-!smas]:

IMPLEMENT inline Address Kmem::user_max() { return (Address) -1; }

//---------------------------------------------------------------------------
IMPLEMENTATION [ia32-smas]:

#include "atomic.h"

Unsigned32 volatile Kmem::current_user_gdt;

PUBLIC static inline
Unsigned32
Kmem::get_current_user_gdt()
{
  return current_user_gdt;
}

/** Set user data and code segment.
 *  Segments are set only if the content would really change.
 *  This is all bitswitching to be effective, will be explained soon. 
 *
 *  Encoding of Pdir entry:
 *  +------28------24------20------16------12------08------04---+--00
 *  |   base31:24   |       |     limit31:20        | base23:18 |   |
 *  +---------------+-------+-------+-------+-------+-------+---+---+
 *  
 *  @return true, if GDT has been changed.
 */
PUBLIC static inline NEEDS["cpu.h","gdt.h","std_macros.h"]
void
Kmem::set_gdt_user (Unsigned32 gdtinfo)
{
  current_user_gdt = gdtinfo;

  Cpu::get_gdt()->set_raw(Gdt::gdt_code_user/8,
			  (gdtinfo & 0x0000FC00) | 0x000003FF,
		    	  (gdtinfo & 0xFF0F00FC) | 0x00C0FB00);
  Cpu::get_gdt()->set_raw(Gdt::gdt_data_user/8,
			  (gdtinfo & 0x0000FC00) | 0x000003FF,
		     	  (gdtinfo & 0xFF0F00FC) | 0x00C0F300);

  barrier();

  Cpu::set_fs(Gdt::gdt_data_user | Gdt::Selector_user);
}

/** Updates the small address space window and increments
 *  the version counter.
 *  @return Version of the update.
 */
PUBLIC static inline NEEDS["atomic.h","paging.h","mem_layout.h"]
void
Kmem::update_smas_window (Address virt, Unsigned32 entry, bool flush)
{
  *(kdir->lookup(virt)) = entry;

  // Increment version of small space window in master copy.
  if (flush) 
    atomic_add ((Mword*) kdir->lookup (Mem_layout::Smas_version), 2U);
}

PUBLIC static inline
void
Kmem::smas_pdir_version (Unsigned32 version)
{
  *(kdir->lookup (Mem_layout::Smas_version)) = version << 1;
}

PUBLIC static inline
Unsigned32
Kmem::smas_pdir_version()
{
  return kdir->entry (Mem_layout::Smas_version).raw() >> 1;
}

IMPLEMENT inline Address Kmem::user_max() { return Kmem::mem_user_max - 1; }

