INTERFACE[amd64]:

#include <cassert>
#include "types.h"
#include "config.h"

class Mapped_allocator;

class Paging
{
public:
  enum
    {
      Valid         = 0x0000000000000001LL, ///< Valid
      Writable      = 0x0000000000000002LL, ///< Writable
      User          = 0x0000000000000004LL, ///< User accessible
      Write_through = 0x0000000000000008LL, ///< Write thru
      Cacheable     = 0x0000000000000000LL, ///< Cache is enabled
      Noncacheable  = 0x0000000000000010LL, ///< Caching is off
      Referenced    = 0x0000000000000020LL, ///< Page was referenced
      Dirty         = 0x0000000000000040LL, ///< Page was modified
      Global        = 0x0000000000000100LL, ///< pinned in the TLB
    };
};


class Ptab;
class Pdir;
class Pdp;
class Pml4;


class Pml4_entry
{
public:
  enum
    {
      Shift         =  Config::PML4E_SHIFT,
      Mask          = (Config::PML4E_MASK),
    };
  
  enum
    {
      Valid         = 0x0000000000000001LL, ///< Valid
      Writable      = 0x0000000000000002LL, ///< Writable
      User          = 0x0000000000000004LL, ///< User accessible
      Write_through = 0x0000000000000008LL, ///< Write thru
      Cacheable     = 0x0000000000000000LL, ///< Cache is enabled
      Noncacheable  = 0x0000000000000010LL, ///< Caching is off
      Referenced    = 0x0000000000000020LL, ///< Page was referenced
      Dirty         = 0x0000000000000040LL, ///< Page was modified
      Pdpfn         = 0x000ffffffffff000LL, ///< page frame number for pdp
    };

private:
  Unsigned64        _raw;
};

class Pdp_entry
{
public:
  enum
    {
      Shift         =  Config::PDPE_SHIFT,
      Mask          = (Config::PDPE_MASK),
    };
  
  enum
    {
      Valid         = 0x0000000000000001LL, ///< Valid
      Writable      = 0x0000000000000002LL, ///< Writable
      User          = 0x0000000000000004LL, ///< User accessible
      Write_through = 0x0000000000000008LL, ///< Write thru
      Cacheable     = 0x0000000000000000LL, ///< Cache is enabled
      Noncacheable  = 0x0000000000000010LL, ///< Caching is off
      Referenced    = 0x0000000000000020LL, ///< Page was referenced
      Dirty         = 0x0000000000000040LL, ///< Page was modified
      Pdirfn        = 0x000ffffffffff000LL, ///< page frame number for pdir
    };

private:
  Unsigned64        _raw;
};

class Pd_entry
{
public:
  enum
    {
      Shift         =  Config::PDE_SHIFT,
      Mask          = (Config::PDE_MASK),
    };

  enum
    {
      Valid         = 0x0000000000000001LL, ///< Valid
      Writable      = 0x0000000000000002LL, ///< Writable
      User          = 0x0000000000000004LL, ///< User accessible
      Write_through = 0x0000000000000008LL, ///< Write thru
      Cacheable     = 0x0000000000000000LL, ///< Cache is enabled
      Noncacheable  = 0x0000000000000010LL, ///< Caching is off
      Referenced    = 0x0000000000000020LL, ///< Page was referenced
      Dirty         = 0x0000000000000040LL, ///< Page was modified
      Superpage     = 0x0000000000000080LL, ///< Is Superpage
      Cpu_global    = 0x0000000000000100LL, ///< pinned in the TLB
      L4_global     = 0x0000000000000200LL, ///< pinned in the TLB
      Ptabfn        = 0x000ffffffffff000LL, ///< page frame number for pt
      Superfn       = 0x000fffffffe00000LL, ///< super page frame number
    };

private:
  Unsigned64        _raw;
  static Unsigned32 _cpu_global;
};

class Pt_entry
{
public:
  enum
    {
      Shift         =  Config::PTE_SHIFT,
      Mask          = (Config::PTE_MASK),
    };

