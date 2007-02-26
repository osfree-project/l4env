INTERFACE:

#include "types.h"

class Mapping_tree;		// forward decls
class Physframe;
class Treemap;

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
  friend class Jdb_mapdb;

  // CREATORS
  Mapping(const Mapping&);	// this constructor is undefined.

  // DATA
  Mapping_entry _data;
} __attribute__((packed));

/** A mapping database.
 */
class Mapdb
{
  friend class Jdb_mapdb;

public:
  // TYPES
  class Frame 
  {
    friend class Mapdb;
    friend class Mapdb_iterator;
    Treemap* treemap;
    Physframe* frame;

  public:
    inline size_t size() const;
    inline Address vaddr (Mapping* m) const;
  };

private:
  // DATA
  Treemap* const _treemap;
};

/** Iterator for mapping trees.  This iterator knows how to descend
    into Treemap nodes.
 */
class Mapdb_iterator
{
  Mapping_tree* _mapping_tree;
  Mapping* _parent;
  Mapping* _cursor;
  size_t   _page_size;
  Treemap *_submap;
  Physframe *_subframe;
  size_t   _submap_index;
  Address  _offs_begin, _offs_end;
  unsigned _restricted;

public:
  inline Mapping* operator->() const { return _cursor; }
  inline operator Mapping*() const   { return _cursor; }
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
       	       	   
 * The mapping database (Mapdb) is organized in a hierarchy of
 * frame-number-keyed maps of Mapping_trees (Treemap).  The top-level
 * Treemap contains mapping trees for superpages.  These mapping trees
 * may contain references to Treemaps for subpages.  (Original credits
 * for this idea: Christan Szmajda.)
 *
 *        Treemap
 *        -------------------------------
 *     	  | | | | | | | | | | | | | | | | array of ptr to 4M Mapping_tree's
 *        ---|---------------------------
 *           |
 *           v a Mapping_tree
 *           ---------------
 *           |             | tree header
 *           |-------------|
 *           |             | 4M Mapping *or* ptr to nested Treemap
 *           |             | e.g.
 *           |      ----------------| Treemap
 *           |             |        v array of ptr to 4K Mapping_tree's
 *           ---------------        -------------------------------
 *                                  | | | | | | | | | | | | | | | |
 *                                  ---|---------------------------
 *                                     |
 *                                     v a Mapping_tree
 *                             	       ---------------
 *                                     |             | tree header
 *                                     |-------------|
 *                                     |             | 4K Mapping
 *                                     |             |
 *                                     |             |
 *                                     |             |
 *                                     ---------------

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
 */

#include <cassert>
#include <cstring>

#include <auto_ptr.h>

#include "config.h"
#include "globals.h"
#include "helping_lock.h"
#include "mapped_alloc.h"
#include "paging.h"
#include "kmem_slab.h"
#include "std_macros.h"

// 
// Mapping_tree
// 
struct Mapping_tree
{
  // DATA
  unsigned _count: 16;		///< Number of live entries in this tree.
  unsigned _size_id: 4;		///< Tree size -- see number_of_entries().
  unsigned _empty_count: 11;	///< Number of dead entries in this tree.
                                //   XXX currently never read, except in
                                //   sanity checks

  unsigned _unused: 1;		// (make this 32 bits to avoid a compiler bug)
 
