INTERFACE [arm]:

#include "types.h"
class Treemap;

class Mapping_entry
{
public:
  union 
    {
      struct 
	{
	  unsigned long space:11;	///< Address-space number
	  unsigned long _pad:1;
	  unsigned long address:20;	///< Virtual address in address space
	  unsigned long tag:11;	        ///< Unmap tag
	} data;
      Treemap *_submap;
    };
  Unsigned8 _depth;
};

IMPLEMENTATION:

class Mapdb_defs {
public:
  enum {
    slab_align = 4
  };
};

