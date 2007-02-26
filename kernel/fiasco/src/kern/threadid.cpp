INTERFACE:

#include "l4_types.h"

class Thread;

class Threadid		// a shorter version of l4_Threadid
{
  // DATA
  Thread *t;
};

IMPLEMENTATION:

#include "kmem.h"
#include "thread_util.h"

// type-conversion constructor

PUBLIC inline NEEDS ["thread_util.h"]
Threadid::Threadid(L4_uid const *public_id)
  : t (lookup_thread (*public_id))
{ }

PUBLIC inline NEEDS ["thread_util.h"]
Threadid::Threadid(L4_uid public_id)
  : t (lookup_thread (public_id))
{ }

PUBLIC inline 
Threadid::Threadid(Thread* thread)
  : t(thread)
{ }

PUBLIC inline 
Thread * Threadid::lookup() const // find thread control block (tcb)
{ return t; }

PUBLIC inline NEEDS ["kmem.h"]
bool 
Threadid::is_nil() const
{ return reinterpret_cast<unsigned long>(t) == Kmem::mem_tcbs; }

PUBLIC inline 
bool 
Threadid::is_valid() const
{ return t; }

