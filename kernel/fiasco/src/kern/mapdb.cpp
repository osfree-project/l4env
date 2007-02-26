INTERFACE:

#include "types.h"

#define MAPDB_RAM_ONLY // mapdb_t can only handle RAM physical addresses.

enum Mapping_type { Map_mem = 0, Map_io };

struct Mapping_tree;		// forward decls
class Mapdb_tree;
class Mapdb_space;
class Physframe;

/** Represents one mapping in a mapping tree.
    Instances of Mapping ("mappings") work as an iterator over a
    mapping tree.  Mapping trees are never visible on the user level.
    Mappings are looked up directly in the mapping database (class
    Mapdb) using Mapdb::lookup().  When carrying out such a
    lookup, the mapping database locks the corresponding mapping tree
    before returning the mapping.  This mapping can the be used to
    iterate over the tree and to look up other mappings in the tree.
    The mapping tree is unlocked (and all mappings pointing into it
    become invalid) when Mapdb::free is called with any one of its
    mappings.
 */
class Mapping 
{
  friend class Mapdb;
  friend class Mapping_tree;
  friend class Jdb_mapdb;
  friend struct Physframe;

  // CREATORS
  Mapping(const Mapping&);	// this constructor is undefined.

  // DATA
  Mapping_entry _data;  
} __attribute__((packed));

/** The mapping database.
 */
class Mapdb
{
  friend class Jdb_mapdb;

private:
  // DATA
#if 0
  Mapdb_tree* _tree;
#else
  Physframe* physframe;
#endif
  Address _start;

};

IMPLEMENTATION:

/* The mapping database.

 * This implementation encodes mapping trees in very compact arrays of
 * fixed sizes, prefixed by a tree header (Mapping_tree).  Array
 * sizes can vary from 4 mappings to 4<<15 mappings.  For each size,
 * we set up a slab allocator.  To grow or shrink the size of an
 * array, we have to allocate a larger or smaller tree from the
 * corresponding allocator and then copy the array elements.
 * 
 * The array elements (Mapping) contain a tree depth element.  This
 * depth and the relative position in the array is all information we
 * need to derive tree structure information.  Here is an example:
 * 
 * array
 * element   depth
 * number    value    comment
 * --------------------------
 * 0         0        Sigma0 mapping
 * 1         1        child of element #0 with depth 0
 * 2         2        child of element #1 with depth 1
 * 3         2        child of element #1 with depth 1
 * 4         3        child of element #3 with depth 2
 * 5         2        child of element #1 with depth 1
 * 6         3        child of element #5 with depth 2
 * 7         1        child of element #0 with depth 0
 * 
 * This array is a pre-order encoding of the following tree:
 * 
 *                   0
 * 	          /     \ 
 *               1       7
 *            /  |  \                   
 *           2   3   5
 *               |   |
 *        	 4   6
       	       	   
 * IDEAS for enhancing this implementation: 

 * We often have to find a tree header corresponding to a mapping.
 * Currently, we do this by iterating backwards over the array
 * containing the mappings until we find the Sigma0 mapping, from
 * whose address we can compute the address of the tree header.  If
 * this becomes a problem, we could add one more byte to the mappings
 * with a hint (negative array offset) where to find the sigma0
 * mapping.  (If the hint value overflows, just iterate using the hint
 * value of the mapping we find with the first hint value.)  Another
 * idea (from Adam) would be to just look up the tree header by using
 * the physical address from the page-table lookup, but we would need
 * to change the interface of the mapping database for that (pass in
 * the physical address at all times), or we would have to include the
 * physical address (or just the address of the tree header) in the
 * Mapdb-user-visible Mapping (which could be different from the
 * internal tree representation).  (XXX: Implementing one of these
 * ideas is probably worthwile doing!)

 * Instead of copying whole trees around when they grow or shrink a
 * lot, or copying parts of trees when inserting an element, we could
 * give up the array representation and add a "next" pointer to the
 * elements -- that is, keep the tree of mappings in a
 * pre-order-encoded singly-linked list (credits to: Christan Szmajda
 * and Adam Wiggins).  24 bits would probably be enough to encode that
 * pointer.  Disadvantages: Mapping entries would be larger, and the
 * cache-friendly space-locality of tree entries would be lost.

 * The current handling of superpages sucks rocks both in this module
 * and in our user, map_util.cpp.  We could support multiple page sizes
 * by not using a physframe[] array only for the largest page size.
 * (Entries of that array point to the top-level mappings -- sigma0
 * mappings.)  Mapping-tree entries would then either be regular
 * mappings or pointers to an array of mappings of the next-smaller
 * size. (credits: Christan Szmajda)
 * One problem with this approach is that lookups become more expensive as
 * a task's mapping potentially can be in many subtrees.
 *
 *        physframe[]
 *        -------------------------------
 *     	  | | | | | | | | | | | | | | | | array of ptr to 4M Mapping_tree's
 *        ---|---------------------------
 *           |
 *           v a Mapping_tree
 *           ---------------
 *           |             | tree header
 *           |-------------|
 *           |             | Mapping *or* ptr to array of ptr to 4K trees
 *           |             | e.g.
 *           |      ----------------|
 *           |             |        v array of ptr to 4M Mapping_tree's
 *           ---------------        -------------------------------
 *                                  | | | | | | | | | | | | | | | |
 *                                  ---|---------------------------
 *                                     |
 *                                     v a Mapping_tree
 *                             	       ---------------
 *                                     |             | tree header
 *                                     |-------------|
 *                                     |             | Mapping
 *                                     |             |
 *                                     |             |
 *                                     |             |
 *                                     ---------------
 */