  Mapping _mappings[0];
};

enum Mapping_depth 
{
  Depth_root = 0, Depth_max = 252, 
  Depth_submap = 253, Depth_empty = 254, Depth_end = 255 
};

PUBLIC inline 
Mapping::Mapping()
{}

inline 
Mapping_entry *
Mapping::data()
{
  return &_data;
}

inline 
const Mapping_entry *
Mapping::data() const
{
  return &_data;
}

/** Address space.
    @return the address space into which the frame is mapped. 
 */
PUBLIC inline NEEDS [Mapping::data]
unsigned
Mapping::space() const
{
  return data()->data.space;
}

/** Set address space.
    @param space the address space into which the frame is mapped. 
 */
PUBLIC inline NEEDS [Mapping::data]
void
Mapping::set_space(unsigned space)
{
  data()->data.space = space;
}

PUBLIC inline NEEDS [Mapping::space, "config.h"]
bool
Mapping::space_is_sigma0()
{
  return space() == Config::sigma0_taskno; 
}

/** Address space.
    @return the address space into which the frame is mapped. 
 */
PUBLIC inline NEEDS [Mapping::data]
unsigned
Mapping::tag() const
{
  return data()->data.tag;
}

/** Set address space.
    @param space the address space into which the frame is mapped. 
 */
PUBLIC inline NEEDS [Mapping::data]
void
Mapping::set_tag(unsigned tag)
{
  data()->data.tag = tag;
}

/** Virtual address.
    @return the virtual address at which the frame is mapped.
 */
PUBLIC inline NEEDS[Mapping::data]
Address 
Mapping::page() const
{
  return data()->data.address;
}

/** Set virtual address.
    @param address the virtual address at which the frame is mapped.
 */
PUBLIC inline NEEDS[Mapping::data]
void 
Mapping::set_page(Address address) 
{
  data()->data.address = address;
}

/** Depth of mapping in mapping tree. */
PUBLIC inline
unsigned 
Mapping::depth() const
{
  return data()->_depth;
}

/** Set depth of mapping in mapping tree. */
PUBLIC inline
void 
Mapping::set_depth(unsigned depth)
{
  data()->_depth = depth;
}

/** free entry?.
    @return true if this is unused.
 */
PUBLIC inline NEEDS [Mapping_depth, Mapping::data]
bool
Mapping::unused() const
{
  return depth() >= Depth_empty;
}

PUBLIC inline NEEDS [Mapping_depth]
bool
Mapping::is_end_tag() const
{
  return depth() == Depth_end;
}

PUBLIC inline NEEDS[Mapping_depth]
Treemap *
Mapping::submap() const
{
  return (data()->_depth == Depth_submap) 
    ? data()->_submap
    : 0;
}

PUBLIC inline NEEDS[Mapping_depth]
void
Mapping::set_submap(Treemap *treemap)
{
  data()->_submap = treemap;
  data()->_depth = Depth_submap;
}

/** Parent.
    @return parent mapping of this mapping.
 */
PUBLIC Mapping *
Mapping::parent()
{
  if (depth() <= Depth_root)
    return 0;			// Sigma0 mappings don't have a parent.

  // Iterate over mapping entries of this tree backwards until we find
  // an entry with a depth smaller than ours.  (We assume here that
  // "special" depths (empty, end) are larger than Depth_max.)
  Mapping *m = this - 1;

  // NOTE: Depth_unused / Depth_submap are high, so no need to test
  // for them
  while (m->depth() >= depth())
    m--;

  return m;
}

// Helpers

//
// Mapping-tree allocators
// 

enum Mapping_tree_size
{
  Size_factor = 4, 
  Size_id_max = 9		// can be up to 15 (4 bits)
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
			  new Kmem_slab(elem_size, Mapdb_defs::slab_align,
			  "Mapping_tree"));
      _allocator[slab_number] = alloc;
    }
}

PUBLIC inline
Kmem_slab *
mapping_tree_allocators::allocator_for_treesize (int size)
{
  return _allocator[size].get();
}

/** Singleton instance of mapping_tree_allocators. */
PUBLIC static
mapping_tree_allocators&
mapping_tree_allocators::instance()
{
  static mapping_tree_allocators tree_allocators;

  return tree_allocators;
}  

static inline NEEDS[mapping_tree_allocators::instance,
		    mapping_tree_allocators::allocator_for_treesize]
Kmem_slab *
allocator_for_treesize (int size)
{
  return mapping_tree_allocators::instance().allocator_for_treesize(size);
}

// 
// class Mapping_tree
// 

PUBLIC inline NOEXPORT NEEDS[allocator_for_treesize]
void*
Mapping_tree::operator new (size_t, unsigned size_factor)
{
  void *t = allocator_for_treesize(size_factor)->alloc();
  return t; 
}

PUBLIC inline NOEXPORT 
              NEEDS[allocator_for_treesize, Mapping_tree::check_integrity]
void
Mapping_tree::operator delete (void* block)
{
  if (!block)
    return;

  // Try to guess right allocator object -- XXX unportable!
  Mapping_tree* t = static_cast<Mapping_tree*>(block);

  t->check_integrity();

  allocator_for_treesize(t->_size_id)->free(block);
}

PUBLIC //inline NEEDS[Mapping_depth, Mapping_tree::last]
Mapping_tree::Mapping_tree (unsigned size_factor, 
			    Address page,
			    unsigned owner)
{
  _count = 1;			// 1 valid mapping
  _size_id = size_factor;	// size is equal to Size_factor << 0
#ifndef NDEBUG
  _empty_count = 0;		// no gaps in tree representation
#endif
  
  _mappings[0].set_depth (Depth_root);
  _mappings[0].set_page (page);
  _mappings[0].set_space (owner);
  _mappings[0].set_tag (owner);
	  
  _mappings[1].set_depth (Depth_end);

  // We also always set the end tag on last entry so that we can
  // check whether it has been overwritten.
  last()->set_depth (Depth_end);
}

PUBLIC //inline NEEDS[Mapping_depth, Mapping_tree::last]
Mapping_tree::Mapping_tree (unsigned size_factor,
			    Mapping_tree* from_tree)
{
  _size_id = size_factor;
  last()->set_depth (Depth_end);
	      
  copy_compact_tree(this, from_tree);
}

// public routines with inline implementations
PUBLIC inline NEEDS[Mapping_tree_size]
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

// A utility function to find the tree header belonging to a mapping. 

/** Our Mapping_tree.
    @return the Mapping_tree we are in.
 */
