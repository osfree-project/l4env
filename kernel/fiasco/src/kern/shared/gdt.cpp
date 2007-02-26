
INTERFACE:

#include "config_gdt.h"
#include "l4_types.h"
#include "x86desc.h"

class Gdt_entry : public X86desc
{
public:
  enum
    {
      Access_type_user    = 0x10,
      Access_code_read    = 0x1a,
      Access_data_write   = 0x12,
      Size_32             = 0x04,
    };
};


class Gdt
{
public:
   /** Segment numbers. */
  enum 
    {
      gdt_tss             = GDT_TSS,
      gdt_code_kernel     = GDT_CODE_KERNEL,
      gdt_data_kernel     = GDT_DATA_KERNEL,
      gdt_code_user       = GDT_CODE_USER,
      gdt_data_user       = GDT_DATA_USER,
      gdt_tss_dbf         = GDT_TSS_DBF,
      gdt_utcb            = GDT_UTCB,
      gdt_ldt             = GDT_LDT,
      gdt_tls1            = GDT_TLS1,
      gdt_tls2            = GDT_TLS2,
      gdt_tls3            = GDT_TLS3,
      gdt_max             = GDT_MAX,
    };

  enum
    {
      Selector_user       = 0x03,
      Selector_kernel     = 0x00,
    };

private:
  Gdt_entry _entries[];
};


IMPLEMENTATION:

PUBLIC inline
Gdt_entry::Gdt_entry(Address base, Unsigned32 limit, 
		     Unsigned8 access, Unsigned8 szbits)
{
  _data.g._limit_low  =  limit & 0x0000ffff;
  _data.g._base_low   =  base  & 0x0000ffff;
  _data.g._base_med   = (base  & 0x00ff0000) >> 16;
  _data.g._access     =  access | Access_present;
  _data.g._limit_high = ((limit & 0x000f0000) >> 16) | 
			(((Unsigned16)szbits) << 4);
  _data.g._base_high  = (base  & 0xff000000) >> 24;
}

PUBLIC inline
Gdt_entry::Gdt_entry(Unsigned64 raw)
  : X86desc(raw)
{}

PUBLIC inline
Address
Gdt_entry::base()
{
  return gdt_entry_base();
}

PUBLIC inline
size_t
Gdt_entry::limit()
{
  return gdt_entry_limit();
}

PUBLIC inline
size_t
Gdt_entry::size()
{
  return gdt_entry_size();
}

PUBLIC inline
int
Gdt_entry::avl()
{
  return (_data.g._limit_high & 0x10) >> 4;
}

PUBLIC inline
int
Gdt_entry::seg32()
{
  return (_data.g._limit_high & 0x40) >> 6;
}

PUBLIC inline
int
Gdt_entry::granularity()
{
  return (_data.g._limit_high & 0x80) >> 7;
}

PUBLIC inline
int
Gdt_entry::writable()
{
  return (type() & 0x02) >> 1;
}

PUBLIC inline
int
Gdt_entry::contents()
{
  return (type() & 0x0c) >> 2;
}

PUBLIC inline
void
Gdt::set_entry_byte(int nr, Address base, Unsigned32 limit,
		    Unsigned8 access, Unsigned8 szbits)
{
  _entries[nr] = Gdt_entry(base, limit, access, szbits);
}

PUBLIC inline
void
Gdt::set_entry_4k(int nr, Address base, Unsigned32 limit,
		  Unsigned8 access, Unsigned8 szbits)
{
  _entries[nr] = Gdt_entry(base, limit >> 12, access, szbits | 0x08);
}

PUBLIC inline
void
Gdt::set_raw(int nr, Unsigned32 low, Unsigned32 high)
{
  _entries[nr].set_raw(low, high);
}

PUBLIC inline
void
Gdt::set_raw(int nr, Unsigned64 val)
{
  _entries[nr].set_raw(val);
}

PUBLIC inline
void
Gdt::get_raw(int nr, Unsigned32 *low, Unsigned32 *high)
{
  _entries[nr].get_raw(low, high);
}

PUBLIC inline
void
Gdt::clear_entry(int nr)
{
  set_raw(nr, 0, 0);
}

PUBLIC inline
Gdt_entry*
Gdt::entries()
{
  return _entries;
}


IMPLEMENTATION[ia32]:

PUBLIC static inline
void
Gdt::set (Pseudo_descriptor *desc)
{
  asm volatile ("lgdt %0" : : "m" (*desc));
}

PUBLIC static inline
void
Gdt::get (Pseudo_descriptor *desc)
{
  asm volatile ("sgdt %0" : "=m" (*desc) : : "memory");
}


IMPLEMENTATION[!smas]:

PUBLIC static inline
int
Gdt::data_segment (bool /* kernel_mode */)
{
  return gdt_data_user | Selector_user;
}


IMPLEMENTATION[smas]:

PUBLIC static inline
int
Gdt::data_segment (bool kernel_mode)
{
  return (kernel_mode) ? gdt_data_kernel | Selector_kernel
		       : gdt_data_user   | Selector_user;
}

