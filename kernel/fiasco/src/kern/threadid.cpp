INTERFACE:

#include "l4_types.h"

class Thread;

class threadid_t		// a shorter version of l4_threadid_t
{
  // DATA
  Thread *t;
};

IMPLEMENTATION:

#include "kmem.h"
#include "thread_util.h"

// type-conversion constructor

PUBLIC inline NEEDS ["thread_util.h"]
threadid_t::threadid_t(L4_uid const *public_id)
  : t (lookup_thread (*public_id))
{ }

PUBLIC inline NEEDS ["thread_util.h"]
threadid_t::threadid_t(L4_uid public_id)
  : t (lookup_thread (public_id))
{ }

PUBLIC inline 
threadid_t::threadid_t(Thread* thread)
  : t(thread)
{ }

PUBLIC inline 
Thread * threadid_t::lookup() const // find thread control block (tcb)
{ return t; }

PUBLIC inline NEEDS ["kmem.h"]
bool 
threadid_t::is_nil() const
{ return reinterpret_cast<unsigned long>(t) == Kmem::mem_tcbs; }

PUBLIC inline 
bool 
threadid_t::is_valid() const
{ return t; }

