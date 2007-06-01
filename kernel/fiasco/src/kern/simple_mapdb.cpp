INTERFACE:

#include "types.h"
#include "mapping.h"

class Mappable;
class Mapping_tree;

/** A mapping database.
 */
class Simple_mapdb
{
public:
  // TYPES

  class Iterator;
  class Frame 
  {
    friend class Simple_mapdb;
    friend class Simple_mapdb::Iterator;
    Mappable* frame;

  public:
    inline size_t size() const;
  };
  
  class Iterator
  {
    Mapping_tree* _mapping_tree;
    Mapping* _parent;
    Mapping* _cursor;
    Address  _offs_begin, _offs_end;
    unsigned _restricted;

  public:
    inline Mapping* operator->() const { return _cursor; }
    inline operator Mapping*() const   { return _cursor; }
    Iterator() : _cursor(0) {}
    Iterator(Frame const &f, Mapping *parent);
    Iterator(Frame const &f, Mapping *parent, unsigned restrict_tag,
	Address va_begin, Address va_end);

    inline size_t size() const;
    Iterator &operator ++ ();
  };
};


IMPLEMENTATION:

#include <cassert>
#include <cstring>

#include <auto_ptr.h>

#include "config.h"
#include "globals.h"
#include "ram_quota.h"
#include "std_macros.h"
#include "mappable.h"


// 
// class Mapdb_iterator
// 

IMPLEMENT 
Simple_mapdb::Iterator::Iterator(Frame const &f, Mapping *parent)
: _mapping_tree(f.frame->tree.get()),
  _parent(parent), _cursor(parent),
  _offs_begin (0), _offs_end (~0U), _restricted (0)
{ ++*this; }

IMPLEMENT 
Simple_mapdb::Iterator::Iterator(Frame const &f, Mapping *parent,
    unsigned restrict_tag, Address va_begin, Address va_end)
: _mapping_tree(f.frame->tree.get()),
  _parent(parent), _cursor(parent),
  _offs_end(va_end),
  _restricted (restrict_tag)
{ 
  if (va_begin > va_end)
    va_begin = va_end;
  _offs_begin = va_begin;
  ++*this; 
}


IMPLEMENT inline
size_t
Simple_mapdb::Iterator::size () const
{ return 1; }

IMPLEMENT
Simple_mapdb::Iterator&
Simple_mapdb::Iterator::operator++ ()
{
  _cursor = _mapping_tree->next_child (_parent, _cursor);

  if (_cursor)
    {			// Found a new regular child mapping.
      assert (!_cursor->submap());
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
    }

  return *this;
}


// 
// class Mapdb
// 


PUBLIC static inline 
Address
Simple_mapdb::vaddr (const Frame&, Mapping* m)
{ return m->page(); }

PUBLIC static inline
bool
Simple_mapdb::valid_address(Mappable *obj)
{ return obj; }

PUBLIC static inline NEEDS[<cassert>]
Mappable *
Simple_mapdb::page_address(Mappable *addr, unsigned long size)
{
  (void)size;
  assert (size == 1);
  return addr;
}

PUBLIC static inline NEEDS[<cassert>]
Mappable *
Simple_mapdb::subpage_address(Mappable *addr, unsigned long offset)
{
  (void)offset;
  assert (offset == 0);
  return addr;
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
PUBLIC static
Mapping *
Simple_mapdb::insert (const Frame& frame,
    Mapping* parent, unsigned space, Address va, Mappable *o, 
    Address size)
{
  (void)size;
  (void)o;
  assert (o == frame.frame);
  assert (size == 1);
  return frame.frame->insert(parent, space, va); 
} 


/** 
 * Lookup a mapping and lock the corresponding mapping tree.  The returned
 * mapping pointer, and all other mapping pointers derived from it, remain
 * valid until free() is called on one of them.  We guarantee that at most 
 * one insert() operation succeeds between one lookup()/free() pair of calls 
 * (it succeeds unless the mapping tree is fu68,ll).
 * @param space Number of virtual address space in which the mapping 
 *              was entered
 * @param va    Virtual address of the mapping
 * @param phys  Physical address of the mapped pag frame
 * @return mapping, if found; otherwise, 0
 */
PUBLIC static
bool
Simple_mapdb::lookup (unsigned space, Address va, Mappable *obj,
    Mapping** out_mapping, Frame* out_lock)
{
  *out_mapping = obj->lookup (space, va);
  if (*out_mapping)
    {
      out_lock->frame = obj;
      return true;
    }

  return false;
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
Simple_mapdb::free (const Frame& f)
{
  f.frame->pack();
  f.frame->lock.clear();
} // free()

/** Delete mappings from a tree.  This function deletes mappings
    recusively.
    @param m Mapping that denotes the subtree that should be deleted.
    @param me_too If true, delete m as well; otherwise, delete only 
           submappings.
 */
PUBLIC static
void 
Simple_mapdb::flush (const Frame& f, Mapping *m, bool me_too,
	      unsigned restrict_tag, Address va_start, Address va_end)
{
  f.frame->tree.get()->flush (m, me_too, restrict_tag, va_start, va_end,
      Simple_tree_submap_ops());
} // flush()

/** Change ownership of a mapping.
    @param m Mapping to be modified.
    @param new_space Number of address space the mapping should be 
                     transferred to
    @param va Virtual address of the mapping in the new address space
 */
PUBLIC static
bool
Simple_mapdb::grant (const Frame& f, Mapping *m, unsigned new_space, 
	      Address va)
{
  return f.frame->tree.get()->grant (m, new_space, va,
      Simple_tree_submap_ops());
}