  enum
    {
      Valid         = 0x0000000000000001LL, ///< Valid
      Writable      = 0x0000000000000002LL, ///< Writable
      User          = 0x0000000000000004LL, ///< User accessible
      Write_through = 0x0000000000000008LL, ///< Write through
      Cacheable     = 0x0000000000000000LL, ///< Cache is enabled
      Noncacheable  = 0x0000000000000010LL, ///< Caching is off
      Referenced    = 0x0000000000000020LL, ///< Page was referenced
      Dirty         = 0x0000000000000040LL, ///< Page was modified
      Pat_index     = 0x0000000000000080LL, ///< Page table attribute index
      Cpu_global    = 0x0000000000000100LL, ///< pinned in the TLB
      L4_global     = 0x0000000000000200LL, ///< pinned in the TLB
      Pfn           = 0x000ffffffffff000LL, ///< page frame number
    };

private:
  Unsigned64 _raw;
};

class Pml4
{
public:
  enum {
      Entries = 512,
  };
protected:
  Pml4_entry _entries[Entries];
};

class Pdp
{
public:
  enum {
      Entries = 512,
  };
protected:
  Pdp_entry _entries[Entries];
};

class Pdir
{
public:
  enum {
      Entries = 512,
  };
protected:
  Pd_entry _entries[Entries];

private:
  static bool _have_superpages;
};

class Ptab
{
public:
  enum {
      Entries = 512,
  };
protected:
  Pt_entry _entries[Entries];
};

namespace Page 
{
  typedef Unsigned32 Attribs;

  enum Attribs_enum 
  {
    KERN_RW       = 0x00000002, ///< User No access
    USER_RO       = 0x00000004, ///< User Read only
    USER_RW       = 0x00000006, ///< User Read/Write
    USER_RX       = 0x00000004, ///< User Read/Execute
    USER_XO       = 0x00000004, ///< User Execute only
    USER_RWX      = 0x00000006, ///< User Read/Write/Execute
    MAX_ATTRIBS   = 0x00000006,
    Cache_mask    = 0x00000018, ///< Cache attrbute mask
    CACHEABLE     = 0x00000000,
    NONCACHEABLE  = 0x00000018,
  };
};


IMPLEMENTATION[amd64]:

#include <cstdio>

#include "cpu.h"
#include "kdb_ke.h"
#include "mapped_alloc.h"
#include "mem_layout.h"
#include "regdefs.h"


PUBLIC static inline
Address
Paging::canonize(Address addr)
{
  if (addr & 0x0000800000000000UL)
      return addr | 0xffff000000000000UL;
  else
      return addr & 0x0000ffffffffffffUL;
}

PUBLIC static inline
Address
Paging::decanonize(Address addr)
{
  return addr & 0x0000ffffffffffffUL;
}
      
IMPLEMENT inline NEEDS["regdefs.h"]
Mword PF::is_translation_error(Mword error)
{
  return !(error & PF_ERR_PRESENT);
}

IMPLEMENT inline NEEDS["regdefs.h"]
Mword PF::is_usermode_error(Mword error)
{
  return (error & PF_ERR_USERMODE);
}


IMPLEMENT inline NEEDS["regdefs.h"]
Mword PF::is_read_error(Mword error)
{
  return !(error & PF_ERR_WRITE);
}

IMPLEMENT inline NEEDS["regdefs.h"]
Mword PF::usermode_error()
{
  return PF_ERR_USERMODE;
}

IMPLEMENT inline NEEDS["regdefs.h"]
Mword PF::write_error()
{
  return PF_ERR_WRITE;
}

IMPLEMENT inline NEEDS["regdefs.h"]
Mword PF::addr_to_msgword0(Address pfa, Mword error)
{
  return    (pfa   & ~(PF_ERR_PRESENT | PF_ERR_WRITE))
          | (error &  (PF_ERR_PRESENT | PF_ERR_WRITE));
}

IMPLEMENT inline NEEDS["regdefs.h"]
Mword PF::pc_to_msgword1(Address pc, Mword error)
{
  return is_usermode_error(error) ? pc : (Mword)-1;
}


//---------------------------------------------------------------------------

PUBLIC inline
Pml4_entry const&
Pml4_entry::operator = (Pml4_entry const &other)
{
  _raw = other.raw();
  return *this;
}

PUBLIC inline
Pml4_entry const&
Pml4_entry::operator = (Unsigned64 raw)
{
  _raw = raw;
  return *this;
}

PUBLIC inline
Address
Pml4_entry::pdpfn() const
{
  return _raw & Pdpfn;
}

PUBLIC inline NEEDS["mem_layout.h"]
Pdp*
Pml4_entry::pdp() const
{
  return reinterpret_cast<Pdp*>(Mem_layout::phys_to_pmem(pdpfn()));
}