#include <cassert>
#include <cstring>
//#include <flux/x86/paging.h>

#include <auto_ptr.h>

#include "config.h"
#include "globals.h"
#include "helping_lock.h"
#include "kmem_alloc.h"
#include "kmem_slab.h"
#include "std_macros.h"

#ifndef offsetof		// should be defined in stddef.h, but isn't
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif


// 
// Mapping_tree
// 

struct Mapping_tree
{
  //friend class Mapdb;
  // DATA
  unsigned _count: 16;		///< Number of live entries in this tree.
  unsigned _size_id: 4;		///< Tree size -- see number_of_entries().
  unsigned _empty_count: 11;	///< Number of dead entries in this tree.
                                //   XXX currently never read, except in
                                //   sanity checks

  unsigned _unused: 1;		// (make this 32 bits to avoid a compiler bug)
 
  Mapping _mappings[0] __attribute__((packed));

} __attribute__((packed));





enum Mapping_depth 
{
  Depth_root = 0, Depth_max = 252, 
  Depth_subtree = 253, Depth_empty = 254, Depth_end = 255 
};

inline 
Mapping::Mapping()
{}

inline 
Mapping_entry *
Mapping::data()
{
  return &_data;
}

/** Virtual address.
    @return the virtual address at which the frame is mapped.
 */
PUBLIC inline NEEDS[Mapping::data, "config.h"]
Address 
Mapping::vaddr()
{
  return (data()->address << Config::PAGE_SHIFT);
}

/** Size of page frame.
    @return the size of the page frame -- 4K or 4M.
 */
PUBLIC inline NEEDS[Mapping::data, "config.h"]
size_t 
Mapping::size()
{
  if ( data()->size ) 
    return Config::SUPERPAGE_SIZE;
  else 
    return Config::PAGE_SIZE;
}

/** Mapping type.
    @return the type of the mapping -- memory or (Map_mem) I/O
             flexpage (Map_io).
 */
PUBLIC inline 
Mapping_type 
Mapping::type()
{
  // return data()->type;;
  return Map_mem;
}



/** free entry?.
    @return true if this is unused.
 */
inline NEEDS[Mapping_depth, Mapping::data]
bool
Mapping::unused()
{
  return (data()->depth > Depth_subtree);
}

