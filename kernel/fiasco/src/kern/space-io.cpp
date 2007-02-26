INTERFACE [ia32-io,ux-io]:

#include "config.h"
#include "io_space.h"
#include "l4_types.h"

EXTENSION class Space
{
  Io_space _io_space;
};

IMPLEMENTATION [ia32-io,ux-io]:

PRIVATE inline
void
Space::init_io_space ()
{
  _io_space.init (&_mem_space);
}

// 
// Utilities for map<Io_space> and unmap<Io_space>
// 

PUBLIC inline
Io_space*
Space::io_space()
{
  return &_io_space;
}

PUBLIC static inline NEEDS[Space::id_lookup]
bool
Space::lookup_space (Task_num id, Io_space** out_io_space)
{
  Space* s = id_lookup (id);
  if (s) 
    *out_io_space = s->io_space();

  return s;
}

/// Is this task a privileged one?
PUBLIC inline NEEDS ["l4_types.h", "config.h"]
bool 
Space::is_privileged () 
{
  // A task is privileged if it has all the IO ports mapped.
  return (!Config::enable_io_protection 
	  || (_io_space.get_io_counter() == L4_fpage::Io_port_max));
}
