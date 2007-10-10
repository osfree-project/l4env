INTERFACE [ia32]:

EXTENSION class Mem_space
{
private:
  void switch_ldt();

public:
  typedef Pdir Dir_type;

protected:
  // DATA
  Dir_type *_dir;
};

INTERFACE [ia32-smas]:

#include "config.h"             // for MAGIC_CONST
#include "mem_layout.h"


IMPLEMENTATION[ia32]:

#include <cassert>
#include "l4_types.h"
#include "kmem.h"
#include "mem_unit.h"
#include "cpu_lock.h"
#include "lock_guard.h"
#include "paging.h"

#include <cstring>
#include "config.h"
#include "kmem.h"

PRIVATE static inline NEEDS ["cpu.h", "kmem.h"]
Pdir *
Mem_space::current_pdir()
{
  return reinterpret_cast<Pdir *>(Kmem::phys_to_virt(Cpu::get_pdbr()));
}

IMPLEMENT inline NEEDS ["cpu.h", "kmem.h"]
void
Mem_space::make_current()
{
  Cpu::set_pdbr((Mem_layout::pmem_to_phys(_dir)));
  _current = this;
}

//---------------------------------------------------------------------------
IMPLEMENTATION [ia32-!smas]:

#include "cpu.h"
#include "kmem.h"
#include "logdefs.h"


IMPLEMENT inline NEEDS["kmem.h","logdefs.h",Mem_space::current_pdir]
void
Mem_space::switchin_context()
{
  // never switch to kernel space (context of the idle thread)
  if (dir() == Kmem::dir())
    return;

  bool need_flush_tlb = false;
  unsigned index      = Pdir::virt_to_idx(Kmem::ipc_window(0));

  if ((*_dir)[index] || (*_dir)[index + 1])
    {
      (*_dir)[index]     = 0;
      (*_dir)[index + 1] = 0;
      need_flush_tlb  = true;
    }

  index = Pdir::virt_to_idx(Kmem::ipc_window(1));

  if ((*_dir)[index] || (*_dir)[index + 1])
    {
      (*_dir)[index]     = 0;
      (*_dir)[index + 1] = 0;
      need_flush_tlb  = true;
    }

  if (need_flush_tlb || dir() != current_pdir())
    {
      CNT_ADDR_SPACE_SWITCH;
      make_current();
      switch_ldt();
    }
}

//---------------------------------------------------------------------------
IMPLEMENTATION [ia32-smas]:

#include "atomic.h"
#include "kmem.h"
#include "logdefs.h"
#include "mem_unit.h"
#include "std_macros.h"

PUBLIC inline
Address
Mem_space::ldt_addr()
{
  return 0;
}


PUBLIC inline
Unsigned32
Mem_space::small_space_area() const
{
  return _dir->entry (Mem_layout::Smas_area).raw();
}

/** Returns assigned data segment base.
 */
PUBLIC inline
unsigned
Mem_space::small_space_base() const
{
  return ( small_space_area()        & 0xFF000000) |
         ((small_space_area() << 16) & 0x00FF0000);
}

/** Returns assigned data segment size.
 */
PUBLIC inline
unsigned
Mem_space::small_space_size() const
{
  return ((small_space_area() & 0x000FFC00) + 0x400) << 12;
}

/** Assigns data segment dimension.
 *  Doesn't care about correct values. Both values are rounded
 *  down to the next MB.
 *  Default is set in kdir::init.
 *
 *  Encoding of Pdir entry:
 *  +------28------24------20------16------12------08------04---+--00
 *  |   base31:24   |       |     limit31:20        | base23:18 |   |
 *  +---------------+-------+-------+-------+-------+-----------+---+
 */
PUBLIC inline
void
Mem_space::set_small_space(unsigned base, unsigned size)
{
  *(_dir->lookup(Mem_layout::Smas_area)) =  (base     & 0xFF000000)
					| ((base     & 0x00FC0000) >> 16)
					| (((size-1) & 0xFFC00000) >> 12);
}

/** Checks if assigned data segment seems to be in a small space.
 */
PUBLIC inline
bool
Mem_space::is_small_space() const
{
  // base set?
  return (small_space_area() & 0xFF0000FC) != 0;
}

PUBLIC inline
void
Mem_space::smas_pdir_version(Unsigned32 version)
{
  *(_dir->lookup (Mem_layout::Smas_version)) = version << 1;
}

PUBLIC inline
Unsigned32
Mem_space::smas_pdir_version() const
{
  return _dir->entry (Mem_layout::Smas_version).raw() >> 1;
}

/** Update the small space window from the kmem directory.
 *  Always update the full window as we have only one version
 *  counter.
 */
