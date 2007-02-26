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

  if (phys > 0x40000000)
    {
      if (level == 0 && entry & Pd_entry::Superpage)
	printf("%03lX/4", phys >> (Config::SUPERPAGE_SHIFT-2));
      else
	printf("%05lX", phys >> Config::PAGE_SHIFT);
    }
  else
    {
      if (level == 0 && entry & Pd_entry::Superpage)
	printf(" %02lx/4", phys >> Config::SUPERPAGE_SHIFT);
      else
	printf(phys < 0x10000000 ? " %04lx" : "%05lx", 
	       phys >> Config::PAGE_SHIFT);
    }

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