inline NEEDS[Mapping_depth, Mapping::data]
bool
Mapping::is_end_tag()
{
  return (data()->depth == Depth_end);
}

inline NEEDS[Mapping::is_end_tag, Mapping::unused]
Mapping*
Mapping::next(const Mapping* end_of_tree)
{
  for (Mapping* m = this + 1; 
       m < end_of_tree && ! m->is_end_tag(); 
       m++)
    {
      if (! m->unused())
	return m;
    }
  
  return 0;
}

#if 0
inline NEEDS[Mapping_depth, Mapping::data]
Mapdb_tree *
Mapping::subtree()
{
  return (data()->depth == Depth_subtree) 
    ? * reinterpret_cast<Mapdb_tree**>(_data)
    : 0;
}
#endif

// A utility function to find the tree header belonging to a mapping. 
// XXX This should probably be a static member of Mapping_tree to reduce 
// coupling between Mapping and Mapping_tree.

/** Our Mapping_tree.
    @return the Mapping_tree we are in.
 */
Mapping_tree *
Mapping::tree()
{
  Mapping *m = this;

  while (m->data()->depth > Depth_root)
    {
      // jump in bigger steps if this is not a free mapping
      if (! m->unused())
	{
	  m -= m->data()->depth;
	  continue;
	}

      m--;
    }

  /* Explanation for "warning: invalid offsetof from non-POD type `struct
   *                  Mapping_tree'; use pointer to member instead"
   * POD stands for "Plain Old Data". It encompasses the types inherited
   * from C, that is, arithmetic, enumeration, pointers, structs/classes
   * (with no constructor/destructor nor virtual methods nor non-POD
   * superclasses) and unions. For the exact details see the C++ standard.
   * I think that the clever gcc is telling you is that "offsetoff" can't
   * do its work because non-POD types can have a complex memory
   * representation. The recomendation about using a pointer to member is
   * right depending on what you are trying to do. */
  return reinterpret_cast<Mapping_tree *>
    (reinterpret_cast<char *>(m) - 4/*offsetof(Mapping_tree, _mappings)*/);
}

/** Parent.
    @return parent mapping of this mapping.
 */
PUBLIC Mapping *
Mapping::parent()
{
  if (data()->depth <= Depth_root)
    {
      // Sigma0 mappings don't have a parent.
      return 0;
    }

  // Iterate over mapping entries of this tree backwards until we find
  // an entry with a depth smaller than ours.  (We assume here that
  // "special" depths (empty, end) are larger than Depth_max.)
  Mapping *m = this - 1;

  // NOTE: Depth_unused is high, so no need to test for m->unused()
  while (m->data()->depth >= data()->depth)
    m--;

  return m;
}

/** Next mapping in the mapping tree.
    @return the next mapping in the mapping tree.  If the mapping has
    children, it is the first child.  Otherwise, if the mapping has a
    sibling, it's the next sibling.  Otherwise, if the mapping is the
    last sibling or only child, it's the mapping's parent.
 */
PUBLIC Mapping *
Mapping::next_iter()
{
  Mapping_tree *t = tree();

  return next (t->end());
}

/** Next child mapping of a given parent mapping.  This
    function traverses the mapping tree like next_iter(); however, it
    stops (and returns 0) if the next mapping is ouside the subtree
    starting with parent.
    @param parent Parent mapping
    @return the next child mapping of a given parent mapping
 */
PUBLIC Mapping *
Mapping::next_child(Mapping *parent)
{
  // Find the next valid entry in the tree structure.
  Mapping *m = next_iter();

  // If we didn't find an entry, or if the entry cannot be a child of
  // "parent", return 0
  if (m == 0
      || m->data()->depth <= parent->data()->depth)
    return 0;

  return m;			// Found!
}

// Helpers

//
// Mapping-tree allocators
// 

enum Mapping_tree_size { 
  Size_factor = 4, 
  Size_id_max = 8 /* can be up to 15 (4 bits) */ 
};

