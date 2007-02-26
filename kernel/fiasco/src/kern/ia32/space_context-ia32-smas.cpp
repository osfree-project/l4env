INTERFACE:

#include "config.h"             // for MAGIC_CONST
#include "linker_syms.h"

EXTENSION class Space_context
{
protected:
  static const unsigned long smas_index
    = (reinterpret_cast<unsigned long>(&_smas_area_1) >> PDESHIFT) & PDEMASK;
  static const unsigned long smas_version
    = (reinterpret_cast<unsigned long>(&_smas_version_1) >> PDESHIFT) & PDEMASK;
};

IMPLEMENTATION[ia32-smas]:

#include "kmem.h"               // for "_unused_*" virtual memory regions
#include "atomic.h"
#include <cassert>

/** Returns assigned data segment base.
 *  */
PUBLIC inline
unsigned Space_context::small_space_base() const
{
      return unsigned( (_dir[smas_index] & 0xFF000000) |
                         ((_dir[smas_index] << 16) & 0xFF0000));

}

/** Returns assigned data segment size.
 *  */
PUBLIC inline
unsigned Space_context::small_space_size() const
{
      return ((unsigned(_dir[smas_index]) & 0xFFF00) << 12);
}

/** Assigns data segment dimension.
 *  Doesn't care about correct values. Both values are rounded
 *  down to the next MB.
 *  Default is set in kdir::init.
 */
PUBLIC inline void
Space_context::set_small_space(unsigned base, unsigned size)
{
  _dir[smas_index] = (base & 0xFF000000) | ((base >> 16) & 0xF0) |
                      (((size - 1) >> 12) & 0xFFF00 );
}

/** Checks if assigned data segment seems to be in a small space.
 */
PUBLIC inline bool
Space_context::is_small_space() const
{
     return ( (_dir[smas_index] & 0xFF0000F0) != 0);
}

/** Set version of small space window.
 *  This will only increase the version, values that are less
 *  than the current one are simply ignored.
 */
PUBLIC inline NEEDS[<cassert>, "atomic.h"]
void 
Space_context::set_pdir_version(unsigned version)
{
  unsigned old;

  assert(!( version & 1 ));

  do {
    old = _dir[smas_version];
  } while ( (old < version) &&
               smp_cas(&(_dir[smas_version]), old, version));
}

/** Update the small space window from the kmem directory.
 *  Always update the full window as we have only one version
 *  counter.
 */
PUBLIC inline NEEDS["kmem.h"]
void 
Space_context::update_smas ()
{
  for ( unsigned i =  ((Kmem::smas_start >> PDESHIFT) & PDEMASK);
         i < ((Kmem::smas_end - Kmem::smas_start) >> PDESHIFT) & PDEMASK;
         i++ ) {
    _dir[i] = Kmem::dir()[i];
  }

  _dir[smas_version] = Kmem::dir()[smas_version];
}

IMPLEMENT inline NEEDS["kmem.h"]
void
Space_context::switchin_context()
{

  bool need_flush_tlb = false;

  unsigned index = (Kmem::ipc_window(0) >> PDESHIFT) & PDEMASK;

  if (_dir[index] || _dir [index + 1])
    {
      _dir[index] = 0;
      _dir[index + 1] = 0;
      need_flush_tlb = true;
    }

  index = (Kmem::ipc_window(1) >> PDESHIFT) & PDEMASK;

  if (_dir[index] || _dir [index + 1])
  {
    _dir[index] = 0;
    _dir[index + 1] = 0;
    need_flush_tlb = true;
  }

  if (is_small_space()) 
  {
    if ( Kmem::dir()[smas_version] > _dir[smas_version] ) 
    {
      update_smas();
    }
      
    if (need_flush_tlb) 
    {
      Kmem::tlb_flush();
    }
  } else if (need_flush_tlb
    || Space_context::current() != this)
  {
    make_current();
  }

  Kmem::set_gdt_user( _dir[smas_index] );

}