PUBLIC static
Mapping_tree *
Mapping_tree::head_of (Mapping *m)
{
  while (m->depth() > Depth_root)
    {
      // jump in bigger steps if this is not a free mapping
      if (m->depth() <= Depth_max)
	{
	  m -= m->depth();
	  continue;
	}

      m--;
    }
  
  return reinterpret_cast<Mapping_tree *>
    (reinterpret_cast<char *>(m) - sizeof(Mapping_tree));
  // We'd rather like to use offsetof as follows, but it's unsupported
  // for non-POD classes.  So we instead rely on GCC's zero-length
  // array magic (sizeof(Mapping_tree) is the size of the header
  // without the array).
  // return reinterpret_cast<Mapping_tree *>
  //   (reinterpret_cast<char *>(m) - offsetof(Mapping_tree, _mappings));
}

/** Next mapping in the mapping tree.
    @param t head of mapping tree, if available
    @return the next mapping in the mapping tree.  If the mapping has
    children, it is the first child.  Otherwise, if the mapping has a
    sibling, it's the next sibling.  Otherwise, if the mapping is the
    last sibling or only child, it's the mapping's parent.
 */
PUBLIC inline NEEDS [Mapping::is_end_tag, Mapping::unused, Mapping_tree::end]
Mapping *
Mapping_tree::next (Mapping *m)
{
  for (m++; 
       m < end() && ! m->is_end_tag(); 
       m++)
    {
      if (! m->unused())
	return m;
    }
  
  return 0;
}

/** Next child mapping of a given parent mapping.  This
    function traverses the mapping tree like next(); however, it
    stops (and returns 0) if the next mapping is outside the subtree
    starting with parent.
    @param parent Parent mapping
    @return the next child mapping of a given parent mapping
 */
PUBLIC inline NEEDS[Mapping_tree::next, Mapping::depth]
Mapping *
Mapping_tree::next_child (Mapping *parent, Mapping *current_child)
{
  // Find the next valid entry in the tree structure.
  Mapping *m = next (current_child);

  // If we didn't find an entry, or if the entry cannot be a child of
  // "parent", return 0
  if (m == 0 || m->depth() <= parent->depth())
    return 0;

  return m;			// Found!
}

// This function copies the elements of mapping tree src to mapping
// tree dst, ignoring empty elements (that is, compressing the
// source tree.  In-place compression is supported.
PUBLIC static
void 
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
       s = src->next (s))
    {
      *d++ = *s;
      dst->_count += 1;
    }
  
  assert (dst->_count == src_count); // Same number of entries
  assert (d < dst->end());
	// Room for one more entry (the Depth_end entry)

  d->set_depth (Depth_end);
  dst->last()->set_depth (Depth_end);
} // copy_compact_tree()

// Don't inline this function -- it eats a lot of stack space!
PUBLIC // inline NEEDS[Mapping::data, Mapping::unused, Mapping::is_end_tag, 
       //	       Mapping_tree::end, Mapping_tree::number_of_entries]
