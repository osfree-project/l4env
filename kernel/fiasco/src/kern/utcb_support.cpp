INTERFACE:

#include "utcb.h"

class Utcb_support
{
public:
  static Utcb *current();
  static void current(Utcb *utcb);
};

IMPLEMENTATION [arm && utcb]:

#include "mem_layout.h"

IMPLEMENT inline NEEDS["mem_layout.h"]
Utcb *
Utcb_support::current()
{ return *((Utcb**)Mem_layout::Utcb_ptr_page); }

IMPLEMENT inline NEEDS["mem_layout.h"]
void
Utcb_support::current(Utcb *utcb)
{ *((Utcb**)Mem_layout::Utcb_ptr_page) = utcb; }