class mapping_tree_allocators 
{
  auto_ptr<Kmem_slab> _allocator [Size_id_max + 1];

  friend class foo;		// Avoid warning about not being constructible
};

mapping_tree_allocators::mapping_tree_allocators()
{
  // create a slab for each mapping tree size
  for (int slab_number = 0;
       slab_number <= Size_id_max;
       slab_number++ )
    {
      unsigned elem_size = 
	(Size_factor << slab_number) * sizeof(Mapping)
	+ sizeof(Mapping_tree);

      auto_ptr<Kmem_slab> alloc (
			  new Kmem_slab(((Config::PAGE_SIZE / elem_size) < 40
			  ? 8*Config::PAGE_SIZE : Config::PAGE_SIZE),
			elem_size, Mapdb_defs::slab_align));
      _allocator[slab_number] = alloc;
    }
}

PUBLIC inline
Kmem_slab*
mapping_tree_allocators::allocator_for_treesize (int size)
{
  return _allocator[size].get();
}

/** Singleton instance of mapping_tree_allocators. */
PUBLIC static inline 
mapping_tree_allocators&
mapping_tree_allocators::instance()
{
  static mapping_tree_allocators tree_allocators;

  return tree_allocators;
}  

static inline
Kmem_slab*
allocator_for_treesize (int size)
{
  return mapping_tree_allocators::instance().allocator_for_treesize(size);
}



PUBLIC inline NEEDS[allocator_for_treesize]
void*
Mapping_tree::operator new (size_t, unsigned size_factor)
{
  return allocator_for_treesize(size_factor)->alloc();
}

PUBLIC inline
void
Mapping_tree::operator delete (void* block, size_t)
{
  if (! block)
    return;

  // Try to guess right allocator object -- XXX unportable!
  Mapping_tree* t = static_cast<Mapping_tree*>(block);

  t->check_integrity();

  allocator_for_treesize(t->_size_id)->free(block);
}

PUBLIC //inline NEEDS[Mapping_depth, Mapping_tree::last]
Mapping_tree::Mapping_tree (unsigned size_factor, unsigned page_number) 
{
  _count = 1;			// 1 valid mapping
  _size_id = size_factor;	// size is equal to Size_factor << 0
#ifndef NDEBUG
  _empty_count = 0;		// no gaps in tree representation
#endif
  
  _mappings[0].data()->depth = Depth_root;
  _mappings[0].data()->address = page_number;
  _mappings[0].data()->set_space_to_sigma0();
  _mappings[0].data()->size = 0;
	  
  _mappings[1].data()->depth = Depth_end;

  // We also always set the end tag on last entry so that we can
  // check whether it has been overwritten.
  last()->data()->depth = Depth_end;
}

PUBLIC //inline NEEDS[Mapping_depth, Mapping_tree::last]
Mapping_tree::Mapping_tree (unsigned size_factor,
				Mapping_tree* from_tree)
{
  _size_id = size_factor;
  last()->data()->depth = Depth_end;
	      
  copy_compact_tree(this, from_tree);
}

// public routines with inline implementations
PUBLIC inline
unsigned 
Mapping_tree::number_of_entries() const
{
  return Size_factor << _size_id;
}

PUBLIC inline
Mapping *
Mapping_tree::mappings()
{
  return & _mappings[0];
}

PUBLIC inline NEEDS[Mapping_tree::mappings, 
		    Mapping_tree::number_of_entries]
Mapping *
Mapping_tree::end()
{
  return mappings() + number_of_entries();
}

PUBLIC inline NEEDS[Mapping_tree::end]
Mapping *
Mapping_tree::last()
{
  return end() - 1;
}

