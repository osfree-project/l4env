INTERFACE[caps]:

#include "l4_types.h"

class slab_cache_anon;
class Ram_quota;
class Mappable;

class Obj_space
{
public:
  typedef Mappable *Phys_addr;

  static char const * const name;

  class Capability
  {
  private:
    Phys_addr _obj;
  public:
    Phys_addr obj() const { return _obj; }
    void obj(Phys_addr obj) { _obj = obj; }
    bool valid() const { return _obj; }
    void invalidate() { _obj = 0; }
  };

  enum {
    Has_superpage = 0,
    Map_page_size = 1,
    Map_superpage_size = 1,
    Map_max_address = L4_fpage::Obj_max,
    Whole_space = L4_fpage::Whole_obj_space,
    Identity_map = 0,
  };

  enum Status {
    Insert_ok = 0,		///< Mapping was added successfully.
    Insert_warn_exists,		///< Mapping already existed
    Insert_warn_attrib_upgrade,	///< Mapping already existed, attribs upgrade
    Insert_err_nomem,		///< Couldn't alloc new page table
    Insert_err_exists		///< A mapping already exists at the target addr
  };

  enum Page_attrib 
    {
      Page_no_attribs = 0,
      /// A mask which contains all mask bits
      Page_all_attribs = 1
    };


private:
  // DATA
  Capability *_caps;
  Ram_quota *_quota;

  //  Mword _bits[Cap_words];

protected:
  enum { 
    Cap_words = L4_fpage::Obj_max,
  };

  static slab_cache_anon* _slabs;
};


IMPLEMENTATION[caps]:

#include <cstring>
#include <cassert>

#include "atomic.h"
#include "config.h"
#include "cpu.h"
#include "ram_quota.h"
#include "slab_cache_anon.h"

char const * const Obj_space::name = "Obj_space";
slab_cache_anon* Obj_space::_slabs;

PUBLIC inline
Obj_space::Obj_space (Ram_quota *q)
  : _caps (0), 
    _quota(q)
{
}

PUBLIC inline NEEDS[Obj_space::caps_free]
Obj_space::~Obj_space ()
{
  caps_free();
}

PUBLIC inline
Ram_quota *
Obj_space::ram_quota() const
{ return _quota; }

PRIVATE inline NEEDS["slab_cache_anon.h", <cstring>, "ram_quota.h"]
bool
Obj_space::caps_alloc ()
{
  if (!(_caps = static_cast <Capability *> (_slabs->q_alloc(_quota))))
    return 0;

  memset (_caps, 0, sizeof (Capability) * Cap_words);
  return _caps;
}

PRIVATE inline NEEDS["slab_cache_anon.h", "ram_quota.h"]
void
Obj_space::caps_free ()
{
  if (_caps)
    {
      _slabs->q_free (_quota, _caps);
      _caps = 0;
    }
}

// 
// Utilities for map<Obj_space> and unmap<Obj_space>
// 

PUBLIC inline NEEDS["config.h"]
bool 
Obj_space::v_fabricate (unsigned /*task_id*/, Address /*snad*/,
    Phys_addr * /*address*/, Address* /*size*/,	unsigned* attribs = 0)
{
  (void)attribs;
  return false;
}

PUBLIC inline
bool 
Obj_space::v_lookup (Address virt, Phys_addr *phys = 0, Address *size = 0,
		     unsigned *attribs = 0)
{
  if (size) *size = 1;
  if (_caps && virt < Map_max_address)
    {
      Capability const &c = _caps[virt];
      if (phys) *phys = c.obj();
      if (c.valid())
	if (attribs) *attribs = 0;
	  
      return c.valid();
    }
  return false;
}

PUBLIC template<typename R>
R *
Obj_space::lookup(Address virt)
{
  Phys_addr o;
  if (!v_lookup(virt, &o))
    return 0;

  // XXX check if o is of type R
  return static_cast<R*>(o);
}


PUBLIC inline NEEDS[<cassert>]
unsigned long
Obj_space::v_delete (Address virt, unsigned long size, 
		     unsigned long /*page_attribs*/ = 0)
{
  if (!_caps || virt >= Map_max_address)
    return 0;

  (void)size;
  assert (size == 1);

  Capability &c = _caps[virt];
  if (c.valid())
    {
      c.invalidate();
      return Page_all_attribs;
    }
  
  return 0;
}

PUBLIC inline NEEDS[Obj_space::caps_alloc]
Obj_space::Status
Obj_space::v_insert (Phys_addr phys, Address virt, size_t size, 
		     unsigned /*page_attribs*/)
{
  (void)size;
  assert (size == 1);
  if (virt >= Map_max_address)
    return Insert_err_nomem;

  if (!_caps && !caps_alloc())
    return Insert_err_nomem;

  assert (size == 1);

  Capability &c = _caps[virt];

  if (c.valid())
    {
      if (c.obj() == phys)
	return Insert_warn_exists;
      else
	return Insert_err_exists;
    }

  c.obj(phys);
  return Insert_ok;
}

PUBLIC inline
bool
Obj_space::is_mappable (Address /*virt*/, size_t size) const
{ return size == 1; }

PUBLIC inline static
void
Obj_space::tlb_flush ()
{}

PUBLIC inline static
bool 
Obj_space::need_tlb_flush ()
{ return false; }

