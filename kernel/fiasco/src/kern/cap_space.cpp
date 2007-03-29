INTERFACE[caps]:

#include "l4_types.h"

class slab_cache_anon;
class Ram_quota;

class Cap_space
{
public:
  static char const * const name;
  typedef Address Phys_addr;
  enum {
    Has_superpage = 1,
    Map_page_size = 1,
    Map_superpage_size = L4_uid::Max_tasks,
    Map_max_address = L4_uid::Max_tasks,
    Whole_space = L4_fpage::Whole_cap_space,
    Identity_map = 1,
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
  Mword* _bits;
  bool   _full;
  bool   _task_caps;
  Ram_quota *_quota;

  //  Mword _bits[Cap_words];

protected:
  enum { 
    Cap_words = L4_uid::Max_tasks / MWORD_BITS,
    Cap_word_bits = MWORD_BITS
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

char const * const Cap_space::name = "Cap_space";
slab_cache_anon* Cap_space::_slabs;

PUBLIC inline
Cap_space::Cap_space (Ram_quota *q)
  : _bits (0), 
    _full (false),
    _quota(q)
{
}

PUBLIC inline NEEDS[Cap_space::bits_free]
Cap_space::~Cap_space ()
{
  bits_free();
}

PUBLIC inline
Ram_quota *
Cap_space::ram_quota() const
{ return _quota; }

PRIVATE inline NEEDS["slab_cache_anon.h", <cstring>, "ram_quota.h"]
bool
Cap_space::bits_alloc ()
{
  if (EXPECT_FALSE(!(_bits = static_cast <Mword*> (_slabs->q_alloc(_quota)))))
    return 0;

  memset (_bits, 0, sizeof (Mword) * Cap_words);
  return _bits;
}

PRIVATE inline NEEDS["slab_cache_anon.h", "ram_quota.h"]
void
Cap_space::bits_free ()
{
  if (_bits)
    {
      _slabs->free (_bits);
      _quota->free(sizeof (Mword) * Cap_words);
      _bits = 0;
    }
}

PRIVATE inline NEEDS["atomic.h"]
bool
Cap_space::cap_insert (Address bit)
{
  Address index = bit / Cap_word_bits;
  Mword mask = 1UL << (bit % Cap_word_bits);

  Mword old;

  do { old = _bits[index]; }
  while (! cas (&_bits[index], old, old | mask));

  return old & mask;
}

PRIVATE inline NEEDS["atomic.h"]
bool
Cap_space::cap_delete (Address bit)
{
  Address index = bit / Cap_word_bits;
  Mword mask = 1UL << (bit % Cap_word_bits);

  Mword old;

  do { old = _bits[index]; }
  while (! cas (&_bits[index], old, old & ~mask));

  return old & mask;
}

PUBLIC inline NEEDS["atomic.h"]
bool
Cap_space::cap_lookup (Address bit)
{
  Address index = bit / Cap_word_bits;
  Mword mask = 1UL << (bit % Cap_word_bits);

  return _full || (_bits && (_bits[index] & mask));
}

// 
// Utilities for map<Cap_space> and unmap<Cap_space>
// 

PUBLIC inline NEEDS["config.h"]
bool 
Cap_space::v_fabricate (unsigned task_id, Address /*address*/, 
			Address* phys, Address* size, unsigned* attribs = 0)
{
  if (task_id == Config::sigma0_taskno)
    {
      if (phys) *phys = 0;
      if (size) *size = Map_max_address;
      if (attribs) *attribs = 0;
      return true;
    }

  return false;
}

PUBLIC inline NEEDS[Cap_space::cap_lookup]
bool 
Cap_space::v_lookup (Address virt, Address *phys = 0, Address *size = 0,
		     unsigned *attribs = 0)
{
  if (_bits)
    {
      assert (!_full);

      if (size) *size = 1;
      if (phys) *phys = virt;
	  
      if (cap_lookup (virt))
	{
	  if (attribs) *attribs = 0;
	  return true;
	}
      return false;
    }

  if (phys) *phys = 0;
  if (size) *size = Map_max_address;
  
  if (_full)
    {
      if (attribs) *attribs = 0;
      return true;
    }

  return false;
}

PUBLIC inline NEEDS[<cassert>, Cap_space::cap_delete]
unsigned long
Cap_space::v_delete (Address virt, unsigned long size, 
		     unsigned long /*page_attribs*/ = 0)
{
  if (! _bits)
    {
      // No individual rights found, and never deleting from universal
      // right (_full == 1).
      return 0;
    }

  (void)size;
  assert (size == 1);

  if (cap_delete (virt))
    return Page_all_attribs;			
  else
    return 0;
}

PUBLIC inline NEEDS[Cap_space::cap_insert, Cap_space::bits_alloc]
Cap_space::Status
Cap_space::v_insert (Address phys, Address virt, size_t size, 
		     unsigned /*page_attribs*/)
{
  if (virt == 0 && size == Map_max_address) // Superpage map
    {
      if (_bits)
	return Insert_err_exists; // Subpages exist
      if (_full)
	return Insert_warn_exists;
      _full = 1;
      return Insert_ok;
    }

  if (! _bits && ! bits_alloc())
    return Insert_err_nomem;

  (void)phys;
  assert (phys == virt);
  assert (size == 1);

  if (cap_insert (virt))
    return Insert_warn_exists;
  else
    return Insert_ok;
}

PUBLIC inline
bool
Cap_space::is_mappable (Address virt, size_t size) const
{
  if (_full)
    {
      // Once we have a superpage, subpages cannot be mapped.
      return virt == 0 && size == Map_max_address;
    }

  return true;
}

PUBLIC inline static
void
Cap_space::tlb_flush ()
{}

PUBLIC inline static
bool 
Cap_space::need_tlb_flush ()
{ return false; }