// This function copies the elements of mapping tree src to mapping
// tree dst, ignoring empty elements (that is, compressing the
// source tree.  In-place compression is supported.
PUBLIC static void 
Mapping_tree::copy_compact_tree(Mapping_tree *dst, Mapping_tree *src)
{
  unsigned src_count = src->_count; // Store in local variable before
				    // it can get overwritten

  // Special case: cannot in-place compact a full tree
  if (src == dst && src->number_of_entries() == src_count)
    return;

  // Now we can assume the resulting tree will not be full.
  assert (src_count < dst->number_of_entries());

  dst->_count = 0;
#ifndef NDEBUG
  dst->_empty_count = 0;
#endif
  
  Mapping *d = dst->mappings();     
  
  for (Mapping *s = src->mappings();
       s;
       s = s->next (src->end()))
    {
      *d++ = *s;
      dst->_count += 1;
    }
  
  assert (dst->_count == src_count); // Same number of entries
  assert (d < dst->end());
	// Room for one more entry (the Depth_end entry)

  d->data()->depth = Depth_end;
  dst->last()->data()->depth = Depth_end;
} // copy_compact_tree()

PUBLIC inline void
Mapping_tree::check_integrity ()
{
#ifndef NDEBUG
  // Sanity checking
  assert (// Either each entry is used
	  number_of_entries() == _count + _empty_count
	  // Or the last used entry is end tag
	  || mappings()[_count + _empty_count].is_end_tag());
  
  Mapping* m = mappings();

  assert (! m->unused()		// The first entry is never unused.
	  && m->data()->space_is_sigma0()
	  && m->data()->depth == 0);

  unsigned 
    used = 0,
    dead = 0;

  while (m < end()
	   && ! m->is_end_tag())
    {
      if (m->unused())
	dead++;
      else
	used++;

      m++;
    }

  assert (_count == used);
  assert (_empty_count == dead);
#endif // ! NDEBUG
}

// 
// Physframe
// 

/** Array elements for holding frame-specific data. */
class Physframe
{
  friend class Mapdb;
  friend class Jdb_mapdb;

  // DATA
  auto_ptr<Mapping_tree> tree;
  //Mapping_tree* tree;
  Helping_lock lock;

  // CONSTRUCTORS
  Physframe ()
    // Optimization: do this using memset in operator new []
    //: tree (0)
  {}

  ~Physframe () 
  {
    assert (! lock.test());

#if 0
    if (tree)
      {
	// Find next-level trees.
	for (Mapping* m = tree->mappings(); 
	     m;
	     m = m->next (tree->end()))
	  {
	    if (m->subtree())
	      delete m->subtree();
	  }
      }
#endif
  }

  void* operator new [] (size_t size)
  {
// XXX: warning We may waste a bit of memory here
    unsigned order;
    order = (size + Config::PAGE_SIZE -1) >> Config::PAGE_SHIFT;
    order = Kmem_alloc::size_to_order(order);
    void* block = Kmem_alloc::allocator()->alloc(order);
    if (block) memset(block, 0, size);  // Optimization: See constructor
    return block;
  }

  void operator delete [] (void* block, size_t size)
  {
    if (! block)
      return;

    unsigned order;
    order = (size + Config::PAGE_SIZE -1) >> Config::PAGE_SHIFT;
    order = Kmem_alloc::size_to_order(order);
    Kmem_alloc::allocator()->free(order, block);
  }
}; // struct Physframe

// 
// class Mapdb
// 

/** Constructor.
 *@param start physical start address of RAM
 *@param end physical end address of RAM.
 */
PUBLIC
Mapdb::Mapdb(Address start, Address end)
  : physframe(new Physframe [((end - start) >> Config::PAGE_SHIFT) + 1]),
    _start(start)
{
  size_t page_number = ((end - start) >> Config::PAGE_SHIFT) + 1;
  size_t page_offset = start >> Config::PAGE_SHIFT;
  assert (physframe);

  // Call this at least once to ensure the mapping-tree allocators
  // will be created earlier than the first instance of Mapdb,
  // ensuring that they live longer.
  mapping_tree_allocators::instance();

  // create a sigma0 mapping for all physical pages
  for (unsigned page_id = 0; page_id < page_number; page_id++)
    {
      auto_ptr<Mapping_tree> new_tree (new (0) Mapping_tree (0, page_id + page_offset));
      physframe[page_id].tree = new_tree;
    }    
} // Mapdb()

