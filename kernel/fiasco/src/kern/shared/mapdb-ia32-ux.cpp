INTERFACE [ia32,ux]:

class Mapping_entry
{
public:
  union 
    {
      struct 
	{
	  unsigned long space:11;	///< Address-space number
	  unsigned long _pad1:1;
	  unsigned long address:20;	///< Virtual address in address space
	  unsigned long tag:11;	        ///< Unmap tag
	  unsigned long _pad2:5;        // Pad to 16-bit boundary to avoid
        } data;                         // compiler bugs
      Treemap *_submap;
    };
  Unsigned8 _depth;
} __attribute__((packed));

IMPLEMENTATION:

class Mapdb_defs
{
public:
  enum {
    slab_align = 1,
  };
};

