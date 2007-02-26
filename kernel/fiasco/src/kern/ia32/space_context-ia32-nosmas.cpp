IMPLEMENTATION[ia32-nosmas]:

#include "kmem.h"

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

  if (need_flush_tlb
      || Space_context::current() != this)
    {
      make_current();
    }
}
