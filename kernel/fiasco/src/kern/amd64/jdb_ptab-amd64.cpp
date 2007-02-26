IMPLEMENTATION [amd64]:

#include "paging.h"

unsigned Jdb_ptab::max_pt_level = 3;

IMPLEMENT inline NEEDS ["paging.h"]
unsigned 
Jdb_ptab::entry_valid(Mword entry, unsigned)
{ 
  return entry & Pt_entry::Valid; 
}

IMPLEMENT inline NEEDS ["paging.h"]
unsigned 
Jdb_ptab::entry_is_pt_ptr(Mword entry, unsigned level,
                          unsigned *entries, unsigned *next_level)
{
  if (level > (max_pt_level - 1) || entry & Pd_entry::Superpage)
    return 0;
  *entries = 512;		// XXX ugly
  *next_level = level+1;
  return 1;
}

IMPLEMENT inline NEEDS ["paging.h"]
Address 
Jdb_ptab::entry_phys(Mword entry, unsigned level)
{
  if (level == 0)
    return entry & Pml4_entry::Pdpfn;
  else if (level == 1)
    return entry & Pdp_entry::Pdirfn;
  else if (level == 2)
    return entry & Pd_entry::Ptabfn;
  else
    return entry & Pt_entry::Pfn; 
}


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
      putstr("        -       ");
      return;
    }

  Address phys = entry_phys(entry, level);

  if (level == 2 && entry & Pd_entry::Superpage)
    printf((phys >> 20) > 0xFF
	   ? "    %03lx/2--" : "     %02lx/2--", phys >> 20);
  else
    printf((phys >> Config::PAGE_SHIFT) > 0xFFF
           ? "      %05lx" : "       %04lx", phys >> Config::PAGE_SHIFT);

  putchar(((cur_pt_level>=max_pt_level || (entry & Pd_entry::Superpage)) &&
	 (entry & Pd_entry::Cpu_global)) ? '+' : '-');
  putchar(entry & Pd_entry::Noncacheable
	    ? 'n' : (entry & Pd_entry::Write_through) ? 't' : '-');
  putchar(entry & Pd_entry::User
	    ? (entry & Pd_entry::Writable) ? 'w' : 'r'
	    : (entry & Pd_entry::Writable) ? 'W' : 'R');
  putstr("  ");
}

IMPLEMENT
void
Jdb_ptab::print_head(Mword entry)
{
  printf("%08x", (Unsigned32)(entry & 0xffffffff));
}

IMPLEMENT inline
unsigned 
Jdb_ptab::first_level_entries()
{ 
  return 512; // XXX ugly
}

PUBLIC
unsigned 
Jdb_ptab::rows() const
{ 
  return entries/4;
}

// calculate row from virtual address
PRIVATE
unsigned 
Jdb_ptab::disp_virt_to_r(Address v)
{
  v = Paging::decanonize(v);
  if (cur_pt_level == 0)
    v >>= Config::PML4E_SHIFT;
  else if (cur_pt_level == 1)
    v >>= Config::PDPE_SHIFT;
  else if (cur_pt_level == 2)
    v >>= Config::SUPERPAGE_SHIFT;
  else
    v >>= Config::PAGE_SHIFT;

  return v / (cols()-1);
}

// calculate column from virtual address
PRIVATE
unsigned 
Jdb_ptab::disp_virt_to_c(Address v)
{
  v = Paging::decanonize(v);
  if (cur_pt_level == 0)
    v >>= Config::PML4E_SHIFT;
  else if (cur_pt_level == 1)
    v >>= Config::PDPE_SHIFT;
  else if (cur_pt_level == 2)
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
  Address v;

  if (cur_pt_level == 0)
    v = e * Config::PML4_SIZE;
  else if (cur_pt_level == 1)
    v = e * Config::PDP_SIZE + virt_base;
  else if (cur_pt_level == 2)
    v = e * Config::SUPERPAGE_SIZE + virt_base;
  else
    v = e * Config::PAGE_SIZE + virt_base;
  return Paging::canonize(v);
}

PUBLIC
void
Jdb_ptab::print_statline(unsigned row, unsigned col)
{
  if (cur_pt_level == 0)
    {
      Jdb::printf_statline("pml4", "<Space>=mode <CR>=goto pdp",
	  "<"L4_PTR_FMT"> task %-3x", disp_virt(row,col), task);
    }
  else if (cur_pt_level == 1)
    {
      Jdb::printf_statline("pdp", "<Space>=mode <CR>=goto pdir",
	  "<"L4_PTR_FMT"> task %-3x", disp_virt(row,col), task);
    }
  else if (cur_pt_level == 2)
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

IMPLEMENT inline
Address
Jdb_ptab_m::get_base(Space *s)
{
  return ((Address)(s->mem_space()->dir()));
}

