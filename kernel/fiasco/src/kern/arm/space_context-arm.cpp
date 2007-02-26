INTERFACE:

#include "types.h"
#include "pagetable.h"

class Space_context : public Page_table
{
public:
  static Space_context *current();
  void make_current();

  using Page_table::lookup;
  Address lookup( void * ) const;
  void switchin_context();

};

IMPLEMENTATION[arm]:

#include "config.h"
#include "kmem.h"

IMPLEMENT inline
Space_context *Space_context::current()
{
  return nonull_static_cast<Space_context*>(Page_table::current());
}

IMPLEMENT inline
void Space_context::make_current()
{
  Page_table::activate();
}

IMPLEMENT inline
Address Space_context::lookup( void *a ) const
{
  P_ptr<void> ph = Page_table::lookup( a, 0, 0 );
  return ph.get_unsigned();
}


IMPLEMENT inline 
void Space_context::switchin_context()
{
  bool need_flush_tlb = false;
#if 0

  unsigned index = (kmem::ipc_window(0) >> PDESHIFT) & PDEMASK;

  if (_dir[index] || _dir [index + 1])
    {
      _dir[index] = 0;
      _dir[index + 1] = 0;
      need_flush_tlb = true;
    }

  index = (kmem::ipc_window(1) >> PDESHIFT) & PDEMASK;

  if (_dir[index] || _dir [index + 1])
    {
      _dir[index] = 0;
      _dir[index + 1] = 0;
      need_flush_tlb = true;
    }

#endif
  if (need_flush_tlb
      || Space_context::current() != this)
    {
      make_current();
    }

}