PUBLIC inline NEEDS["mem_layout.h", "mapped_alloc.h", <cstdio>]
Pdp*
Pml4_entry::alloc_pdp(Mapped_allocator* ptab_alloc, Unsigned64 attr)
{
  if (!valid()) { *this = ((Unsigned64) Mem_layout::pmem_to_phys(ptab_alloc->alloc(Config::PAGE_SHIFT))) | attr;
      pdp()->clear();
  }

  _raw |= attr;

  return reinterpret_cast<Pdp*>(Mem_layout::phys_to_pmem(pdpfn()));
}

PUBLIC inline
Unsigned64
Pml4_entry::raw() const volatile
{
  return _raw;
}

PUBLIC inline
Unsigned64&
Pml4_entry::raw()
{
  return _raw;
}

PUBLIC inline
void
Pml4_entry::add_attr(Unsigned64 attr)
{
  _raw |= attr;
}

PUBLIC inline
void
Pml4_entry::del_attr(Unsigned64 attr)
{
  _raw &= ~attr;
}

PUBLIC inline
int
Pml4_entry::valid() const
{
  return _raw & Valid;
}

PUBLIC inline
int
Pml4_entry::writable() const
{
  return _raw & Writable;
}

//---------------------------------------------------------------------------

PUBLIC inline
Pdp_entry const&
Pdp_entry::operator = (Pdp_entry const &other)
{
  _raw = other.raw();
  return *this;
}

PUBLIC inline
Pdp_entry const&
Pdp_entry::operator = (Unsigned64 raw)
{
  _raw = raw;
  return *this;
}

PUBLIC inline
Address
Pdp_entry::pdirfn() const
{
  return _raw & Pdirfn;
}

PUBLIC inline NEEDS["mem_layout.h"]
Pdir*
Pdp_entry::pdir() const
{
  return reinterpret_cast<Pdir*>(Mem_layout::phys_to_pmem(pdirfn()));
}

PUBLIC inline NEEDS["mem_layout.h", "mapped_alloc.h", <cstdio>]
Pdir*
Pdp_entry::alloc_pdir(Mapped_allocator* ptab_alloc, Unsigned64 attr)
{
  if (!valid()) { 
      *this = ((Unsigned64) Mem_layout::pmem_to_phys(ptab_alloc->alloc(Config::PAGE_SHIFT))) | attr;
      pdir()->clear();
  }
  _raw |= attr;
  return pdir();
}

PUBLIC inline
Unsigned64
Pdp_entry::raw() const volatile
{
  return _raw;
}

PUBLIC inline
Unsigned64&
Pdp_entry::raw()
{
  return _raw;
}

PUBLIC inline
void
Pdp_entry::add_attr(Unsigned64 attr)
{
  _raw |= attr;
}

PUBLIC inline
void
Pdp_entry::del_attr(Unsigned64 attr)
{
  _raw &= ~attr;
}

PUBLIC inline
int
Pdp_entry::valid() const
{
  return _raw & Valid;
}

PUBLIC inline
int
Pdp_entry::writable() const
{
  return _raw & Writable;
}

//---------------------------------------------------------------------------

Unsigned32 Pd_entry::_cpu_global = Pd_entry::L4_global;

PUBLIC inline
int
Pd_entry::valid() const
{
  return _raw & Valid;
}

PUBLIC inline
int
Pd_entry::user() const
{
  return _raw & User;
}

PUBLIC inline
int
Pd_entry::writable() const
{
  return _raw & Writable;
}

PUBLIC inline
int
Pd_entry::superpage() const
{
  return _raw & Superpage;
}

PUBLIC inline
Address
Pd_entry::ptabfn() const
{
  return _raw & Ptabfn;
}

PUBLIC inline
Address
Pd_entry::superfn() const
{
  return _raw & Superfn;
}

PUBLIC inline NEEDS["mem_layout.h"]
Ptab*
Pd_entry::ptab() const
{
  return reinterpret_cast<Ptab*>(Mem_layout::phys_to_pmem(ptabfn()));
}

PUBLIC inline
Unsigned64
Pd_entry::raw() const volatile
{
  return _raw;
}

PUBLIC inline
Unsigned64&
Pd_entry::raw() 
{
  return _raw;
}

