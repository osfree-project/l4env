INTERFACE:

#include "space.h"

class Mapping_entry
{
private:
  Space *_space;		///< Address-space
public:
  unsigned size:1;		///< 0 = 4K mapping, 1 = 4M mapping
  unsigned address:20;		///< Virtual address in address space
  unsigned depth:8;		///< Depth in mapping tree
} __attribute__((packed));

class Mapdb_defs {
public:
  enum {
    slab_align = 1
  };
};

IMPLEMENTATION[ia32-ux-v4]:

#include "globals.h"
#include "space.h"

PUBLIC inline bool Mapping_entry::space_is_sigma0() {
  return _space == sigma0;
}

PUBLIC inline void Mapping_entry::set_space_to_sigma0() {
  _space = sigma0;
}

PUBLIC inline Space* Mapping_entry::space() {
  return _space;
}

PUBLIC inline void Mapping_entry::space (Space* s) {
  _space = s;
}

