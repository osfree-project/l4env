IMPLEMENTATION [ia32,ux]:

#include "paging.h"

unsigned Jdb_ptab::max_pt_level = 1;

IMPLEMENT inline NEEDS ["paging.h"]
unsigned 
Jdb_ptab::entry_valid(Mword entry, unsigned)
{ return entry & Pt_entry::Valid; }

IMPLEMENT inline NEEDS ["paging.h"]
unsigned 
Jdb_ptab::entry_is_pt_ptr(Mword entry, unsigned level,
                          unsigned *entries, unsigned *next_level)
{
  if (level > 0 || entry & Pd_entry::Superpage)
    return 0;
  *entries = 1024;
  *next_level = 1;
  return 1;
}

IMPLEMENT inline NEEDS ["paging.h"]
Address 
Jdb_ptab::entry_phys(Mword entry, unsigned)
{ return entry & Pt_entry::Pfn; }


IMPLEMENT
void 
Jdb_ptab::print_entry(Mword entry, unsigned level)
{
  if (dump_raw)
    {
      printf(L4_PTR_FMT, entry);
      return;
    }

  if (!entry_valid(entry,level))
    {
      putstr("    -   ");
      return;
    }

  Address phys = entry_phys(entry, level);

  if (level == 0 && entry & Pd_entry::Superpage)
    printf((phys >> 20) > 0xFF
	    ? "%03lX/4" : " %02lX/4", phys >> 20);
  else
    printf((phys >> Config::PAGE_SHIFT) > 0xFFFF
	    ? "%05lx" : " %04lx", phys >> Config::PAGE_SHIFT);

  putchar(((cur_pt_level>=max_pt_level || (entry & Pd_entry::Superpage)) &&
	 (entry & Pd_entry::Cpu_global)) ? '+' : '-');
  putchar(entry & Pd_entry::Noncacheable
	    ? 'n' : (entry & Pd_entry::Write_through) ? 't' : '-');
  putchar(entry & Pd_entry::User
	    ? (entry & Pd_entry::Writable) ? 'w' : 'r'
	    : (entry & Pd_entry::Writable) ? 'W' : 'R');
}

IMPLEMENT inline
unsigned 
Jdb_ptab::first_level_entries()
{ return 1024; }

PUBLIC
unsigned 
Jdb_ptab::rows() const
{ return entries/8; }

IMPLEMENTATION [ia32,ux,arm]:

PRIVATE
unsigned 
Jdb_ptab::disp_virt_to_r(Address v)
{
  if (cur_pt_level == 0)
    v >>= Config::SUPERPAGE_SHIFT;
  else
    v >>= Config::PAGE_SHIFT;

  return v / (cols()-1);
}

PRIVATE
unsigned 
Jdb_ptab::disp_virt_to_c(Address v)
{
  if (cur_pt_level == 0)
    v >>= Config::SUPERPAGE_SHIFT;
  else
    v >>= Config::PAGE_SHIFT;

  return (v % (cols()-1)) + 1;
}

PRIVATE
Address 
Jdb_ptab::disp_virt(unsigned row, unsigned col)
{
  Mword e = (col-1) + (row * (cols()-1));
  if (cur_pt_level == 0)
    return e * Config::SUPERPAGE_SIZE;
  else
    return e * Config::PAGE_SIZE + virt_base;
}

PUBLIC
void
Jdb_ptab::print_statline(unsigned row, unsigned col)
{
  if (cur_pt_level<max_pt_level)
    {
      Jdb::printf_statline("pdir", "<Space>=mode <CR>=goto ptab/superpage",
	  "<"L4_PTR_FMT"> task %-3x", disp_virt(row,col), task);
    }
  else // PT_MODE
    {
      Jdb::printf_statline("ptab", "<Space>=mode <CR>=goto page",
	  "<"L4_PTR_FMT"> task %-3x", disp_virt(row,col), task);
    }
}

IMPLEMENT
void
Jdb_ptab::print_head(Mword entry)
{
  printf(L4_PTR_FMT, entry);
  return;
}

IMPLEMENT inline
Address
Jdb_ptab_m::get_base(Space *s)
{
  return ((Address)(s->mem_space()->dir()));
}