PUBLIC inline
void
Pd_entry::add_attr(Unsigned64 attr)
{
  _raw |= attr;
}

PUBLIC inline
void
Pd_entry::del_attr(Unsigned64 attr)
{
  _raw &= ~attr;
}

PUBLIC inline
Pd_entry const&
Pd_entry::operator = (Pd_entry const& other)
{
  _raw = other.raw();
  return *this;
}

PUBLIC inline
Pd_entry const&
Pd_entry::operator = (Unsigned64 raw)
{
  _raw = raw;
  return *this;
}

PUBLIC static inline
void
Pd_entry::enable_global()
{
  _cpu_global |= Pd_entry::Cpu_global;
}

/**
 * Global entries are entries that are not automatically flushed when the
 * page-table base register is reloaded. They are intended for kernel data
 * that is shared between all tasks.
 * @return global page-table--entry flags
 */    
PUBLIC static inline
Unsigned32
Pd_entry::global()
{
  return _cpu_global;
}


//---------------------------------------------------------------------------

PUBLIC inline
Pt_entry const&
Pt_entry::operator = (Pt_entry const &other)
{
  _raw = other.raw();
  return *this;
}

PUBLIC inline
Pt_entry const&
Pt_entry::operator = (Unsigned64 raw)
{
  _raw = raw;
  return *this;
}

PUBLIC inline
Unsigned64
Pt_entry::raw() const
{
  return _raw;
}

PUBLIC inline
Unsigned64&
Pt_entry::raw()
{
  return _raw;
}

PUBLIC inline
void
Pt_entry::add_attr(Unsigned64 attr)
{
  _raw |= attr;
}

PUBLIC inline
void
Pt_entry::del_attr(Unsigned64 attr)
{
  _raw &= ~attr;
}

PUBLIC inline
int
Pt_entry::valid() const
{
  return _raw & Valid;
}

PUBLIC inline
int
Pt_entry::writable() const
{
  return _raw & Writable;
}

PUBLIC inline
Address
Pt_entry::pfn() const
{
  return _raw & Pfn;
}

PUBLIC static inline
Unsigned32
Pt_entry::global()
{
  return Pd_entry::global();
}

//---------------------------------------------------------------------------

PUBLIC
Address
Pml4::virt_to_phys(Address virt) const
{
  Pml4_entry pml4_entry = entry(virt);
  if (!pml4_entry.valid())
    return (Address) -1;

  Pdp_entry pdp_entry = *(pml4_entry.pdp()->lookup(virt));
  if (!pdp_entry.valid())
    return (Address) -1;
  
  Pd_entry pd_entry = *(pdp_entry.pdir()->lookup(virt));
  if (!pd_entry.valid())
    return (Address) -1;
  if (pd_entry.superpage())
    return pd_entry.superfn() | (virt & ~Config::SUPERPAGE_MASK);

  Pt_entry pt_entry = *(pd_entry.ptab()->lookup(virt));
  if (!pt_entry.valid())
    return (Address) -1;

  return pt_entry.pfn() | (virt & ~Config::PAGE_MASK);
}

PUBLIC static inline
unsigned
Pml4::virt_to_idx(Address virt)
{
  return (virt >> Pml4_entry::Shift) & Pml4_entry::Mask;
}

PUBLIC static inline
Address
Pml4::idx_to_virt(unsigned idx)
{
  return (idx * Config::PML4_SIZE);
}

PUBLIC inline
Pml4_entry*
Pml4::lookup(Address virt) 
{
  return _entries + virt_to_idx(virt);
}


PUBLIC inline
const Pml4_entry*
Pml4::lookup(Address virt) const
{
  return _entries + virt_to_idx(virt);
}
PUBLIC inline
Pml4_entry
Pml4::entry(Address virt) const
{
  return _entries[virt_to_idx(virt)];
}

PUBLIC inline
Pml4_entry*
Pml4::index(unsigned idx)
{
  assert(idx < Entries);
  return _entries + idx;
}

PUBLIC inline
Unsigned64&
Pml4::operator[](unsigned idx)
{
  assert(idx < Entries);
  return _entries[idx].raw();
}

PUBLIC inline
Unsigned64
Pml4::operator[](unsigned idx) const
{
  assert(idx < Entries);
  return _entries[idx].raw();
}

