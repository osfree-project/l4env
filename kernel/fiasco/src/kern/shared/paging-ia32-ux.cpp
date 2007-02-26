INTERFACE[ia32,ux]:

#include "types.h"
#include "config.h"
#include "assert.h"

class Ptab;
class Pdir;

class Pd_entry
{
public:
  enum
    {
      Shift         =  Config::SUPERPAGE_SHIFT,
      Mask          = (Config::SUPERPAGE_SIZE/4-1),
    };

  enum
    {
      Valid         = 0x00000001, ///< Valid
      Writable      = 0x00000002, ///< Writable
      User          = 0x00000004, ///< User accessible
      Write_through = 0x00000008, ///< Write thru
      Cacheable     = 0x00000000, ///< Cache is enabled
      Noncacheable  = 0x00000010, ///< Caching is off
      Referenced    = 0x00000020, ///< Page was referenced
      Dirty         = 0x00000040, ///< Page was modified
      Superpage     = 0x00000080, ///< Is Superpage
      Cpu_global    = 0x00000100, ///< pinned in the TLB
      L4_global     = 0x00000200, ///< pinned in the TLB
      Ptabfn        = 0xfffff000, ///< page frame number for page table
      Superfn       = 0xffc00000, ///< super page frame number
    };

private:
  Unsigned32        _raw;
  static Unsigned32 _cpu_global;
};

class Pt_entry
{
public:
  enum
    {
      Shift         =  Config::PAGE_SHIFT,
      Mask          = (Config::PAGE_SIZE/4-1),
    };

  enum
    {
      Valid         = 0x00000001, ///< Valid
      Writable      = 0x00000002, ///< Writable
      User          = 0x00000004, ///< User accessible
      Write_through = 0x00000008, ///< Write through
      Cacheable     = 0x00000000, ///< Cache is enabled
      Noncacheable  = 0x00000010, ///< Caching is off
      Referenced    = 0x00000020, ///< Page was referenced
      Dirty         = 0x00000040, ///< Page was modified
      Pat_index     = 0x00000080, ///< Page table attribute index
      Cpu_global    = 0x00000100, ///< pinned in the TLB
      L4_global     = 0x00000200, ///< pinned in the TLB
      Pfn           = 0xfffff000, ///< page frame number
    };

private:
  Unsigned32        _raw;
};

class Pdir
{
protected:
  Pd_entry _entries[1024];

private:
  static bool       _have_superpages;
};

class Ptab
{
protected:
  Pt_entry _entries[1024];
};

namespace Page 
{
  typedef Unsigned32 Attribs;

  enum Attribs_enum 
  {
    USER_NO       = 0x00000002, ///< User No access
    USER_RO       = 0x00000004, ///< User Read only
    USER_RW       = 0x00000006, ///< User Read/Write
    USER_RX       = 0x00000004, ///< User Read/Execute
    USER_XO       = 0x00000004, ///< User Execute only
    USER_RWX      = 0x00000006, ///< User Read/Write/Execute
    MAX_ATTRIBS   = 0x00000006,
  };
};


IMPLEMENTATION[ia32,ux]:

#include <cstring>
#include "mem_layout.h"
#include "regdefs.h"

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
Unsigned32
Pd_entry::raw() const
{
  return _raw;
}

PUBLIC inline
Unsigned32
Pd_entry::raw() const volatile
{
  return _raw;
}

PUBLIC inline
Unsigned32&
Pd_entry::raw()
{
  return _raw;
}

PUBLIC inline
void
Pd_entry::add_attr(Unsigned32 attr)
{
  _raw |= attr;
}

PUBLIC inline
void
Pd_entry::del_attr(Unsigned32 attr)
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
Pd_entry::operator = (Unsigned32 raw)
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
Pt_entry::operator = (Unsigned32 raw)
{
  _raw = raw;
  return *this;
}

PUBLIC inline
Unsigned32
Pt_entry::raw() const
{
  return _raw;
}

PUBLIC inline
Unsigned32&
Pt_entry::raw()
{
  return _raw;
}

PUBLIC inline
void
Pt_entry::add_attr(Unsigned32 attr)
{
  _raw |= attr;
}

PUBLIC inline
void
Pt_entry::del_attr(Unsigned32 attr)
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

bool Pdir::_have_superpages;

PUBLIC
Address
Pdir::virt_to_phys(Address virt) const
{
  Pd_entry p = entry(virt);
  if (!p.valid())
    return (Address) -1;
  if (p.superpage())
    return p.superfn() | (virt & ~Config::SUPERPAGE_MASK);

  Pt_entry e = *(p.ptab()->lookup(virt));
  if (!e.valid())
    return (Address) -1;

  return e.pfn() | (virt & ~Config::PAGE_MASK);
}

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
  assert(idx < 1024);
  return _entries + idx;
}

PUBLIC inline
Unsigned32&
Pdir::operator[](unsigned idx)
{
  assert(idx < 1024);
  return _entries[idx].raw();
}

PUBLIC inline
Unsigned32
Pdir::operator[](unsigned idx) const
{
  assert(idx < 1024);
  return _entries[idx].raw();
}

PUBLIC inline NEEDS[<cstring>]
void
Pdir::clear()
{
  memset(_entries, 0, Config::PAGE_SIZE);
}

PUBLIC
void
Pdir::map_superpage(Address phys, Address virt, 
		    Address (*ptab_alloc)(), Unsigned32 attr)
{
  Pd_entry *pd = lookup(virt);

  if (_have_superpages)
    *pd = phys | attr | Pd_entry::Dirty | Pd_entry::Superpage;

  else
    {
      *pd = ptab_alloc() | attr;
      Ptab *pt = pd->ptab();

      for (Address a = phys; a < phys+Config::SUPERPAGE_SIZE; 
	   a += Config::PAGE_SIZE, virt += Config::PAGE_SIZE)
	{
	  *(pt->lookup(virt)) = a | attr | Pt_entry::Dirty;
	}
    }
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
  assert(idx < 1024);
  return _entries + idx;
}

PUBLIC inline
Unsigned32&
Ptab::operator[](unsigned idx)
{
  assert(idx < 1024);
  return _entries[idx].raw();
}

PUBLIC inline
Unsigned32
Ptab::operator[](unsigned idx) const
{
  assert(idx < 1024);
  return _entries[idx].raw();
}

PUBLIC inline NEEDS[<cstring>]
void
Ptab::clear()
{
  memset(_entries, 0, Config::PAGE_SIZE);
}

