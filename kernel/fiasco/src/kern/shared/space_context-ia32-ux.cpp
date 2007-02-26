INTERFACE:

#include <flux/x86/paging.h>	// for Pd_entry
#include "types.h"

class Space_context
{
public:
  static Space_context *	current();
  void				make_current();
  Address			lookup (Address addr, bool *writable) const;
  const Pd_entry *		dir() const;
  void				switchin_context();

protected:		// DATA (be careful here!):  just a page directory
  Pd_entry _dir[1024];
};

IMPLEMENTATION[ia32-ux]:

#include "config.h"
#include "kmem.h"

IMPLEMENT inline NEEDS [<flux/x86/paging.h>, "config.h", "kmem.h"]
Address
Space_context::lookup(Address addr, bool *writable) const
{
  Pd_entry pde = _dir[(addr >> PDESHIFT) & PDEMASK];

  if (!(pde & INTEL_PDE_VALID))
    return (Address) -1;

  if (pde & INTEL_PDE_SUPERPAGE)
    {
      if (writable) *writable = (pde & INTEL_PDE_WRITE);
      return (pde & Config::SUPERPAGE_MASK) | (addr & ~Config::SUPERPAGE_MASK);
    }

  Pt_entry pte = reinterpret_cast<Pt_entry *>
    ((pde & Config::PAGE_MASK) + Kmem::mem_phys)[(addr >> PTESHIFT) & PTEMASK];

  if (!(pte & INTEL_PTE_VALID))
    return (Address) -1;

  if (writable) *writable = (pte & INTEL_PTE_WRITE);
  return (pte & Config::PAGE_MASK) | (addr & ~Config::PAGE_MASK);
}

IMPLEMENT inline
const Pd_entry*
Space_context::dir () const
{
  return _dir;
}