PUBLIC inline NEEDS["cpu.h"]
void
Pml4::clear()
{
  Cpu::memset_mwords(_entries, 0, Config::PAGE_SIZE/sizeof(Mword));
}

// XXX function for early mapping initializations
PUBLIC
void
Pml4::map_range(Address phys_start, Address phys_end, Address virt,
    		Address (*ptab_alloc)(), Unsigned64 attr)
{
  assert(!(phys_start & ~Config::PAGE_MASK));
  assert(!(phys_end & ~Config::PAGE_MASK));
  assert(!(virt & ~ Config::PAGE_MASK));
  
  if (!(phys_start & ~Config::SUPERPAGE_MASK) && 
      !(phys_end & ~Config::SUPERPAGE_MASK))
    {
      // map superpages
      for (Address phys_addr = phys_start; phys_addr < phys_end; 
	   phys_addr += Config::SUPERPAGE_SIZE, virt += Config::SUPERPAGE_SIZE)
	{
	  map_superpage(phys_addr, virt, ptab_alloc, attr);
	}
    }
  else
    {
      // map pages
      for (Address phys_addr = phys_start; phys_addr < phys_end;
	  phys_addr += Config::PAGE_SIZE, virt += Config::PAGE_SIZE)
	{
	  map_page(phys_addr, virt, ptab_alloc, attr);
	}
    }
}


// XXX function for early mapping initializations
PUBLIC
void
Pml4::map_superpage(Address phys, Address virt, 
		    Address (*ptab_alloc)(), Unsigned64 attr)
{
  Pml4_entry *pml4_entry = lookup(virt);
  if (!pml4_entry->valid())
    *pml4_entry = ptab_alloc() | attr | Pml4_entry::User;

  Pdp_entry *pdp_entry = pml4_entry->pdp()->lookup(virt);
  if (!pdp_entry->valid())
    *pdp_entry = ptab_alloc() | attr | Pdp_entry::User;
  
  Pd_entry *pd_entry = pdp_entry->pdir()->lookup(virt);
  /* XXX we definitly have superpages */
  *pd_entry = phys & Config::SUPERPAGE_MASK | 
              attr | Pd_entry::Dirty | Pd_entry::Cpu_global | 
	      Pd_entry::Superpage;
}

// XXX function for early mapping initializations
PUBLIC
void
Pml4::map_page(Address phys, Address virt,
    		Address (*ptab_alloc)(), Unsigned64 attr)
{
  Pml4_entry *pml4_entry = lookup(virt);
  if (!pml4_entry->valid())
    *pml4_entry = ptab_alloc() | attr | Pml4_entry::User;
  
  Pdp_entry *pdp_entry = pml4_entry->pdp()->lookup(virt);
  if (!pdp_entry->valid())
    *pdp_entry = ptab_alloc() | attr | Pdp_entry::User;
  
  Pd_entry *pd_entry = pdp_entry->pdir()->lookup(virt);
  if (!pd_entry->valid())
    *pd_entry = ptab_alloc() | attr | Pd_entry::User;

  Pt_entry *pt_entry = pd_entry->ptab()->lookup(virt);
  *pt_entry = phys | attr | Pt_entry::Dirty | Pt_entry::Cpu_global;
}

// XXX function for mappings with resource allocation
// check if mapping already exists
// free resources if failure
PUBLIC
int
Pml4::map_slow_page(Address phys, Address virt,
    		     Mapped_allocator* ptab_alloc, Unsigned64 attr)
{
  Unsigned64 dir_attr;

  if (attr & Paging::Global)
    dir_attr = attr & (~Paging::Global);
    
  Pml4_entry *pml4_entry = lookup(virt);
  if (!pml4_entry->valid())
    {
      *pml4_entry = (Unsigned64) Mem_layout::pmem_to_phys(ptab_alloc->alloc(Config::PAGE_SHIFT));
      pml4_entry->pdp()->clear();
    }
  pml4_entry->add_attr(attr | Pml4_entry::Writable | Pml4_entry::User);
    
  Pdp_entry *pdp_entry = pml4_entry->pdp()->lookup(virt);
  if (!pdp_entry->valid())
    {
      *pdp_entry = (Unsigned64) Mem_layout::pmem_to_phys(ptab_alloc->alloc(Config::PAGE_SHIFT));
      pdp_entry->pdir()->clear();
    }
  pdp_entry->add_attr(attr | Pdp_entry::Writable | Pdp_entry::User);
    
  Pd_entry *pd_entry = pdp_entry->pdir()->lookup(virt);
  if (!pd_entry->valid())
    {
      *pd_entry = (Unsigned64) Mem_layout::pmem_to_phys(ptab_alloc->alloc(Config::PAGE_SHIFT));
      pd_entry->ptab()->clear();
    }
  pd_entry->add_attr(attr | Pd_entry::Writable | Pd_entry::User);
    
  Pt_entry *pt_entry = pd_entry->ptab()->lookup(virt);
  *pt_entry = phys | attr;

  return 1;
}

