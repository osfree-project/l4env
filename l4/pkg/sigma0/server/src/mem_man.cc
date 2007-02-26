#include "mem_man.h"
#include "globals.h"

#include <l4/cxx/iostream.h>
#include <l4/sys/kdebug.h>

Mem_man Mem_man::_ram;

Region const *
Mem_man::find(Region const &r) const
{
  if (!r.valid())
    return 0;

  Tree::Node_type const *n = _tree.find(r);
  if (!n)
    return 0;

  if (n->key.contains(r))
    return &n->key;

  return 0;
}

bool 
Mem_man::add(Region const &r)
{
  /* try to merge with prev region */
  Region rs = r;
  if (rs.start() > 0)
    {
      rs.start(rs.start()-1);

      Tree::Node_type const *n = _tree.find(rs);
      if (n && n->key.owner() == r.owner())
	{
	  r.start(n->key.start());
	  _tree.remove(n->key);
	}
    }

  /* try to merge with next region */
  rs = r;
  if (rs.end() + 1 != 0)
    {
      rs.end(rs.end()+1);

      Tree::Node_type const *n = _tree.find(rs);
      if (n && n->key.owner() == r.owner())
	{
	  r.end(n->key.end());
	  _tree.remove(n->key);
	}
    }

  /* do throw away regions owned by myself */
  if (r.owner() == sigma0_taskno)
    return true;

  while (_tree.insert(r)==-1)
    if (!ram()->morecore())
      {
	if (debug_errors)
	  L4::cout << PROG_NAME": Out of memory\n";
	return false;
      }

  return true;
}
bool 
Mem_man::add_free(Region const &r)
{
  if (!r.valid())
    return true;

  // calculate the combined set of all overlapping regions within the tree
  while (1)
    {
      Tree::Node_type const *n = _tree.find(r);
      
      if (!n)
	break;
      
      if (n->key.start() < r.start())
	r.start(n->key.start());
      if (n->key.end() > r.end())
	r.end(n->key.end());

      _tree.remove(n->key);
    }

  return add(r);
}
 
bool
Mem_man::alloc_from(Region const *r2, Region const &_r)
{
  Region r(_r);
  if (r2->owner() && r2->owner() != r.owner())
    return false;

  if (r2->owner() == r.owner())
    return true;

  if (r == *r2)
    {
      _tree.remove(*r2);
      return add(r);
    }

  if (r.start() == r2->start())
    {
      r2->start(r.end()+1);
      //L4::cout << "move start to " << *r2 << '\n';
      add(r);
      return true;
    } 
  
  if (r.end() == r2->end())
    {
      r2->end(r.start()-1);
      //L4::cout << "shrink end to " << *r2 << '\n';
      add(r);
      return true;
    }

  Region const nr(r.end()+1, r2->end(),r2->owner());
  r2->end(r.start()-1);
  //L4::cout << "split to " << *r2 << "; " << nr << '\n';
  add(r);
  add(nr);

  return true;
}

unsigned long
Mem_man::alloc(Region const &r)
{
  if (!r.valid())
    return ~0UL;
  Region const *r2 = find(r);
  if (!r2)
    return ~0UL;

  //L4::cout << "alloc_from(" << *r2 << ", " << r << ")\n";
  if (!alloc_from(r2, r))
    return ~0UL;

  return r.start();
}

namespace {
  struct Match_end
  {
    unsigned long const size;

    Match_end(unsigned long size) : size(size) {}
    bool operator () (Mem_man::Tree::Node_type const *_n) const 
      {
	if (_n->key.owner())
	  return false;

	unsigned long st = l4_round_page(_n->key.start());

	if (st < _n->key.end() && _n->key.end()-st >= size-1)
	  return true;
	return false;
      }
  };
  struct Match_start
  {
    unsigned long const size;

    Match_start(unsigned long size) : size(size) {}
    bool operator () (Mem_man::Tree::Node_type const *_n) const
      {
	if (_n->key.owner())
	  return false;

	unsigned long st = (_n->key.start() + size-1) & ~(size-1);
	//L4::cout << "test: " << (void*)st << " - " << _n->key << '\n';

	if (st < _n->key.end() && _n->key.end()-st >= size-1)
	  return true;
	return false;
      }
  };
}

bool 
Mem_man::morecore()
{
  Tree::Node_type const *n = _tree.root();
  
  n = Mem_man::rev_iterate(n, Match_end(L4_PAGESIZE));

  if (!n)
    {
      if (debug_memory_maps)
	L4::cout << PROG_NAME": morecore did not find more free memory\n";
      return false;
    }

  Region a = Region::bs(l4_round_page(n->key.end() - L4_PAGESIZE -1), L4_PAGESIZE, sigma0_taskno);

  Page_alloc_base::free((void*)a.start());

  alloc_from(&n->key, a);

  if (debug_memory_maps)
    L4::cout << PROG_NAME": morecore: total=" << Page_alloc_base::total()/1024 
      << "kB avail=" << Page_alloc_base::allocator()->avail()/1024 
      << "kB: added " << a << '\n';
  return true;
}

unsigned long
Mem_man::alloc_first(unsigned long size, unsigned owner)
{
  Tree::Node_type const *n = _tree.root();

  n = Mem_man::iterate(n, Match_start(size));

  if (!n)
    return ~0UL;

  Region a = Region::bs((n->key.start() + size-1) & ~(size-1), size, owner);

  alloc_from(&n->key, a);

  return a.start();
}

namespace {
  struct Dump
  {
    bool operator () (Mem_man::Tree::Node_type const *_n) const
      {
	L4::cout << _n->key << '\n';
	return false;
      }
  };
}

void 
Mem_man::dump()
{ 
  Mem_man::iterate(_tree.root(), Dump());
}
  //_tree.dump(L4::cout); }

