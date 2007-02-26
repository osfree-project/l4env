IMPLEMENTATION [arm]:

#include "paging.h"
#include "simpleio.h"

unsigned Jdb_ptab::max_pt_level = 1;

IMPLEMENT inline NEEDS ["paging.h"]
unsigned 
Jdb_ptab::entry_valid(Mword entry, unsigned)
{ return entry &  0x03; }

IMPLEMENT inline NEEDS ["paging.h"]
unsigned
Jdb_ptab::entry_is_pt_ptr(Mword entry, unsigned level,
                          unsigned *entries, unsigned *next_level)
{
  if (level > 0 || (entry & 0x03) == 0x02)
    return 0;

  if ((entry & 0x03) == 0x01)
    *entries = 256;  // coarse (1KB)
  else
    *entries = 1024; // fine (4KB)
      
  *next_level = 1;
  return 1;
}

IMPLEMENT inline NEEDS ["paging.h"]
Address 
Jdb_ptab::entry_phys(Mword entry, unsigned level)
{ 
  unsigned t = entry & 0x03;
  if (level == 0)
    {
      switch(t)
	{
	default:
	case 0:
	  return 0;
	case 1:
	  return entry & 0xfffffc00;
	case 2:
	  return entry & 0xfff00000;
	case 3:
	  return entry & 0xfffff000;
	}
    }
  else
    {
      switch (t)
	{
	default:
	case 0:
	  return 0;
	case 1:
	  return entry & 0xffff0000;
	case 2:
	  return entry & 0xfffff000;
	case 3:
	  return entry & 0xfffffc00;
	}
    }
}

IMPLEMENT
void 
Jdb_ptab::print_entry(Mword entry, unsigned level)
{
  if (dump_raw)
    printf("%08lx", entry);
  else
    {
      if (!entry_valid(entry,level))
	{
	  putstr("    -   ");
	  return;
	}
      Address phys = entry_phys(entry, level);

      unsigned t = entry & 0x03;
      unsigned ap = entry >> 4;
      char ps;
      if (level == 0)
	switch (t)
	  {
	  case 1: ps = 'C'; break;
	  case 2: ps = 'S'; ap = entry >> 10; break;
	  case 3: ps = 'F'; break;
	  default: ps = 'U'; break;
	  }
      else
	switch (t)
	  {
	  case 1: ps = 'l'; break;
	  case 2: ps = 's'; break;
	  case 3: ps = 't'; break;
	  default: ps = 'u'; break;
	  }

      printf("%05lx-%c", phys >> Config::PAGE_SHIFT, ps);
      if (level == 0 && (t == 1 || t == 3))
        putchar('-');
      else
	putchar(ap & 0x2
		? (ap & 0x1) ? 'w' : 'r'
		: (ap & 0x1) ? 'W' : 'R');
    }
}

IMPLEMENT
unsigned 
Jdb_ptab::first_level_entries()
{ return 4096; }

PUBLIC
unsigned 
Jdb_ptab::rows() const
{ return entries/8; }
