INTERFACE [arm-x0]:

class Mem_desc
{
public:
  enum Mem_type
  {
    Undefined    = 0x0,
    Conventional = 0x1,
    Reserved     = 0x2,
    Dedicated    = 0x3,
    Shared       = 0x4,

    Bootloader   = 0xe,
    Arch         = 0xf,
  };

private:
  Mword _l, _h;
};

//----------------------------------------------------------------------------
INTERFACE:

#include "types.h"

class Kip
{
public:
  void print() const;

  char const *version_string() const;

  // returns the 1st address beyond all available physical memory
  Address main_memory_high() const;
  
private:
  static Kip *global_kip asm ("GLOBAL_KIP");
};

#define L4_KERNEL_INFO_MAGIC (0x4BE6344CL) /* "L4�K" */

//============================================================================
IMPLEMENTATION [arm-x0]:

#include <cassert>

PUBLIC inline
Mem_desc::Mem_desc(Address start, Address end, Mem_type t, bool v = false, 
                   unsigned st = 0)
: _l((start & ~0x3ffUL) | (t & 0x0f) | ((st << 4) & 0x0f0) 
     | (v?0x0200:0x0)),
  _h(end)
{}

PUBLIC inline
Address Mem_desc::start() const
{ return _l & ~0x3ffUL; }

PUBLIC inline
Address Mem_desc::end() const
{ return _h | 0x3ffUL; }

PUBLIC inline
void
Mem_desc::type(Mem_type t)
{ _l = (_l & ~0x0f) | (t & 0x0f); }

PUBLIC inline
Mem_desc::Mem_type Mem_desc::type() const
{ return (Mem_type)(_l & 0x0f); }

PUBLIC inline
unsigned Mem_desc::ext_type() const
{ return _l & 0x0f0; }

PUBLIC inline
unsigned Mem_desc::is_virtual() const
{ return _l & 0x200; }

PUBLIC inline
Mem_desc *Kip::mem_descs()
{ return (Mem_desc*)(((Address)this) + (_mem_info >> (MWORD_BITS/2))); }

PUBLIC inline
Mem_desc const *Kip::mem_descs() const
{ return (Mem_desc const *)(((Address)this) + (_mem_info >> (MWORD_BITS/2))); }

PUBLIC inline
unsigned Kip::num_mem_descs() const
{ return _mem_info & ((1 << (MWORD_BITS/2))-1); }

PUBLIC inline
void Kip::num_mem_descs (unsigned n)
{
  _mem_info = (_mem_info & ~((1 << (MWORD_BITS/2))-1)
	       | (n & ((1 << (MWORD_BITS/2))-1)));
}

PUBLIC inline
void
Kip::init_mem_info( unsigned offset, unsigned n)
{
  unsigned long ofs = offset; 
  //(unsigned long)&(((Kip*)10)->_mem) - 10;
  _mem_info = (ofs << (MWORD_BITS/2)) | (n & ((1 << (MWORD_BITS/2)) -1));
  Mem_desc *m = mem_descs();
  Mem_desc *const end = m + n;
  for (;m<end;++m)
    m->type(Mem_desc::Undefined);
}

PUBLIC inline
void
Kip::finish_mem_info()
{
  Mem_desc const *m = mem_descs();
  Mem_desc const *const end = m + num_mem_descs();
  for (unsigned i=0; m<end; m++, i++)
    {
      if (m->type() == Mem_desc::Undefined)
	{
	  num_mem_descs (i);
	  break;
	}
    }
}

PUBLIC
Mem_region 
Kip::max_mem_region(Mem_desc::Mem_type t) const
{
  Mem_region r;
  r.start = -1UL;
  r.end   = 0;
  Mem_desc const *m = mem_descs();
  Mem_desc const *const end = m + num_mem_descs();
  for (;m<end;m++)
    {
      if ((!m->is_virtual()) && (m->type()==t))
	{
	  Address s = m->start();
	  Address e = m->end();
	  if (s<r.start)
	    r.start = s;
	  if (e>r.end)
	    r.end = e;
	}
    }
  return r;
}

PUBLIC
bool Kip::add_mem_region(Mem_desc const &md)
{
  Mem_desc *m = mem_descs();
  Mem_desc *end = m + num_mem_descs();
  for (;m<end;++m)
    {
      if (m->type() == Mem_desc::Undefined)
	{
	  *m = md;
	  return true;
	}
    }

  // Add mem region failed -- must be a Fiasco startup problem.  Bail out.
  assert (0);

  return false;
}

PUBLIC 
Mem_region Kip::last_free() const
{
  Mem_desc const *m = mem_descs();
  Mem_desc const *const end = m + num_mem_descs();
  Mem_region r;
  r.start = 0;
  r.end   = 0;
  for (;m<end;++m)
    {
      Address s = m->start();
      Address e = m->end();
      
      // Speep out stupid descriptors (that have the end before the start)
      if (s>=e)
	{
  	  const_cast<Mem_desc*>(m)->type(Mem_desc::Undefined);
	  continue;
	}
      
      if (m->is_virtual())	// skip virtual memory descriptors
	continue;
	    
      switch (m->type())
	{
	case Mem_desc::Conventional:
	  if (e>r.end)
	    r.end = e;
	  if (s>r.start)
	    r.start = s;
	  break;
	case Mem_desc::Reserved: 
	case Mem_desc::Dedicated:
	case Mem_desc::Shared:
	case Mem_desc::Arch:
	case Mem_desc::Bootloader:
	  if (e>=r.start && e<r.end)
	    {
	      r.start = e + 1;
	    }
	  if (s>=r.start && s<r.end)
	    {
	      r.end = s - 1;
	    }
	  break;
	default:
	  break;
	}
    }
  return r;
}

//----------------------------------------------------------------------------
IMPLEMENTATION:

#include "config.h"
#include "version.h"

Kip *Kip::global_kip;
PUBLIC static inline void Kip::init_global_kip (Kip *kip) { global_kip = kip; }
PUBLIC static inline Kip *const Kip::k() { return global_kip; }

asm(".section .initkip.version, \"a\", %progbits        \n"	\
    ".string \"" CONFIG_KERNEL_VERSION_STRING "\"       \n"	\
    ".previous                                          \n");

asm(".section .initkip.features.fini, \"a\", %progbits  \n"	\
    ".string \"\"                                       \n"	\
    ".previous                                          \n");