PUBLIC inline NEEDS["kmem.h","std_macros.h"]
bool 
Mem_space::update_smas()
{
  // XXX Selective update disabled as there is still a versioning
  // problem.

  barrier();

  if (Kmem::smas_pdir_version() == smas_pdir_version())
    return false;

  unsigned i;

  for (i = Pdir::virt_to_idx(Mem_layout::Smas_start);
       i < Pdir::virt_to_idx(Mem_layout::Smas_end);
       i++)
    (*_dir)[i] = (*Kmem::dir())[i];

  smas_pdir_version (Kmem::smas_pdir_version());
  return true;
}

PUBLIC inline
void
Mem_space::set_io_bitmap(Unsigned32 v)
{
  *_dir->lookup(Mem_layout::Io_bitmap) = v;
}

PUBLIC inline
Unsigned32
Mem_space::is_set_io_bitmap() const
{
  return _dir->entry(Mem_layout::Io_bitmap).valid();
}

PUBLIC inline
Unsigned32
Mem_space::get_io_bitmap() const
{
  return _dir->entry(Mem_layout::Io_bitmap).raw();
}

PUBLIC inline
void
Mem_space::set_io_bitmap_backup(Unsigned32 v)
{
  *_dir->lookup(Mem_layout::Smas_io_bmap_bak) = v;
}

PUBLIC inline
Unsigned32
Mem_space::get_io_bitmap_backup() const
{
  return _dir->entry(Mem_layout::Smas_io_bmap_bak).raw();
}

/** Switch in new context.
 *  If we switch to a new small space then we have to set the IO bitmap of the
 *  _current_ big space to the backup copy of the new small space. If we switch
 *  to a new big space then we have to set its IO bitmap to the backup copy of
 *  its IO bitmap.
 */
IMPLEMENT inline NEEDS["kmem.h","logdefs.h","mem_unit.h"]
void
Mem_space::switchin_context()
{
  // never switch to kernel space (context of idle thread)
  if (this == (Mem_space*)Kmem::dir())
    return;

  Unsigned8 need_flush_whole_tlb    = 0;
  Unsigned8 need_flush_iobitmap_tlb = 0;
  Unsigned8 change_gdt              = (small_space_area() !=
				       Kmem::get_current_user_gdt());
  Mem_space *big_sc_old         = current_mem_space();
  Mem_space *big_sc_new         = is_small_space() ? big_sc_old : this;
  const unsigned index              = Pdir::virt_to_idx(Kmem::ipc_window(0));

  // Check if IPC window of the new big space is dirty. That would mean that
  // a thread belonging to the new big space is performing long IPC. This
  // could induce a full TLB flush even if we don't switch address spaces!
  if ((*big_sc_new->_dir)[index    ] | (*big_sc_new->_dir)[index + 1] | 
      (*big_sc_new->_dir)[index + 2] | (*big_sc_new->_dir)[index + 3])
    {
      (*big_sc_new->_dir)[index    ] = 0;
      (*big_sc_new->_dir)[index +1 ] = 0;
      (*big_sc_new->_dir)[index +2 ] = 0;
      (*big_sc_new->_dir)[index +3 ] = 0;
      need_flush_whole_tlb = 1;
    }

  if (is_small_space())
    {
      // Big->Small || Small->Small

      if (change_gdt)
	{
	  // different small space
	  if (Config::enable_io_protection)
	    {
	      // IO bitmap TLB flush only necessary if IO bitmap was set in
	      // previous big space
	      if (big_sc_old->is_set_io_bitmap())
		need_flush_iobitmap_tlb = 1;

	      big_sc_new->set_io_bitmap(get_io_bitmap_backup());
	    }
	  if (big_sc_new->update_smas())
	    need_flush_whole_tlb = 1;
	}
      // else same small space

      if (need_flush_whole_tlb)
	Mem_unit::tlb_flush();

      else if (Config::enable_io_protection && need_flush_iobitmap_tlb)
	{
	  CNT_IOBMAP_TLB_FLUSH;
	  Mem_unit::tlb_flush(Mem_layout::Io_bitmap);
	  Mem_unit::tlb_flush(Mem_layout::Io_bitmap + Config::PAGE_SIZE);
	}
    }
  else
    {
      // Big->Big || Small->Big

      if (Config::enable_io_protection && (this != big_sc_old || change_gdt))
	set_io_bitmap(get_io_bitmap_backup());

      // IO bitmap TLB flush necessary if we switch from a small space to a
      // big space without changing the big space.
      if (Config::enable_io_protection && 
	  change_gdt && this == big_sc_old && is_set_io_bitmap())
	need_flush_iobitmap_tlb = 1;

      if (need_flush_whole_tlb || this != big_sc_old)
	{
	  CNT_ADDR_SPACE_SWITCH;
	  make_current();
	}
      else if (Config::enable_io_protection && need_flush_iobitmap_tlb)
	{
	  CNT_IOBMAP_TLB_FLUSH;
      	  Mem_unit::tlb_flush(Mem_layout::Io_bitmap);
	  Mem_unit::tlb_flush(Mem_layout::Io_bitmap + Config::PAGE_SIZE);
	}
    }

  // set new GDT entries for user segment selectors if changed
  if (small_space_area() != Kmem::get_current_user_gdt())
    Kmem::set_gdt_user (small_space_area());
}

