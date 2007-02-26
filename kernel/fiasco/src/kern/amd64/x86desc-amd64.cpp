
INTERFACE:

#include "l4_types.h"

class X86desc
{
public:
  enum
    {
      Accessed            = 0x01,
      Access_kernel       = 0x00,
      Access_user         = 0x60,
      Access_present      = 0x80,

      Access_tss          = 0x09,
      Access_intr_gate    = 0x0e,
      Access_trap_gate    = 0x0f,

      Long_mode	  	  = 0x02, // XXX for code segments
    };

protected:
  // interrupt gate / trap gate
  struct Idt_entry_desc
    {
      Unsigned16   _offset_low;
      Unsigned16   _selector;
      Unsigned8    _zero_and_ist;
      Unsigned8    _access;
      Unsigned16   _offset_high;
    } __attribute__((packed));

  // segment descriptor
  struct Gdt_entry_desc
    {
      Unsigned16   _limit_low;
      Unsigned16   _base_low;
      Unsigned8    _base_med;
      Unsigned8    _access;
      Unsigned8    _limit_high;
      Unsigned8    _base_high;
    } __attribute__((packed));

  struct Task_gate_desc
    {
      Unsigned16   _avail1;
      Unsigned16   _selector;
      Unsigned8    _avail2;
      Unsigned8    _access;
      Unsigned16   _avail3;
    } __attribute__((packed));

  // segment descriptor not present
  struct Not_present_desc
    {
      Unsigned32   _avail1;
      Unsigned8    _avail2;
      Unsigned8    _access;
      Unsigned16   _avail3;
    } __attribute__((packed));

  union
    {
      Idt_entry_desc   i;
      Gdt_entry_desc   g;
      Task_gate_desc   t;
      Not_present_desc n;
      Unsigned64       r;
    } _data;
};

INTERFACE [amd64] :

// XXX new class for 64 Bit mode descriptors
class X86desc_64 
{
public:
  enum
    {
      Accessed            = 0x01,
      Access_kernel       = 0x00,
      Access_user         = 0x60,
      Access_present      = 0x80,

      Access_intr_gate    = 0x0e,
      Access_trap_gate    = 0x0f,
    };

protected:
  // interrupt gate / trap gate
  struct Idt_entry_desc
    {
      Unsigned16   _offset_low_0;
      Unsigned16   _selector;
      Unsigned8    _zero_and_ist;
      Unsigned8    _access;
      Unsigned16   _offset_low_1;
      Unsigned32   _offset_high;
      Unsigned32   _ignored;
    } __attribute__((packed));
  
  struct Unsigned128
    {
      Unsigned64 low;
      Unsigned64 high;
    } __attribute__((packed));

  union
    {
      Idt_entry_desc i;
      Unsigned128    r;
    } _data;
};

// XXX base is now 64 bit
class Pseudo_descriptor
{
  Unsigned16 _limit;
  Unsigned64 _base;
} __attribute__((packed));


IMPLEMENTATION[debug]:

#include <cstdio>
#include "jdb_symbol.h"

PUBLIC
const char*
X86desc::type_str() const
{
  static char const * const desc_type[32] =
    {
      "reserved",          "16-bit tss (avail)",
      "ldt",               "16-bit tss (busy)",
      "16-bit call gate",  "task gate",
      "16-bit int gate",   "16-bit trap gate",
      "reserved",          "32-bit tss (avail)",
      "reserved",          "32-bit tss (busy)",
      "32-bit call gate",  "reserved",
      "32-bit int gate",   "32-bit trap gate",
      "data r/o",          "data r/o acc",
      "data r/w",          "data r/w acc",
      "data r/o exp-dn",   "data r/o exp-dn",
      "data r/w exp-dn",   "data r/w exp-dn acc",
      "code x/o",          "code x/o acc", 
      "code x/r",          "code x/r acc",
      "code x/r conf",     "code x/o conf acc",
      "code x/r conf",     "code x/r conf acc"
    };

  return desc_type[_data.n._access & 0x1f];
}

PUBLIC
void
X86desc::gdt_entry_show()
{
  // segment descriptor
  Address b = gdt_entry_base();
  Address l = gdt_entry_size();
  printf("%016lx-%016lx dpl=%d %dbit %s %02X (\033[33;1m%s\033[m)\n",
	    b, b+l, (access() & 0x60) >> 5, 
	     _data.g._limit_high & 0x40 ? 32 : 16,
    	     access() & 0x10 ? "code/data" : "system   ",
	     access() & 0x1f, type_str());
}

PUBLIC
void
X86desc::idt_entry_show()
{
  Address o = idt_entry_offset();
  //const char *symbol;

  printf("%016lx  sel=%04x dpl=%d %02X (\033[33;1m%s\033[m)",
	 o, _data.i._selector, dpl(), type(), type_str());

  //if ((symbol = Jdb_symbol::match_addr_to_symbol(o, 0)))
  //  printf(" : %.*s", 26, symbol);

  putchar('\n');
}

PUBLIC
void
X86desc::task_gate_show()
{
  printf("--------  sel=%04x dpl=%d %02X (\033[33;1m%s\033[m)\n",
         _data.t._selector, dpl(), type(), type_str());
}

PUBLIC
void
X86desc::show()
{
  if (present())
    {
      if ((access() & 0x16) == 0x06)
	idt_entry_show();
      else if (type() == 0x05)
	task_gate_show();
      else
	gdt_entry_show();
    }
  else
    {
      printf("--------  dpl=%d %02X (\033[33;1m%s\033[m)\n",
	  dpl(), type(), type_str());
    }
}