//---------------------------------------------------------------------------

PUBLIC static inline
unsigned
Pdp::virt_to_idx(Address virt)
{
  return (virt >> Pdp_entry::Shift) & Pdp_entry::Mask;
}

PUBLIC inline
Pdp_entry*
Pdp::lookup(Address virt)
{
  return _entries + virt_to_idx(virt);
}

PUBLIC inline
Pdp_entry
Pdp::entry(Address virt) const
{
  return _entries[virt_to_idx(virt)];
}

PUBLIC inline
Pdp_entry*
Pdp::index(unsigned idx)
{
  assert(idx < Entries);
  return _entries + idx;
}

PUBLIC inline
Unsigned64&
Pdp::operator[](unsigned idx)
{
  assert(idx < Entries);
  return _entries[idx].raw();
}

PUBLIC inline
Unsigned64
Pdp::operator[](unsigned idx) const
{
  assert(idx < Entries);
  return _entries[idx].raw();
}

PUBLIC inline NEEDS["cpu.h"]
void
Pdp::clear()
{
  Cpu::memset_mwords(_entries, 0, Config::PAGE_SIZE/sizeof(Mword));
}

//---------------------------------------------------------------------------

bool Pdir::_have_superpages;

PUBLIC static inline
unsigned
Pdir::virt_to_idx(Address virt)
{
  return (virt >> Pd_entry::Shift) & Pd_entry::Mask;
}

PUBLIC inline
Pd_entry*
Pdir::lookup(Address virt)
{
  return _entries + virt_to_idx(virt);
}

PUBLIC inline
Pd_entry
Pdir::entry(Address virt) const
{
  return _entries[virt_to_idx(virt)];
}

PUBLIC inline
const Pd_entry*
Pdir::index(unsigned idx) const
{
  assert(idx < 1024);
  return _entries + idx;
}

PUBLIC inline
Pd_entry*
Pdir::index(unsigned idx)
{
  assert(idx < Entries);
  return _entries + idx;
}

PUBLIC inline
Unsigned64&
Pdir::operator[](unsigned idx)
{
  assert(idx < Entries);
  return _entries[idx].raw();
}

PUBLIC inline
Unsigned64
Pdir::operator[](unsigned idx) const
{
  assert(idx < Entries);
  return _entries[idx].raw();
}

PUBLIC inline NEEDS["cpu.h"]
void
Pdir::clear()
{
  Cpu::memset_mwords(_entries, 0, Config::PAGE_SIZE/sizeof(Mword));
}


PUBLIC static inline
void
Pdir::have_superpages(bool yes)
{
  _have_superpages = yes;
}


//---------------------------------------------------------------------------

PUBLIC static inline
unsigned
Ptab::virt_to_idx(Address virt)
{
  return (virt >> Pt_entry::Shift) & Pt_entry::Mask;
}

PUBLIC inline
Pt_entry*
Ptab::lookup(Address virt)
{
  return _entries + virt_to_idx(virt);
}

PUBLIC inline
Pt_entry
Ptab::entry(Address virt) const
{
  return _entries[virt_to_idx(virt)];
}

PUBLIC inline
Pt_entry*
Ptab::index(unsigned idx)
{
  assert(idx < Entries);
  return _entries + idx;
}

PUBLIC inline
Unsigned64&
Ptab::operator[](unsigned idx)
{
  assert(idx < Entries);
  return _entries[idx].raw();
}

PUBLIC inline
Unsigned64
Ptab::operator[](unsigned idx) const
{
  assert(idx < Entries);
  return _entries[idx].raw();
}

PUBLIC inline NEEDS["cpu.h"]
void
Ptab::clear()
{
  Cpu::memset_mwords(_entries, 0, Config::PAGE_SIZE/sizeof(Mword));
}