/** Destructor. */
PUBLIC
Mapdb::~Mapdb()
{
  delete [] physframe;
}

/** Insert a new mapping entry with the given values as child of
    "parent".
    We assume that there is at least one free entry at the end of the
    array so that at least one insert() operation can succeed between a
    lookup()/free() pair of calls.  This is guaranteed by the free()
    operation which allocates a larger tree if the current one becomes
    to small.
    @param parent Parent mapping of the new mapping.
    @param space  Number of the address space into which the mapping is entered
    @param va     Virtual address of the mapped page.
    @param size   Size of the mapping.  For memory mappings, 4K or 4M.
    @param type   Type of the mapping (Map_mem or Map_io).
    @return If successful, new mapping.  If out of memory or mapping 
             tree full, 0.
    @post    All Mapping* pointers pointing into this mapping tree,
             except "parent" and its parents, will be invalidated.
 */
PUBLIC static Mapping *
Mapdb::insert(Mapping *parent,
	      Mapdb_space space, 
	      Address va, 
	      size_t size,
	      Mapping_type type)
{
  assert(type == Map_mem);	// we don't yet support Map_io
  (void)type;

  Mapping_tree *t = parent->tree();

  // After locating the right place for the new entry, it will be
  // stored there (if this place is empty) or the following entries
  // moved by one entry.

  // We cannot continue if the last array entry is not free.  This
  // only happens if an earlier call to free() with this mapping tree
  // couldn't allocate a bigger array.  In this case, signal an
  // out-of-memory condition.
  if (EXPECT_FALSE (! t->last()->unused()) )
    return 0;

  // If the parent mapping already has the maximum depth, we cannot
  // insert a child.
  if (EXPECT_FALSE (parent->data()->depth == Depth_max) )
    return 0;

  Mapping
    *insert = 0, 
    *free = 0;

  // - Find an insertion point for the new entry. Acceptable insertion
  //   points are either before a sibling (same depth) or at the end
  //   of the subtree.  "insert" keeps track of the last acceptable
  //   insertion point.
  // - Find a free entry in the array encoding the subtree ("free").
  //   There might be none; in this case, we stop at the end of the
  //   subtree.

  for (Mapping* m = parent + 1;
       m < t->end();
       m++)
    {
      // End of subtree?  If we reach this point, this is our insert spot.
      if (m->is_end_tag()
	  || m->data()->depth <= parent->data()->depth)
	{
	  insert = m;
	  break;
	}

      if (m->unused())
	free = m;
      else if (free		// Only look for insert spots after free
	       && m->data()->depth <= parent->data()->depth + 1)
	{
	  insert = m;
	  break;
	}
    }

  assert (insert);
  assert (free == 0 || (free->unused() && free < insert));

  // We now update "free" to point to a free spot that is acceptable
  // as well.

  if (free)
    {
      // "free" will be the latest free spot before the "insert" spot.
      // If there is anything between "free" and "insert", move it
      // upward to make space just before insert.
      while (free + 1 != insert)
	{
	  *free = *(free + 1);
	  free++;
	}

#ifndef NDEBUG
      // Tree-header maintenance
      t->_empty_count -= 1;	// Allocated dead entry
#endif
    }
  else				// free == 0
    {
      // There was no free spot in the subtree.  Move everything
      // downward until we have free space.  This is guaranteed to
      // succeed, because we ensured earlier that the last entry of
      // the array is free.

      free = insert;		// This will be the free spot

      // Find empty spot
      while (! insert->unused())
	insert++;

      // Tree maintenance: handle end tag, empty count
      if (insert->is_end_tag())
	{
	  // Need to move end tag.
	  if (insert + 1 < t->end())
	    insert++;		// Arrange for copying end tag as well
	}
#ifndef NDEBUG
      else
	t->_empty_count -= 1;	// Allocated dead entry
#endif

      // Move mappings
      while (insert > free)
	{
	  *insert = *(insert - 1);
	  --insert;
	}
    }

  t->_count += 1;		// Adding an alive entry

  // found a place to insert new child.
  free->data()->depth = Mapping_depth(parent->data()->depth + 1);
  free->data()->address = va >> Config::PAGE_SHIFT;
  free->data()->space(space.value);
  free->data()->size = (size == Config::SUPERPAGE_SIZE);

  t->check_integrity();
  return free;
} // insert()


