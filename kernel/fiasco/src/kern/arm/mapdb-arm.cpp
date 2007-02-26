INTERFACE [arm-x0]:

#include "types.h"

class Mapping_entry
{
public:
  unsigned space:11;		///< Address-space number
  unsigned size:1;		///< 0 = 4K mapping, 1 = 4M mapping
  unsigned address:20;		///< Virtual address in address space
  unsigned depth:8;		///< Depth in mapping tree
};

class Mapdb_defs {
public:
  enum {
    slab_align = 4
  };
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm-x0]:

#include "config.h"		// sigma0_taskno

PUBLIC inline bool Mapping_entry::space_is_sigma0() 
{
  return space == Config::sigma0_taskno;
}

PUBLIC inline void Mapping_entry::set_space_to_sigma0() 
{
  space = Config::sigma0_taskno;
}
