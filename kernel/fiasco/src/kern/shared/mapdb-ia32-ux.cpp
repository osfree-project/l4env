INTERFACE [ia32,ux]:

class Mapdb_defs
{
public:
  enum {
    slab_align = 1,
  };
};

INTERFACE [ia32,ux]:

class Mapping_entry
{
public:
  unsigned space:11;		///< Address-space number
  unsigned size:1;		///< 0 = 4K mapping, 1 = 4M mapping
  unsigned address:20;		///< Virtual address in address space
  unsigned depth:8;		///< Depth in mapping tree
} __attribute__((packed));

//////////////////////////////////////////////////////////////////////////////

IMPLEMENTATION [ia32,ux]:

#include "config.h"

PUBLIC inline bool Mapping_entry::space_is_sigma0()
{ return space == Config::sigma0_taskno; }

PUBLIC inline void Mapping_entry::set_space_to_sigma0()
{ space = Config::sigma0_taskno; }