/** Lookup a mapping and lock the corresponding mapping tree.  The returned
    mapping pointer, and all other mapping pointers derived from it, remain
    valid until free() is called on one of them.  We guarantee that at most 
    one insert() operation succeeds between one lookup()/free() pair of calls 
    (it succeeds unless the mapping tree is full).
    @param space Number of virtual address space in which the mapping 
                 was entered
    @param va    Virtual address of the mapping
    @param phys  Physical address of the mapped pag frame
    @param type  Type of the mapping (Map_mem or Map_io).
    @return mapping, if found; otherwise, 0
 */
PUBLIC Mapping *
Mapdb::lookup(Mapdb_space space,
	      Address va,
	      Address phys)	// Mapping_type type
{
  assert (phys >= _start);
  assert (phys != 0xffffffff);	// Protect against naive use of
				// virt_to_phys on user's part

  Mapping_tree *t;
  phys -= _start;

  // get and lock the tree.
  physframe[phys >> Config::PAGE_SHIFT].lock.lock();
  
  t = physframe[phys >> Config::PAGE_SHIFT].tree.get();
  assert(t);

  Mapping *m;

  for (m = t->mappings();
       m;
       m = m->next (t->end()))
    {
      if (m->data()->space() == space.value
	  && m->data()->address == va >> Config::PAGE_SHIFT)
	{
	  // found!
	  return m;
	}
    }

  // not found -- unlock tree
  physframe[phys >> Config::PAGE_SHIFT].lock.clear();

  return 0;
} // lookup()

/** Unlock the mapping tree to which the mapping belongs.  Once a tree
    has been unlocked, all Mapping instances pointing into it become
    invalid.

    A mapping tree is locked during lookup().  When the tree is
    locked, the tree may be traversed (using member functions of
    Mapping, which serves as an iterator over the tree) or
    manipulated (using insert(), free(), flush(), grant()).  Note that
    only one insert() is allowed during each locking cycle.

    @param mapping_of_tree Any mapping belonging to a mapping tree.
 */
