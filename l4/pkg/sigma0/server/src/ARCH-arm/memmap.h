/* -*- c++ -*- */
#ifndef L4_X_MEMMAP_H__
#define L4_X_MEMMAP_H__

#include "page.h"


class Memmap
{
public:
  enum {
    LOWER_LIMIT   = 0xc0000000,
    MEM_MAX       = 64UL*1024UL*1024UL,
    SUPERPAGE_MAX = 4096,
    O_MAX         = 255,
  };
  
  Memmap() {}

  bool free(l4_addr_t address, char owner = 1);
  bool free_superpage(l4_addr_t address, char owner = 1);
  bool alloc(l4_addr_t address, char owner);
  bool alloc_superpage(l4_addr_t address, char owner);
  l4_uint16_t nr_free(l4_addr_t address) const;
  char owner(l4_addr_t address) const;
  char owner_superpage(l4_addr_t address) const;
  bool is_free(l4_addr_t address) const 
  { 
    char o = owner_superpage(address);
    if(o == 1)
      return owner(address);
    if(o == 0)
      return true;
    return false;
  }

  bool is_reserved(l4_addr_t address) const { return owner(address) == 1; }
  void reserve(l4_addr_t address) { alloc(address, 1); }

  void find_free(l4_umword_t *d1, l4_umword_t *d2, char owner);

private:
  Page *atop(l4_addr_t address);
  SuperPage *atos(l4_addr_t address);
  Page const *atop(l4_addr_t address) const;
  SuperPage const *atos(l4_addr_t address) const;

  Page memmap[MEM_MAX/L4_PAGESIZE];
  SuperPage memmapsp[SUPERPAGE_MAX];

public:
  l4_addr_t mem_high;
};

inline Page *Memmap::atop(l4_addr_t address)
{
  if(address<LOWER_LIMIT)
    return 0;
  
  address -= LOWER_LIMIT;
  if(address >= MEM_MAX)
    return 0;

  return memmap + address/L4_PAGESIZE;
}

inline SuperPage *Memmap::atos(l4_addr_t address)
{
  if(address<LOWER_LIMIT)
    return 0;
  
  address -= LOWER_LIMIT;
  if(address >= MEM_MAX)
    return 0;

  return memmapsp + address/L4_SUPERPAGESIZE;
}

inline Page const *Memmap::atop(l4_addr_t address) const
{
  if(address<LOWER_LIMIT)
    return 0;
  
  address -= LOWER_LIMIT;
  if(address >= MEM_MAX)
    return 0;

  return memmap + address/L4_PAGESIZE;
}

inline SuperPage const *Memmap::atos(l4_addr_t address) const
{
  if(address<LOWER_LIMIT)
    return 0;
  
  address -= LOWER_LIMIT;
  if(address >= MEM_MAX)
    return 0;

  return memmapsp + address/L4_SUPERPAGESIZE;
}

#endif /* L4_X_MEMMAP_H__ */