void
Mapping_tree::check_integrity(int owner = -1)
{
  (void)owner;
#ifndef NDEBUG
  // Sanity checking
  assert (// Either each entry is used
	  number_of_entries() == static_cast<unsigned>(_count) + _empty_count
	  // Or the last used entry is end tag
	  || mappings()[_count + _empty_count].is_end_tag());
  
  Mapping* m = mappings();

  assert (! m->unused()		// The first entry is never unused.
	  && m->depth() == 0
	  && (owner == -1 || m->space() == unsigned(owner)));

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

PUBLIC inline NEEDS[Mapping::submap, Mapping_tree::next_child, Treemap::owner]
Treemap* 
Mapping_tree::find_submap (Mapping* parent)
{
  assert (! parent->submap());

  // We need just one step to find a possible submap, because they are
  // always a parent's first child.
  Mapping* m = next (parent);

  if (m && m->submap())
    {
      Treemap* t = m->submap();
      assert (t->owner() == parent->space());
      return t;
    }

  return 0;
}

// 
// class Physframe
// 

/** Array elements for holding frame-specific data. */
class Physframe
{
  friend class Mapdb;
  friend class Mapdb_iterator;
  friend class Treemap;
  friend class Jdb_mapdb;

  // DATA
  auto_ptr<Mapping_tree> tree;
  Helping_lock lock;
}; // struct Physframe

#if 0 // Optimization: do this using memset in Physframe::alloc()
inline
Physframe::Physframe ()
{}

inline
void *
Physframe::operator new (size_t, void *p) 
{ return p; }
#endif

static inline
Physframe *
Physframe::alloc(size_t size)
{
  unsigned long ps = (size*sizeof(Physframe) + Config::PAGE_SIZE - 1) 
    >> Config::PAGE_SHIFT;
  Physframe* block 
    = (Physframe*)Mapped_allocator::allocator()->unaligned_alloc (ps);
  
#if 1				// Optimization: See constructor
  if (block) 
    memset(block, 0, size * sizeof(Physframe));
#else
  assert (block);
  for (unsigned i = 0; i < size; ++i)
    new (block + i) Physframe();
#endif
  
  return block;
}

inline NOEXPORT 
       NEEDS[Mapping::submap, Mapping_tree::next, 
	     Mapping_tree::operator delete,
	     Mapping_tree::mappings, Mapping_tree::end,
	     Treemap::~Treemap, Treemap::operator delete]
Physframe::~Physframe()
{
  if (tree.get())
    {
      Lock_guard <Helping_lock> guard (&lock);

      // Find next-level trees.
      for (Mapping* m = tree->mappings(); 
	   m;
	   m = tree->next (m))
	{
	  if (m->submap())
	    delete m->submap();
	}

      tree.reset();
    }
}

static // inline NEEDS[Physframe::~Physframe]
void 
Physframe::free(Physframe *block, size_t size)
{
  for (unsigned i = 0; i < size; ++i)
    block[i].~Physframe();
  
  size = (size*sizeof(Physframe) + Config::PAGE_SIZE - 1) >> Config::PAGE_SHIFT;
  Mapped_allocator::allocator()->unaligned_free (size, block);
}

// 
// class Treemap
// 

class Treemap
{
  friend class Jdb_mapdb;

  // DATA
  size_t   _key_end;		///< Number of Physframe entries
  unsigned _owner_id;		///< ID of owner of mapping trees 
  Address  _page_offset;	///< Virt. page address in owner's addr space
  size_t   _page_size;		///< Page size of mapping trees
  Physframe* _physframe;	///< Pointer to Physframe array
  const size_t* const _sub_sizes; ///< Pointer to array of sub-page sizes
  const unsigned _sub_sizes_max;  ///< Number of valid _page_sizes entries

  friend class Mapdb_iterator;
  Mapdb_iterator _unwind;	///< Chaining information for Mapdb_iterator
};

PUBLIC
Treemap::Treemap(size_t key_end, unsigned owner_id, 
		 Address page_offset, size_t page_size,
		 const size_t* sub_sizes, unsigned sub_sizes_max)
  : _key_end (key_end),
    _owner_id (owner_id),
    _page_offset (page_offset),
    _page_size (page_size),
    _physframe (Physframe::alloc(key_end)),
    _sub_sizes (sub_sizes),
    _sub_sizes_max (sub_sizes_max)
{
  assert (_physframe);

  // Call this at least once to ensure the mapping-tree allocators
  // will be created earlier than the first instance of Treemap,
  // ensuring that they live longer.
  mapping_tree_allocators::instance();
}

PUBLIC
inline NEEDS[Physframe::free, Mapdb_iterator::neutralize]
Treemap::~Treemap()
{
  Physframe::free(_physframe, _key_end);

  // Make sure that _unwind.~Mapdb_iterator is harmless: Reinitialize it.
  _unwind.neutralize();
}

static 
slab_cache_anon*
Treemap::allocator ()
{
  static auto_ptr<Kmem_slab_simple> alloc 
    (new Kmem_slab_simple (sizeof(Treemap), sizeof(Mword), "Treemap"));

  return alloc.get();
}

PUBLIC
inline 
void*
Treemap::operator new (size_t /*size*/)
{
  return allocator()->alloc();
}

PUBLIC
inline
void
Treemap::operator delete (void *block)
{
  allocator()->free(block);
}

PUBLIC inline
size_t
Treemap::page_size() const
{
  return _page_size;
}

PUBLIC inline
unsigned
Treemap::owner() const
{
  return _owner_id;
}

PUBLIC inline
Unsigned64
Treemap::end_addr() const
{
  return static_cast<Unsigned64>(_page_size) * _key_end + _page_offset;
}

PUBLIC inline NEEDS[Mapping::page, "paging.h"]
Address
Treemap::vaddr (Mapping* m) const
{
  return Paging::canonize (m->page() * _page_size);
}

PUBLIC inline NEEDS[Mapping::set_page]
void
Treemap::set_vaddr (Mapping* m, Address address) const
{
  m->set_page (address / _page_size);
}

PUBLIC
Physframe*
Treemap::tree (Address key)
{
  assert (key < _key_end);

  Physframe *f = &_physframe[key];

  f->lock.lock();

  if (! f->tree.get())
    {
      auto_ptr<Mapping_tree> new_tree
	(new (0) Mapping_tree (0, key + _page_offset / _page_size, 
			       _owner_id));

      f->tree = new_tree;
    }

  return f;
}

PUBLIC
Physframe*
Treemap::frame (Address key)
{
  assert (key < _key_end);
  return &_physframe[key];
}

PUBLIC
bool
Treemap::lookup (Address key, unsigned search_space, Address search_va,
		 Mapping** out_mapping, Treemap** out_treemap,
		 Physframe** out_frame)
{
  // get and lock the tree.
  assert (key / _page_size < _key_end);
  Physframe *f = tree (key / _page_size); // returns locked frame
  assert (f);

  Mapping_tree *t = f->tree.get();
  assert (t);
  assert (t->mappings()[0].space() == _owner_id);
  assert (vaddr(t->mappings()) == (key & ~(_page_size - 1)) + _page_offset);

  Mapping *m;
  bool ret = false;

  for (m = t->mappings(); m; m = t->next (m))
    {
      if (Treemap *sub = m->submap())
	{
	  // XXX Recursion.  The max. recursion depth should better be
	  // limited!
	  if ((ret = sub->lookup (key & (_page_size - 1), 
				  search_space, search_va, 
				  out_mapping, out_treemap, out_frame)))
	    break;	      
	}
      else if (m->space() == search_space 
	       && vaddr(m) == (search_va & ~(_page_size - 1)))
	{
	  *out_mapping = m;
	  *out_treemap = this;
	  *out_frame = f;
	  return true;		// found! -- return locked
	}
    }

  // not found, or found in submap -- unlock tree
  f->lock.clear();

  return ret;
}

PUBLIC
Mapping *
Treemap::insert (Physframe* frame,
		 Mapping* parent,
		 unsigned space, 
		 Address va, 
		 Address phys,
		 size_t size)
{
  Mapping_tree* t = frame->tree.get();
  Treemap* submap = 0;
  bool insert_submap = _page_size != size;

  // Inserting subpage mapping?  See if we can find a submap.  If so,
  // we don't have to allocate a new Mapping entry.
  if (insert_submap)
    {
      assert (_page_size > size);

      submap = t->find_submap (parent);
    }

  if (! submap) // Need allocation of new entry -- for submap or
		// normal mapping
    {
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
      if (EXPECT_FALSE (parent->depth() == Depth_max) )
	return 0;
      
      Mapping
	*insert = 0, 
	*free = 0;
      
      // - Find an insertion point for the new entry. Acceptable insertion
      //   points are either before a sibling (same depth) or at the end
      //   of the subtree; for submap insertions, it's always before
      //   the first sibling.  "insert" keeps track of the last
      //   acceptable insertion point.
      // - Find a free entry in the array encoding the subtree ("free").
      //   There might be none; in this case, we stop at the end of the
      //   subtree.
      
      for (Mapping* m = parent + 1;
	   m < t->end();
	   m++)
	{
	  // End of subtree?  If we reach this point, this is our insert spot.
	  if (m->is_end_tag()
	      || m->depth() <= (insert_submap 
				? parent->depth() + 1
				: parent->depth()))
	    {
	      insert = m;
	      break;
	    }
	  
	  if (m->unused())
	    free = m;
	  else if (free		// Only look for insert spots after free
		   && m->depth() <= parent->depth() + 1)
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
      
      // found a place to insert new child (free).
  
      // Check for submap entry.
      if (! insert_submap)	// Not a submap entry
	{
	  free->set_depth (parent->depth() + 1);
	  free->set_space (space);
	  free->set_tag (space);
	  set_vaddr (free, va);
	  
	  t->check_integrity(_owner_id);
	  return free;
	}

      assert (_sub_sizes_max > 0);

      submap = new Treemap (_page_size / _sub_sizes[0],
			    parent->space(), vaddr (parent), 
			    _sub_sizes[0], _sub_sizes + 1, 
			    _sub_sizes_max - 1);
      if (! submap)
	return 0;

      free->set_submap (submap);
    }

  Physframe* subframe = submap->tree ((phys & (_page_size - 1)) 
				        / submap->_page_size);
  Mapping_tree* subtree = subframe->tree.get();

  if (! subtree)
    return 0;

  // XXX recurse.
  Mapping* ret = submap->insert (subframe, subtree->mappings(), space, va, 
				 (phys & (_page_size - 1)),
				 size);
  submap->free (subframe);

  return ret;
} // Treemap::insert()

PUBLIC
void 
Treemap::free (Physframe* f)
{
  // We are the owner of the tree lock.
  assert (f->lock.lock_owner() == current());

  // Before we unlock the tree, we need to make sure that there is
  // room for at least one new mapping.  In particular, this means
  // that the last entry of the array encoding the tree must be free.

  // (1) When we use up less than a quarter of all entries of the
  // array encoding the tree, copy to a smaller tree.  Otherwise, (2)
  // if the last entry is free, do nothing.  Otherwise, (3) if less
  // than 3/4 of the entries are used, compress the tree.  Otherwise,
  // (4) copy to a larger tree.

  Mapping_tree *t = f->tree.get();
  bool maybe_out_of_memory = false;

  do // (this is not actually a loop, just a block we can "break" out of)
    {
      // (1) Do we need to allocate a smaller tree?
      if (t->_size_id > 0	// must not be smallest size
	  && (static_cast<unsigned>(t->_count) << 2) < t->number_of_entries())
	{
	  auto_ptr<Mapping_tree> new_t ( 
	    new (t->_size_id - 1) Mapping_tree (t->_size_id - 1, t));

	  if (new_t.get())
	    {
	      t = new_t.get();
	      
	      // Register new tree.
	      f->tree = new_t;
	      
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
	  f->tree = new_t;
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

  t->check_integrity(_owner_id);

  // Unlock tree.
  f->lock.clear();
} // Treemap::free()

PUBLIC
void 
Treemap::flush (Physframe* f, Mapping* parent, bool me_too,
		unsigned restricted, Address offs_begin, Address offs_end)
{
  assert (! parent->unused());

  // This is easy to do: We just have to iterate over the array
  // encoding the tree.
  Mapping_tree *t = f->tree.get();
  Mapping *start_of_deletions = parent;
  unsigned p_depth = parent->depth();
  unsigned deleted = 0;
#ifndef NDEBUG
  unsigned empty_elems_passed = 0;
#endif

  if (me_too)
    {
      parent->set_depth (Depth_empty);
      t->_count -= 1;
      deleted++;
    }
  else
    start_of_deletions++;

  unsigned m_depth = p_depth + 1;
  bool skip_subtree = false;

  for (Mapping* m = parent + 1;
       m < t->end()
	 && ! m->is_end_tag();
       m++)
    {
      if (unsigned (m->depth()) <= p_depth)
	{
	  // Found another data element -- stop deleting.  Since we
	  // created holes in the tree representation, account for it.
#ifndef NDEBUG
	  t->_empty_count += deleted;
#endif

	  t->check_integrity(_owner_id);
	  return;
	}

      if (m->depth() == Depth_empty)
	{
#ifndef NDEBUG
	  empty_elems_passed++;
#endif
	  continue;
	}

      // m is a submap or a regular mapping.
      if (skip_subtree)
	{ 
	  if (m->depth() > p_depth + 1)	// Can use m->depth() even for submaps.
	    {
	      start_of_deletions++;
	      continue;
	    }

	  skip_subtree = false;
	}

      if (Treemap* submap = m->submap())
	{
	  if (! me_too
	      // Check for immediate child.  Note: It's a bad idea to
	      // call m->parent() because that would iterate backwards
	      // over already-deleted entries.
	      && m_depth == p_depth + 1
	      && (offs_begin > 0 
		  || offs_end < submap->_page_size * submap->_key_end))
	    {
	      for (unsigned i = offs_begin / submap->_page_size;
		   i < ((offs_end + submap->_page_size - 1)
			/ submap->_page_size);
		   i++)
		{
		  Address page_offs_begin = i * submap->_page_size;
		  Address page_offs_end = page_offs_begin + submap->_page_size;
		  
		  Physframe* subframe = submap->frame(i);

		  Lock_guard <Helping_lock> guard (&subframe->lock);

		  if (! restricted
		      && offs_begin <= page_offs_begin 
		      && offs_end >= page_offs_end)
		    subframe->tree.reset();
		  else
		    submap->flush (subframe, subframe->tree->mappings(), false,
				   restricted,
				   page_offs_begin > offs_begin 
				     ? 0 : offs_begin & (_page_size - 1),
				   page_offs_end < offs_end 
				     ? _page_size
				     : ((offs_end - 1) 
					& (_page_size - 1)) + 1);
		}

	      start_of_deletions++;
	      continue;
	    }
	  else
	    delete submap;	// Wow, that was fast!
	}
      else			// Found a real mapping
	{
	  m_depth = m->depth();

	  if (restricted 
	      // Check for immediate child -- see comment above
	      && m_depth == p_depth + 1
	      && m->tag() != restricted)
	    {
	      assert (! me_too);

	      // Skip over subtree of not-to-be-flushed child.
	      skip_subtree = true;
	      start_of_deletions++;
	      continue;
	    }
	}

      // Delete the element.
      m->set_depth (Depth_empty);
      t->_count -= 1;
      deleted++;
    }

  // We deleted stuff at the end of the array -- move end tag
  if (start_of_deletions < t->end())
    {
      start_of_deletions->set_depth (Depth_end);

#ifndef NDEBUG
      // also, reduce number of free entries
      t->_empty_count -= empty_elems_passed;
#endif
    }

  t->check_integrity(_owner_id);
  return;
} // Treemap::flush()

PUBLIC
void 
Treemap::grant (Physframe* f, Mapping* m, unsigned new_space, Address va)
{
  m->set_space (new_space);
  set_vaddr (m, va);

  if (Treemap* submap = f->tree.get()->find_submap(m))
    {
      submap->_owner_id = new_space;
      submap->_page_offset = va;
      
      for (size_t key = 0;
	   key < submap->_key_end;
	   ++key)
	{
	  Physframe* subframe = submap->frame(key);
	  if (Mapping_tree* tree = subframe->tree.get())
	    {
	      Lock_guard<Helping_lock> guard (&subframe->lock);
	      tree->mappings()->set_space (new_space);
	      set_vaddr (tree->mappings(), va + key * submap->_page_size);
	    }
	}
    }
}

// 
// class Mapdb_iterator
// 

PUBLIC inline 
Mapdb_iterator::Mapdb_iterator ()
  : _submap (0),
    _submap_index (0)
{
}

/** Create a new mapping-tree iterator.  If parent is the result of a
    fresh lookup (and not the result of an insert operation), you can
    pass the corresponding Mapdb::Frame for optimization.
 */
PUBLIC inline NEEDS[Mapping::submap]
Mapdb_iterator::Mapdb_iterator (const Mapdb::Frame& f, Mapping* parent)
  : _mapping_tree (f.frame->tree.get()),
    _parent (parent),
    _cursor (parent),
    _page_size (f.treemap->_page_size),
    _submap (0),
    _subframe (0),
    _submap_index (0),
    _offs_begin (0),
    _offs_end (~0U),
    _restricted (0)
{
  assert (_mapping_tree == Mapping_tree::head_of (parent));
  assert (! parent->submap());
  ++*this;
}

/** Create a new mapping-tree iterator.  If parent is the result of a
    fresh lookup (and not the result of an insert operation), you can
    pass the corresponding Mapdb::Frame for optimization.
 */
PUBLIC inline NEEDS[Mapping::submap, Treemap::vaddr]
Mapdb_iterator::Mapdb_iterator (const Mapdb::Frame& f, Mapping* parent,
				unsigned restrict_tag, 
				Address va_begin, Address va_end)
  : _mapping_tree (f.frame->tree.get()),
    _parent (parent),
    _cursor (parent),
    _page_size (f.treemap->_page_size),
    _submap (0),
    _subframe (0),
    _submap_index(0),
    _restricted (restrict_tag)
{
  assert (_mapping_tree == Mapping_tree::head_of (parent));
  assert (! parent->submap());

  if (va_begin < f.treemap->vaddr(parent))
    va_begin = f.treemap->vaddr(parent);
  if (va_begin > va_end)
    va_begin = va_end;

  _offs_begin = va_begin - f.treemap->vaddr(parent);
  _offs_end = va_end - f.treemap->vaddr(parent);

  ++*this;
}

PUBLIC inline NEEDS[Physframe, <auto_ptr.h>, "helping_lock.h", Treemap]
Mapdb_iterator::~Mapdb_iterator ()
{
  // unwind lock information
  while (_submap)
    {
      if (_subframe)
	_subframe->lock.clear();

      *this = _submap->_unwind;
    }
}

/** Make sure that the destructor does nothing.  Handy for scratch
    iterators such as Treemap::_unwind. */
PUBLIC inline
void
Mapdb_iterator::neutralize ()
{
  _submap = 0;
}

PUBLIC inline 
size_t
Mapdb_iterator::size ()
{
  return _page_size;
}

PUBLIC inline NEEDS[Mapping_tree::mappings, Mapping_tree::next_child, 
		    Mapping::submap]
Mapdb_iterator&
Mapdb_iterator::operator++ ()
{
  for (;;)
    {
      _cursor = _mapping_tree->next_child (_parent, _cursor);
      
      if (_cursor && ! _cursor->submap())
	{			// Found a new regular child mapping.
	  if (_restricted
	      && _cursor->depth() == _parent->depth() + 1)
	    {			// Direct descendant of parent
	      // Skip over subtree unless subtree tag == restriction
	      while (_cursor 
		     && _cursor->tag() != _restricted)
		{
		  do
		    {
		      _cursor = _mapping_tree->next_child (_parent, _cursor);
		    }
		  while (_cursor 
			 && _cursor->depth() > _parent->depth() + 1);
		}

	      assert (! _cursor
		      || (! _cursor->submap()
			  && _cursor->depth() == _parent->depth() + 1
			  && _cursor->tag() == _restricted));
	    }

	  if (_cursor)
	    return *this;
	}

      if (_cursor)		// _cursor is a submap
	{
	  Treemap* submap = _cursor->submap();
	  assert (submap);

	  // Before descending into array of subpages, save iterator
	  // state for later unwinding to this point.
	  submap->_unwind = *this;

	  // For subpages of original parent mapping, apply possible tag and
	  // virtual-address restrictions.  (Do not apply restrictions
	  // to other subpages because operations on a part of a
	  // superpage always affect *complete* child-superpages, as
	  // we do not support splitting out parts of superpages.
	  Address parent_size = submap->_page_size * submap->_key_end;

	  if (_offs_end > parent_size)
	    _offs_end = parent_size;

	  if (_cursor->parent() == _parent)
	    {			// Submap of iteration root
	      _offs_begin = _offs_begin & (parent_size - 1);
	      _offs_end = ((_offs_end - 1) & (parent_size - 1)) + 1;
	    }
	  else			// Submap of a child mapping
	    {
	      _offs_begin = 0;
	      _offs_end = parent_size;

	      _restricted = 0;	// Lift task-tag restriction
	    }

	  // Initialize rest of iterator for subframe iteration
	  _submap = submap;
	  _page_size = _submap->_page_size;
	  _subframe = 0;
	  _submap_index = _offs_begin / _page_size;
	  _mapping_tree = 0;
	  _parent = _cursor = 0;
	}

      else if (! _submap)	// End of iteration
	return *this;
	  
      // Clear previously acquired lock.
      if (_subframe)
	{
	  _subframe->lock.clear();
	  _subframe = 0;
	}

      // Find a new subframe.
      Physframe* f = 0;

      unsigned end_offs = 
	(_offs_end + _submap->_page_size - 1) / _submap->_page_size;

      assert (end_offs <= _submap->_key_end);

      for (;
	   _submap_index < end_offs;
	   )
	{
	  f = _submap->frame(_submap_index++);
	  if (f->tree.get())
	    break;
	}
      
      if (f && f->tree.get())	// Found a subframe
	{
	  _subframe = f;
	  f->lock.lock();	// Lock it
	  _mapping_tree = f->tree.get();
	  _parent = _cursor = _mapping_tree->mappings();
	  continue;
	}
      
      // No new subframes found -- unwind to superpage tree
      *this = _submap->_unwind;
    }
}


// 
// class Mapdb
// 

/** Constructor.
    @param End physical end address of RAM.  (Used only if 
           Config::Mapdb_ram_only is true.) 
 */
PUBLIC
Mapdb::Mapdb(Address end_frame, const size_t* page_sizes, 
	     unsigned page_sizes_max)
  : _treemap (new Treemap(end_frame, Config::sigma0_taskno,
			  /* va offset = */ 0,
			  page_sizes[0], page_sizes + 1, page_sizes_max - 1))
{
  assert (_treemap);
} // Mapdb()

/** Destructor. */
PUBLIC
Mapdb::~Mapdb()
{
  delete _treemap;
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
    @return If successful, new mapping.  If out of memory or mapping 
           tree full, 0.
    @post  All Mapping* pointers pointing into this mapping tree,
           except "parent" and its parents, will be invalidated.
 */
PUBLIC
Mapping *
Mapdb::insert (const Mapdb::Frame& frame,
	       Mapping* parent,
	       unsigned space, 
	       Address va, 
	       Address phys,
	       size_t size)
{
  return frame.treemap->insert (frame.frame, parent, space, va, 
				phys, size);
} // insert()


/** 
 * Lookup a mapping and lock the corresponding mapping tree.  The returned
 * mapping pointer, and all other mapping pointers derived from it, remain
 * valid until free() is called on one of them.  We guarantee that at most 
 * one insert() operation succeeds between one lookup()/free() pair of calls 
 * (it succeeds unless the mapping tree is full).
 * @param space Number of virtual address space in which the mapping 
 *              was entered
 * @param va    Virtual address of the mapping
 * @param phys  Physical address of the mapped pag frame
 * @return mapping, if found; otherwise, 0
 */
PUBLIC
bool
Mapdb::lookup (unsigned space,
	       Address va,
	       Address phys,
	       Mapping** out_mapping,
	       Mapdb::Frame* out_lock)
{
  assert (phys != (Address) -1); // Protect against naive use of
				 // virt_to_phys on user's part
  return _treemap->lookup (phys, space, va, out_mapping, 
			   & out_lock->treemap, & out_lock->frame);
}

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
PUBLIC
static void 
Mapdb::free (const Mapdb::Frame& f)
{
  f.treemap->free(f.frame);
} // free()

/** Delete mappings from a tree.  This function deletes mappings
    recusively.
    @param m Mapping that denotes the subtree that should be deleted.
    @param me_too If true, delete m as well; otherwise, delete only 
           submappings.
 */
PUBLIC static
void 
Mapdb::flush (const Mapdb::Frame& f, Mapping *m, bool me_too,
	      unsigned restrict_tag, Address va_start, Address va_end)
{
  Address size = f.treemap->page_size();
  Address offs_begin = va_start > f.treemap->vaddr(m) 
    ? va_start - f.treemap->vaddr(m) : 0;
  Address offs_end = va_end > f.treemap->vaddr(m) + size 
    ? size : va_end - f.treemap->vaddr(m);

  f.treemap->flush (f.frame, m, me_too, restrict_tag, offs_begin, offs_end);
} // flush()

/** Change ownership of a mapping.
    @param m Mapping to be modified.
    @param new_space Number of address space the mapping should be 
                     transferred to
    @param va Virtual address of the mapping in the new address space
 */
PUBLIC
void 
Mapdb::grant (const Mapdb::Frame& f, Mapping *m, unsigned new_space, 
	      Address va)
{
  f.treemap->grant (f.frame, m, new_space, va);
}

/** Return page size of given mapping and frame. */
PUBLIC static inline NEEDS[Treemap::page_size]
size_t
Mapdb::size (const Mapdb::Frame& f, Mapping * /*m*/)
{
  // XXX add special case for _mappings[0]: Return superpage size.
  return f.treemap->page_size();
}

PUBLIC static inline NEEDS[Treemap::vaddr]
Address
Mapdb::vaddr (const Mapdb::Frame& f, Mapping* m)
{
  return f.treemap->vaddr(m);
}

// 
// class Mapdb::Frame
// 

IMPLEMENT inline NEEDS[Treemap::page_size]
size_t
Mapdb::Frame::size () const
{
  return treemap->page_size();
}

IMPLEMENT inline NEEDS[Treemap::vaddr]
Address
Mapdb::Frame::vaddr (Mapping* m) const
{
  return treemap->vaddr(m);

}

PUBLIC inline NEEDS [Treemap::end_addr]
bool
Mapdb::valid_address(Address phys)
{
  return phys < _treemap->end_addr ();
}