PUBLIC void 
Mapdb::free(Mapping* mapping_of_tree)
{
  Mapping_tree *t = mapping_of_tree->tree();

  // We assume that the zeroth mapping of the tree is a sigma0
  // mapping, that is, its virtual address == the page's physical
  // address.
  Address phys_pno = t->mappings()[0].data()->address - (_start >> Config::PAGE_SHIFT);

  // We are the owner of the tree lock.
  assert (physframe[phys_pno].lock.lock_owner() == current());

  // Before we unlock the tree, we need to make sure that there is
  // room for at least one new mapping.  In particular, this means
  // that the last entry of the array encoding the tree must be free.

  // (1) When we use up less than a quarter of all entries of the
  // array encoding the tree, copy to a smaller tree.  Otherwise, (2)
  // if the last entry is free, do nothing.  Otherwise, (3) if less
  // than 3/4 of the entries are used, compress the tree.  Otherwise,
  // (4) copy to a larger tree.

  bool maybe_out_of_memory = false;

  do // (this is not actually a loop, just a block we can "break" out of)
    {
      // (1) Do we need to allocate a smaller tree?
      if (t->_size_id > 0	// must not be smallest size
	  && (t->_count << 2) < t->number_of_entries())
	{
	  auto_ptr<Mapping_tree> new_t ( 
	    new (t->_size_id - 1) Mapping_tree (t->_size_id - 1, t));

	  if (new_t.get())
	    {
	      t = new_t.get();
	      
	      // Register new tree.
	      physframe[phys_pno].tree = new_t;
	      
	      break;
	    }
	}

      // (2) Is last entry is free?
      if (t->last()->unused())
	break;			// OK, last entry is free.

      // Last entry is not free -- either compress current array
      // (i.e., move free entries to end of array), or allocate bigger
      // array.

      // (3) Should we compress the tree?  
      // We also try to compress if we cannot allocate a bigger
      // tree because there is no bigger tree size.
      if (t->_count < (t->number_of_entries() >> 2)
		      + (t->number_of_entries() >> 1)
	  || t->_size_id == Size_id_max) // cannot enlarge?
	{
	  if (t->_size_id == Size_id_max)
	    maybe_out_of_memory = true;

	  Mapping_tree::copy_compact_tree(t, t); // in-place compression

	  break;
	}

      // (4) OK, allocate a bigger array.

      auto_ptr<Mapping_tree> new_t (
	new (t->_size_id + 1) Mapping_tree (t->_size_id + 1, t));

      if (new_t.get())
	{
	  t = new_t.get();

	  // Register new tree. 
	  physframe[phys_pno].tree = new_t;
	}
      else
	{
	  // out of memory -- just do tree compression and hope that helps.
	  maybe_out_of_memory = true;

	  Mapping_tree::copy_compact_tree(t, t); // in-place compression
	}
    } 
  while (false);

  // The last entry of the tree should now be free -- exept if we're
  // out of memory.
  assert(t->last()->unused()
	 || maybe_out_of_memory);

  t->check_integrity();

  // Unlock tree.
  physframe[phys_pno].lock.clear();
} // free()

/** Delete mappings from a tree.  This function deleted mappings
    recusively.
    @param m Mapping that denotes the subtree that should be deleted.
    @param me_too If true, delete m as well; otherwise, delete only 
           submappings.
 */
PUBLIC static void 
Mapdb::flush(Mapping *m, bool me_too)
{
  assert (! m->unused());

  // This is easy to do: We just have to iterate over the array
  // encoding the tree.
  Mapping_tree *t = m->tree();
  Mapping *start_of_deletions = m;
  unsigned m_depth = m->data()->depth;
  unsigned deleted = 0, empty_elems_passed = 0;

  if (me_too)
    {
      m->data()->depth = Depth_empty;
      t->_count -= 1;
      deleted++;
    }
  else
    start_of_deletions++;

  m++;

  for (;
       m < t->end()
	 && ! m->is_end_tag();
       m++)
    {
      if (unsigned (m->data()->depth) <= m_depth)
	{
	  // Found another data element -- stop deleting.  Since we
	  // created holes in the tree representation, account for it.
#ifndef NDEBUG
	  t->_empty_count += deleted;
#endif

	  t->check_integrity();
	  return;
	}

      if (m->data()->depth == Depth_empty)
	{
	  empty_elems_passed++;
	  continue;
	}

      // Delete the element.
      m->data()->depth = Depth_empty;
      t->_count -= 1;
      deleted++;
    }

  // We deleted stuff at the end of the array -- move end tag
  if (start_of_deletions < t->end())
    {
      start_of_deletions->data()->depth = Depth_end;

#ifndef NDEBUG
      // also, reduce number of free entries
      t->_empty_count -= empty_elems_passed;
#endif
    }

  t->check_integrity();
  return;
} // flush()

/** Change ownership of a mapping.
    @param m Mapping to be modified.
    @param new_space Number of address space the mapping should be 
                     transferred to
    @param va Virtual address of the mapping in the new address space
 */
PUBLIC void 
Mapdb::grant(Mapping *m, Mapdb_space new_space, Address va)
{
  m->data()->space(new_space.value);
  m->data()->address = va >> Config::PAGE_SHIFT;
}
