/* -*- c++ -*- */
#ifndef L4_X_MEMMAP_H__
#define L4_X_MEMMAP_H__

#include "page.h"
#include "config.h"

class Region
{
public:
  Region( unsigned long start, unsigned long size, 
          unsigned long grain, unsigned short *map )
    : _start(start), _size(size), _grain(grain), _map(map), _next(0)
  {}
  
  unsigned long start() const { return _start; }
  unsigned long size() const  { return _size;  }
  unsigned long grain() const { return _grain; }

  unsigned owner( unsigned long addr ) const 
  { return _map[(addr-_start) >> _grain]; }
  
  void owner( unsigned long addr, unsigned owner )
  { _map[(addr-_start) >> _grain] = owner; }
  unsigned is_free( unsigned long addr ) const { return owner(addr)==0; }
  void reserve( unsigned long addr )
  { owner(addr, -1UL); }


  Region *next() const { return _next; }
  void next( Region *r ) { _next = r; }

  Region *find( unsigned long addr )
  {
    Region *r = this;
    while( r && (addr < r->_start || addr >=r->_start+r->_size))
      r = r->_next;

    return r;
  }
  
private:
  unsigned long const _start;
  unsigned long const _size;
  unsigned long const _grain;
  unsigned short *_map;
  Region *_next;
};

template< unsigned long Start, unsigned long Size, unsigned long Grain >
class Region_instance : public Region
{
public:
  Region_instance() : Region(Start,Size,Grain,map)
  {
    for (unsigned i = 0; i <= Size >> Grain; ++i)
      map[i] = 0; // mark free
  }

private:
  unsigned short map[Size >> Grain];
};


class Memmap
{
public:
  enum {
    LOWER_LIMIT   = Ram_base,
    MEM_MAX       = 64UL*1024UL*1024UL,
    SUPERPAGE_MAX = 4096,
    O_MAX         = 255,
  };
  
  Memmap() : regions(0) {}

  void add_region( Region *r ) { r->next(regions); regions = r; }

  bool free(l4_addr_t address, char owner = 1);
  bool free_superpage(l4_addr_t address, char owner = 1);
  bool alloc(l4_addr_t address, char owner, unsigned &grain);
  char alloc_r(unsigned long addr, char owner, unsigned &grain);
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
  void reserve(l4_addr_t address) { unsigned gr; alloc(address, 1, gr); }

  void find_free(l4_umword_t *d1, l4_umword_t *d2, char owner);

private:
  Page *atop(l4_addr_t address);
  SuperPage *atos(l4_addr_t address);
  Page const *atop(l4_addr_t address) const;
  SuperPage const *atos(l4_addr_t address) const;

  Page memmap[MEM_MAX/L4_PAGESIZE];
  SuperPage memmapsp[SUPERPAGE_MAX];

  Region *regions;

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
