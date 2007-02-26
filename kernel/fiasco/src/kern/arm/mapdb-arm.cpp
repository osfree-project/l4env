INTERFACE:

#include "types.h"

class Mapping_entry
{
private:  
  unsigned _space:11;		///< Address-space number
public:
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


IMPLEMENTATION[arm]:

#include "config.h"		// sigma0_taskno

PUBLIC inline bool Mapping_entry::space_is_sigma0() {
  return _space == Config::sigma0_taskno;
}

PUBLIC inline void Mapping_entry::set_space_to_sigma0() {
  _space = Config::sigma0_taskno;
}

PUBLIC inline unsigned Mapping_entry::space() {
  return _space;
}

PUBLIC inline void Mapping_entry::space (unsigned s) {
  _space = s;
}

