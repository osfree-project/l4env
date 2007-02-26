INTERFACE [ux]:

EXTENSION class Kmem
{
protected:
  static Pdir *kdir;	///< Kernel page directory
};

IMPLEMENTATION [ux]:

#include <cerrno>
#include <unistd.h>
#include <sys/mman.h>

#include "emulation.h"

// 
// ACCESSORS
// 

IMPLEMENT inline Mword Kmem::is_io_bitmap_page_fault(Address)
{ return false; }

IMPLEMENT inline Mword Kmem::is_smas_page_fault(Address)
{ return false; }

IMPLEMENT inline
Mword
Kmem::is_ipc_page_fault(Address addr, Mword error)
{ return addr < mem_user_max && (error & PF_ERR_REMTADDR); }

IMPLEMENT inline NEEDS ["regdefs.h"]
Mword
Kmem::is_kmem_page_fault(Address addr, Mword error)
{ return !(addr < mem_user_max && (error & PF_ERR_USERADDR)); }

/**
 * Compute physical address from a kernel-virtual address.
 * @param addr a virtual address
 * @return corresponding physical address if a mappings exists.
 *         -1 otherwise.
 */
IMPLEMENT inline NEEDS["paging.h","std_macros.h"]
Address
Kmem::virt_to_phys (const void *addr)
{
  Address a = reinterpret_cast<Address>(addr);

  if (EXPECT_TRUE(Mem_layout::in_pmem(a)))
    return Mem_layout::pmem_to_phys(a);

  return kdir->virt_to_phys(a);
}

IMPLEMENT inline Address Kmem::kcode_start()
{ return Mem_layout::Kernel_start_frame; }

IMPLEMENT inline Address Kmem::kcode_end()
{ return Mem_layout::Kernel_end_frame; }

IMPLEMENT Address Kmem::kmem_base()
{ return Boot_info::kmem_start (mem_max); }

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
    Pd_entry::enable_global();

  // set up the kernel mapping for physical memory.  mark all pages as
  // referenced and modified (so when touching the respective pages
  // later, we save the CPU overhead of marking the pd/pt entries like
  // this)

  // we also set up a one-to-one virt-to-phys mapping for two reasons:
  // (1) so that we switch to the new page table early and re-use the
  // segment descriptors set up by bootstrap.c.  (we'll set up our own
  // descriptors later.)  (2) a one-to-one phys-to-virt mapping in the
  // kernel's page directory sometimes comes in handy

  for (Address phys = 0, virt = Mem_layout::Physmem; phys < mem_max; 
       phys += Config::SUPERPAGE_SIZE, virt += Config::SUPERPAGE_SIZE)
    kdir->map_superpage(phys, virt, himem_alloc,
			Pd_entry::Valid | Pd_entry::Writable | 
			Pd_entry::Referenced | Pd_entry::global());

  // now switch to our new page table
  Emulation::set_pdir_addr (virt_to_phys (kdir));

  // map the cpu_page we allocated earlier just before io_bitmap
  Address cpu_page_vm;

  assert((Mem_layout::Io_bitmap & ~Config::SUPERPAGE_MASK) == 0);

  if (Cpu::have_superpages() &&
      Config::SUPERPAGE_SIZE - (cpu_page & ~Config::SUPERPAGE_MASK) < 0x10000)
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
      Address pt_phys = himem_alloc();
      Ptab    *pt     = reinterpret_cast<Ptab *>(phys_to_virt(pt_phys));

      *(kdir->lookup(Mem_layout::Io_bitmap - Config::SUPERPAGE_SIZE))
   	= pt_phys | Pt_entry::Valid | Pt_entry::Writable 
		  | Pt_entry::Referenced | Pt_entry::global();
    
      *(pt->lookup(Config::SUPERPAGE_SIZE - Config::PAGE_SIZE))
	= cpu_page | Pt_entry::Valid | Pt_entry::Writable
		   | Pt_entry::Referenced | Pt_entry::Dirty 
		   | Pt_entry::global();

      cpu_page_vm = Mem_layout::Io_bitmap - Config::PAGE_SIZE;
    }

  if (mmap ((void *) cpu_page_vm, Config::PAGE_SIZE, PROT_READ 
	    | PROT_WRITE, MAP_SHARED | MAP_FIXED, Boot_info::fd(), cpu_page) 
      == MAP_FAILED)
    printf ("CPU page mapping failed: %s\n", strerror (errno));

  Cpu::init_tss (alloc_from_page(&cpu_page_vm, sizeof(Tss)));

  // CPU initialization done

  // allocate the kernel info page
  Kip *kinfo = static_cast<Kip*>(phys_to_virt (himem_alloc()));

  // initialize global pointer to the KIP
  Kip::init_global_kip (kinfo);
}