//------------------------------------------------------------------------
IMPLEMENTATION [ia32-segments-!smas]:

IMPLEMENT inline NEEDS["cpu.h"]
void
Mem_space::switch_ldt()
{
  Cpu::enable_ldt (ldt_addr(), ldt_size());
}

//------------------------------------------------------------------------
IMPLEMENTATION [ia32-!segments-!smas]:

IMPLEMENT inline
void
Mem_space::switch_ldt()
{
}

IMPLEMENTATION [ia32]:

/*
 * The following functions are all no-ops on native ia32.
 * Pages appear in an address space when the corresponding PTE is made
 * ... unlike Fiasco-UX which needs these special tricks
 */

IMPLEMENT inline
void
Mem_space::page_map (Address, Address, Address, unsigned)
{}

IMPLEMENT inline
void
Mem_space::page_protect (Address, Address, unsigned)
{}

IMPLEMENT inline
void
Mem_space::page_unmap (Address, Address)
{}

IMPLEMENT inline NEEDS ["kmem.h"]
void Mem_space::kmem_update (void *addr)
{
  unsigned i = Pdir::virt_to_idx((Address)addr);

  (*_dir)[i] = (*Kmem::dir())[i];
}

/**
 * Copy multiple PDEs between two address spaces.
 * Be aware that some PDEs are reserved and contain thread-local information
 * like space_index, chief number, IO bitmap which must not be overwritten.
 * Callers are expected to know which slots are safe to copy.
 */
IMPLEMENT inline NEEDS [<cassert>, "mem_unit.h", "paging.h"]
void
Mem_space::remote_update (Address loc_addr, Mem_space const *rem,
			  Address rem_addr, size_t n)
{
  bool tlb_flush    = false;
  unsigned loc_slot = Pdir::virt_to_idx(loc_addr),
           rem_slot = Pdir::virt_to_idx(rem_addr);

  // PDE boundary check
  assert (loc_slot + n <= 1024);
  assert (rem_slot + n <= 1024);
  
  while (n--)
    {
      if ((*_dir)[loc_slot])
        tlb_flush = true;
      
      // XXX: If we copy a PDE and it maps either a superpage with PDE_GLOBAL
      // or a pagetable with one or multiple PTE_GLOBAL entries, these won't
      // be flushed on a TLB flush. Suggested solution: When mapping a page
      // with global bit, a PDE_NO_LONGIPC bit should be set in the
      // corresponding PDE and this bit should be checked here before copying.

      // This loop seems unnecessary, but remote_update is also used for
      // updating the long IPC window.
      // Now consider following scenario with super pages:
      // Sender A makes long IPC to receiver B.
      // A setups the IPC window by reading the pagedir slot from B in an 
      // temporary register. Now the sender is preempted by C. Then C unmaps 
      // the corresponding super page from B. C switch to A back, using 
      // switch_to, which clears the IPC window pde slots from A. BUT then A 
      // write the  content of the temporary register, which contain the now 
      // invalid pde slot, in his own page directory and starts the long IPC.
      // Because no pagefault will happen, A will write to now invalid memory.
      // So we compare after storing the pde slot, if the copy is still
      // valid. And this solution is much faster than grabbing the cpu lock,
      // when updating the ipc window.
      for(;;) {

	const volatile Pd_entry *val = rem->dir()->index(rem_slot);
	(*_dir)[loc_slot] =  val->raw();
	
	if(EXPECT_TRUE((*_dir)[loc_slot] == val->raw()))
	  break;
      }
      
      loc_slot++;
      rem_slot++;

    }

  if (tlb_flush)
    Mem_unit::tlb_flush();
}

// --------------------------------------------------------------------
IMPLEMENTATION[ia32-segments]:

PRIVATE inline
void
Mem_space::free_ldt_memory()
{
  if (ldt_addr())
    Mapped_allocator::allocator()->free(Config::PAGE_SHIFT,
				  reinterpret_cast<void*>(ldt_addr()));
}

// --------------------------------------------------------------------
IMPLEMENTATION[!smas]:

IMPLEMENT inline
void
Mem_space::update_small(Address, bool)
{}

// --------------------------------------------------------------------
IMPLEMENTATION[smas]:

#include "std_macros.h"

/** Update a page in the small space window from kmem.
 * @param flush true if TLB-Flush necessary
 */
IMPLEMENT
void
Mem_space::update_small(Address virt, bool flush)
{
  if (is_small_space() && virt < small_space_size())
    {
      // small space => Kmem
      Kmem::update_smas_window(small_space_base() + virt,
			       _dir->entry(virt).raw(), flush);
      barrier();
      // Kmem => current big space
      current_mem_space()->kmem_update((void*)(small_space_base() + virt));
    }
}