PUBLIC
const char*
X86desc_64::type_str() const
{
  static char const * const desc_type[32] =
    {
      "reserved",          "reserved",
      "64-bit ldt",        "reserved",
      "reserved",          "reserved",
      "reserved",          "reserved",
      "reserved",          "64-bit tss (avail)",
      "reserved",          "64-bit tss (busy)",
      "64-bit call gate",  "reserved",
      "64-bit int gate",   "64-bit trap gate",
      "data r/o",          "data r/o acc",
      "data r/w",          "data r/w acc",
      "data r/o exp-dn",   "data r/o exp-dn",
      "data r/w exp-dn",   "data r/w exp-dn acc",
      "code x/o",          "code x/o acc", 
      "code x/r",          "code x/r acc",
      "code x/r conf",     "code x/o conf acc",
      "code x/r conf",     "code x/r conf acc"
    };

  return desc_type[_data.i._access & 0x1f];
}

PUBLIC
void
X86desc_64::gdt_entry_show()
{
  // segment descriptor
  printf("0000000000000000-ffffffffffffffff"
         " dpl=%d %s %02X (\033[33;1m%s\033[m)\n",
	     (access() & 0x60) >> 5, 
	     access() & 0x10 ? "code/data" : "system   ",
	     access() & 0x1f, type_str());
}

PUBLIC
void
X86desc_64::idt_entry_show()
{
  Address o = idt_entry_offset();
  //const char *symbol;

  printf("%016lx  sel=%04x dpl=%d %02X (\033[33;1m%s\033[m)",
	 o, _data.i._selector, dpl(), type(), type_str());

  //if ((symbol = Jdb_symbol::match_addr_to_symbol(o, 0)))
  //  printf(" : %.*s", 26, symbol);

  putchar('\n');
}

PUBLIC
void
X86desc_64::show()
{
  if (present())
    {
      if ((access() & 0x16) == 0x06)
	idt_entry_show();
      else
	gdt_entry_show();
    }
  else
    {
      printf("----------------  dpl=%d %02X (\033[33;1m%s\033[m)\n",
	  dpl(), type(), type_str());
    }
}

IMPLEMENTATION:

PUBLIC inline
Unsigned64
X86desc::get_raw()
{
  return _data.r;
}

PUBLIC inline
void
X86desc::get_raw(Unsigned32 *low, Unsigned32 *high)
{
  *low  = _data.r & 0xffffffff;
  *high = _data.r >> 32;
}

PUBLIC inline
void
X86desc::set_raw(Unsigned64 val)
{
  _data.r = val;
}

PUBLIC inline
void
X86desc::set_raw(Unsigned32 low, Unsigned32 high)
{
  _data.r = ((Unsigned64)high << 32 | low);
}

PUBLIC inline
X86desc::X86desc()
{
}

PUBLIC inline
X86desc::X86desc(Unsigned64 val)
{
  set_raw(val);
}

PUBLIC inline
Address
X86desc::gdt_entry_base()
{
  return    (Address)_data.g._base_low 
	 | ((Address)_data.g._base_med  << 16) 
	 | ((Address)_data.g._base_high << 24);
}

PUBLIC inline
Unsigned32
X86desc::gdt_entry_limit()
{
  return      (Unsigned32)_data.g._limit_low
	  | (((Unsigned32)_data.g._limit_high & 0x0f) << 16);
}

PUBLIC inline
Unsigned32
X86desc::gdt_entry_size()
{
  Address l = gdt_entry_limit();
  return _data.g._limit_high & 0x80 ? ((l+1) << 12)-1 : l;
}

PUBLIC inline
Address
X86desc::idt_entry_offset()
{
  return    (Address)_data.i._offset_low
         | ((Address)_data.i._offset_high << 16);
}

PUBLIC inline
Unsigned8
X86desc::access()
{
  return _data.n._access;
}

PUBLIC inline
int
X86desc::present()
{
  return (access() & 0x80) >> 7;
}

PUBLIC inline
Unsigned8
X86desc::type()
{
  return _data.n._access & 0x1f;
}

PUBLIC inline
Unsigned8
X86desc::dpl()
{
  return (_data.n._access & 0x60) >> 5;
}

// XXX minimal implementation
PUBLIC inline
int
X86desc_64::present()
{
  return (access() & 0x80) >> 7;
}

PUBLIC inline
Unsigned8
X86desc_64::type()
{
  return _data.i._access & 0x1f;
}

PUBLIC inline
Unsigned8
X86desc_64::dpl()
{
  return (_data.i._access & 0x60) >> 5;
}

PUBLIC inline
Address
X86desc_64::idt_entry_offset()
{
  return    (Address)_data.i._offset_low_0
         | ((Address)_data.i._offset_low_1 << 16)
	 | ((Address)_data.i._offset_high << 32);
}

PUBLIC inline
Unsigned8
X86desc_64::access()
{
  return _data.i._access;
}
// XXX end of minimal implementation
  
PUBLIC inline
bool
X86desc::unsafe()
{
  return present() && (dpl() != 3);
}

PUBLIC inline
Pseudo_descriptor::Pseudo_descriptor()
{}

PUBLIC inline
Pseudo_descriptor::Pseudo_descriptor(Address base, Unsigned16 limit)
  : _limit(limit), _base(base)
{}

PUBLIC inline
Address
Pseudo_descriptor::base() const
{
  return _base;
}

PUBLIC inline
Unsigned16
Pseudo_descriptor::limit() const
{
  return _limit;
}

