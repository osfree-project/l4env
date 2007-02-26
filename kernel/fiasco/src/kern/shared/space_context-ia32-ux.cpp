INTERFACE:

#include <flux/x86/paging.h>	// for pd_entry_t
#include "types.h"

class Space_context
{
public:
  static Space_context *	current();
  void				make_current();
  vm_offset_t			lookup (vm_offset_t a) const;
  const pd_entry_t *		dir() const;
  void				switchin_context();

protected:		// DATA (be careful here!):  just a page directory
  pd_entry_t _dir[1024];
};

IMPLEMENTATION[ia32-ux]:

#include "config.h"
#include "kmem.h"
#include "undef_oskit.h"

IMPLEMENT inline NEEDS [<flux/x86/paging.h>, "config.h", "kmem.h",
                        "undef_oskit.h"]
vm_offset_t
Space_context::lookup(vm_offset_t a) const
{
  pd_entry_t p = _dir[(a >> PDESHIFT) & PDEMASK];

  if (!(p & INTEL_PDE_VALID))
    return 0xffffffff;

  if (p & INTEL_PDE_SUPERPAGE)
    return (p & Config::SUPERPAGE_MASK) | (a & ~Config::SUPERPAGE_MASK);

  pt_entry_t t = reinterpret_cast<pt_entry_t *>
    ((p & Config::PAGE_MASK) + Kmem::mem_phys)[(a >> PTESHIFT) & PTEMASK];

  return (t & INTEL_PTE_VALID) ?
         (t & Config::PAGE_MASK) | (a & ~Config::PAGE_MASK) : 0xffffffff;
}

IMPLEMENT inline
const pd_entry_t*
Space_context::dir () const
{
  return _dir;
}
