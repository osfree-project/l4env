/**
 * \file	roottask/server/src/memmap.c
 * \brief	memory resource handling
 *
 * \date	05/10/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author      Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 * Before allocating memory we check the quota of the task.
 *
 **/
#include <stdio.h>
#include <string.h>

#include "l4/rmgr/proto.h"

#include "memmap.h"
#include "quota.h"
#include "region.h"
#include "rmgr.h"
#include "version.h"

typedef struct
{
  l4_uint16_t free_pages;
  owner_t     owner;
  l4_uint8_t  padding;
} __attribute__((packed)) __superpage_t;

extern owner_t __memmap[];
__superpage_t  __memmap4mb[SUPERPAGE_MAX];

l4_addr_t mem_high;

void
memmap_init(void)
{
  unsigned i;
  __superpage_t *s;

  memset(__memmap, O_RESERVED, PAGE_MAX);
  for (s = __memmap4mb, i = 0; i < SUPERPAGE_MAX; i++, s++)
    *s = (__superpage_t) { 0, O_RESERVED, 0 };
}

void
memmap_set(unsigned start, int state, unsigned size)
{
  memset(__memmap + start, state, size);
}

void
memmap_set_range(l4_addr_t start, l4_addr_t end, owner_t owner)
{
  memmap_set((start - ram_base)/L4_PAGESIZE, owner,
             l4_round_page(end-l4_trunc_page(start))/L4_PAGESIZE);
}

int
memmap_set_page(l4_addr_t address, owner_t owner)
{
  owner_t *m       = __memmap    + (address - ram_base)/L4_PAGESIZE;
  __superpage_t *s = __memmap4mb + (address - ram_base)/L4_SUPERPAGESIZE;

  if (s >= __memmap4mb + SUPERPAGE_MAX)
    return 0;			/* address exceeds __memmap4mb array */
  if (address-ram_base >= mem_high)
    return 0;			/* address exceeds __memmap array */
  if (*m == owner)
    return 1;			/* already allocated */
  if (s->owner == O_FREE)
    s->owner = O_RESERVED;
  else if (s->owner != O_RESERVED)
    return 0;

  s->free_pages--;
  *m = owner;
  return 1;
}

int
memmap_free_page(l4_addr_t address, owner_t owner)
{
  owner_t *m       = __memmap    + (address - ram_base)/L4_PAGESIZE;
  __superpage_t *s = __memmap4mb + (address - ram_base)/L4_SUPERPAGESIZE;

  if (s >= __memmap4mb + SUPERPAGE_MAX)
    return 0;			/* address exceeds __memmap4mb array */
  if (s->owner == O_FREE)
    return 1;			/* belonging superpage already free */
  if (s->owner != O_RESERVED)
    return 0;			/* belonging superpage owned by someone */
  if (address-ram_base >= mem_high)
    return 0;			/* address exceeds __memmap array */
  if (*m == O_FREE)
    return 1;			/* page already free */
  if (*m != owner)
    return 0;			/* page not owned by owner */

  if (owner >= O_USER)
    quota_free_mem(owner, address, L4_PAGESIZE);

  *m = O_FREE;
  s->free_pages++;

  if (s->free_pages == L4_SUPERPAGESIZE/L4_PAGESIZE)
    s->owner = O_FREE;
  else if (s->free_pages == 1)
    s->owner = O_RESERVED;
  return 1;
}

int
memmap_alloc_page(l4_addr_t address, owner_t owner)
{
  owner_t *m       = __memmap    + (address - ram_base)/L4_PAGESIZE;
  __superpage_t *s = __memmap4mb + (address - ram_base)/L4_SUPERPAGESIZE;

  if (s >= __memmap4mb + SUPERPAGE_MAX)
    return 0;			/* address exceeds __memmap4mb array */
  if (address-ram_base >= mem_high)
    return 0;			/* address exceedxs _memmap array */
  if (*m == owner || s->owner == owner)
    return 1;			/* already allocated */
  if (*m != O_FREE)
    return 0;

  if (s->owner == O_FREE)
    s->owner = O_RESERVED;
  else if (s->owner != O_RESERVED)
    return 0;

  if (owner >= O_USER)
    if (! quota_alloc_mem(owner, address, L4_PAGESIZE))
      return 0;

  s->free_pages--;
  *m = owner;

  if (s->free_pages == 0)
    {
      /* no free pages in superpage -- check if the whole page belongs to
       * owner */
      owner_t *sm = __memmap +
	             (l4_trunc_superpage(address) - ram_base)/L4_PAGESIZE;

      for (m=sm; m<sm+L4_SUPERPAGESIZE/L4_PAGESIZE; m++)
	{
	  if (*m != owner)
	    /* at least one page doesn't belong to owner */
	    return 1;
	}

      /* whole superpage belongs to owner */
      s->owner = owner;
    }

  return 1;
}

owner_t
memmap_owner_page(l4_addr_t address)
{
  owner_t *m       = __memmap    + (address - ram_base)/L4_PAGESIZE;
  __superpage_t *s = __memmap4mb + (address - ram_base)/L4_SUPERPAGESIZE;

  if (s >= __memmap4mb + SUPERPAGE_MAX)
    return O_RESERVED;		/* address exceeds __memmap4mb array */
  if (address-ram_base >= mem_high)
    return O_RESERVED;		/* address exceeds __memmap array */

  return s->owner == O_RESERVED ? *m : s->owner;
}

int
memmap_free_superpage(l4_addr_t address, owner_t owner)
{
  __superpage_t *s = __memmap4mb + (address - ram_base)/L4_SUPERPAGESIZE;

  if (s >= __memmap4mb + SUPERPAGE_MAX)
    return 0;			/* address exceeds __memmap4mb array */
  if (s->owner == O_FREE)
    return 1;			/* already free */
  if (s->owner != owner)
    return 0;

  if (owner >= O_USER)
    quota_free_mem(owner, address, L4_SUPERPAGESIZE);

  s->owner = O_FREE;
  s->free_pages = L4_SUPERPAGESIZE/L4_PAGESIZE;

  /* assume __memmap[] is correctly set up here (all single pages
     marked free) */
  return 1;
}

int
memmap_alloc_superpage(l4_addr_t address, owner_t owner)
{
  __superpage_t *s = __memmap4mb + (address - ram_base)/L4_SUPERPAGESIZE;

  if (s >= __memmap4mb + SUPERPAGE_MAX)
    return 0;			/* address exceeds __memmap4mb array */
  if (s->owner == owner)
    return 1;			/* already allocated */
  if (s->owner != O_FREE)
    {
      /* check if all pages inside belong to the new owner already */
      owner_t *m, *sm;
      l4_size_t already_have = 0;

      if (s->owner != O_RESERVED)
	return 0;

      address &= L4_SUPERPAGEMASK;
      if (address-ram_base >= mem_high)
	return 0;

      for (sm = m = __memmap + (address - ram_base)/L4_PAGESIZE;
	   m < sm + L4_SUPERPAGESIZE/L4_PAGESIZE;
	   m++)
	{
	  if (! (*m == O_FREE || *m == owner))
	    return 0;		/* don't own superpage exclusively */

	  if (*m == owner)
	    already_have += L4_PAGESIZE;
	}

      if (owner >= O_USER)
	{
	  if (already_have)
	    {
	      /* XXX we're sloppy with the exact addresses we free here,
		 knowing that the address doesn't really matter to
		 quota_free_mem() */
	      quota_free_mem(owner, address, already_have);
	    }

	  if (! quota_alloc_mem(owner, address, L4_SUPERPAGESIZE))
	    {
	      if (already_have)
		/* XXX we deal with the quota's private values here... :-( */
		quota_add_mem(owner, already_have); /* reset */

	      return 0;
	    }
	}

      /* ok, we're the exclusive owner of this superpage */
      memset(sm, O_FREE, L4_SUPERPAGESIZE/L4_PAGESIZE);
    }
  else
    {
      if (owner >= O_USER)
	if (! quota_alloc_mem(owner, address & L4_SUPERPAGEMASK,
			      L4_SUPERPAGESIZE))
	  return 0;
    }

  s->owner = owner;
  return 1;
}

owner_t
memmap_owner_superpage(l4_addr_t address)
{
  __superpage_t *s = __memmap4mb + (address - ram_base)/L4_SUPERPAGESIZE;

  if (s >= __memmap4mb + SUPERPAGE_MAX)
    return O_RESERVED;		/* address exceeds __memmap4mb array */

  return s->owner;
}

unsigned
memmap_nrfree_superpage(l4_addr_t address)
{
  __superpage_t *s = __memmap4mb + (address - ram_base)/L4_SUPERPAGESIZE;

  if (s >= __memmap4mb + SUPERPAGE_MAX)
    return 0;			/* address exceeds __memmap4mb array */
  if (s->owner == O_FREE || s->owner == O_RESERVED)
    return s->free_pages;

  return 0;
}

static void
memmap_dump_region(l4_addr_t start, l4_addr_t end, owner_t owner)
{
  const char *h_beg = l4_version == VERSION_FIASCO ? "\033[33m" : "";
  const char *h_end = l4_version == VERSION_FIASCO ? "\033[m"   : "";

  printf("  [%08lx-%08lx) [%s%7ldKB%s] ",
      start, end, owner == O_FREE ? h_beg : "", 
      (end-start) >> 10, owner == O_FREE ? h_end : "");
  if (owner == O_FREE)
    printf("[%sfree%s]", h_beg, h_end);
  else if (owner == O_RESERVED)
    printf("[reserved]");
  else
    printf("#%02x", owner);
  putchar('\n');
}

static void
memmap_dump_area(l4_addr_t start, l4_addr_t end, owner_t owner)
{
  int region;

  while ((region = region_find(start + ram_base, end + ram_base)) > 0)
    {
      region_t *r = region_get(region);

      if (r->begin > start + ram_base)
	memmap_dump_region(start + ram_base, r->begin, owner);

      printf("  [%08lx-%08lx) [%7ldKB] %s",
	  r->begin, r->end, (r->end - r->begin) >> 10, r->name);
      putchar('\n');
      start = r->end - ram_base;
    }

  if (start < end)
    memmap_dump_region(start + ram_base, end + ram_base, owner);
}

void
memmap_dump(void)
{
  l4_addr_t start, end;
  owner_t old_state, new_state;

  printf("Roottask memmap:\n");
  for (old_state = memmap_owner_page(ram_base), start = end = 0;
       end < PAGE_MAX; end++)
    {
      new_state = memmap_owner_page((end<<L4_PAGESHIFT) + ram_base);
      if (old_state != new_state)
	{
	  memmap_dump_area(start<<L4_PAGESHIFT, end<<L4_PAGESHIFT, old_state);
	  old_state = new_state;
	  start     = end;
	}
    }
  memmap_dump_area(start<<L4_PAGESHIFT, end<<L4_PAGESHIFT, old_state);
}

l4_addr_t
find_free_chunk(l4_umword_t pages, int down)
{
  int i;
  l4_addr_t addr;

  if (down)
    {
      for (i=0, addr=mem_high-L4_PAGESIZE;
		addr>0x00400000 && i<pages; addr-=L4_PAGESIZE)
	i = ((memmap_owner_page(addr) != O_FREE)) ? 0 : i+1;
      addr += L4_PAGESIZE;
    }
  else
    {
      for (i=0, addr=0x00400000; addr<mem_high && i<pages; addr+=L4_PAGESIZE)
      i = ((memmap_owner_page(addr) != O_FREE))	? 0 : i+1;
      addr -= pages*L4_PAGESIZE;
    }

  return (i>=pages) ? addr : 0;
}

l4_addr_t
reserve_range(unsigned int size_and_align, owner_t owner,
              l4_addr_t range_low, l4_addr_t range_high)
{
#ifndef min
#define min(x,y) ((x)<(y)?(x):(y))
#endif
  l4_addr_t address, min_address, max_address;
  unsigned int have_pages;
  unsigned int size  = size_and_align & L4_PAGEMASK;
  unsigned int align = size_and_align & ~L4_PAGEMASK & ~RMGR_MEM_RES_FLAGS_MASK;
  unsigned int flags = size_and_align & ~L4_PAGEMASK & RMGR_MEM_RES_FLAGS_MASK;
  unsigned int need_pages = size / L4_PAGESIZE;

  /* suggestive round alignment */
  if (align < L4_LOG2_PAGESIZE)
    align = L4_LOG2_PAGESIZE;
  else if (align > 28)
    align = 28;
  align = ~((1 << align) - 1);

  max_address = mem_high + ram_base;

  if (range_high != 0)
    max_address = min(max_address, range_high);

  if (flags & RMGR_MEM_RES_DMA_ABLE)
    max_address = min(max_address, 16 << 20);

  /* round down to next L4 page */
  max_address &= L4_PAGEMASK;

  /* round up to next L4 page */
  min_address = (range_low + ~L4_PAGEMASK) & L4_PAGEMASK;

  min_address += ram_base;

  /* some sanity checks */
  if (max_address-min_address < size)
    return -1;

  if (flags & RMGR_MEM_RES_UPWARDS)
    {
      /* search upwards min_address til max_address */
      max_address -= size;

      for (address = min_address; ; )
	{
	  /* round up to next proper alignend chunk */
	  address = (address + ~align) & align;

	  if (address > max_address)
	    return -1;

	  for (have_pages = 0; ; address += L4_PAGESIZE)
	    {
	      if (!quota_check_mem(owner, address, L4_PAGESIZE)
		  || memmap_owner_page(address) != O_FREE)
		break;

	      if (++have_pages >= need_pages)
		{
		  address -= size - L4_PAGESIZE;
		  goto found;
		}
	    }

	  address += L4_PAGESIZE;
	}
    }
  else
    {
      /* search downwards mem_high-0 */
      for (address = max_address; address >= min_address + size; )
	{
	  /* round down to next proper aligned chunk */
	  address = ((address-size) & align) + size;

	  for (have_pages = 0;; )
	    {
	      address -= L4_PAGESIZE;

	      if (!quota_check_mem(owner, address, L4_PAGESIZE)
		  || memmap_owner_page(address) != O_FREE)
		break;

	      if (++have_pages >= need_pages)
		{
		  /* we have found a chunk which is big enough */
		  int ret, a;
	found:

		  for (a=address; a<address+size; a+=L4_PAGESIZE)
		    {
		      ret = memmap_alloc_page(a, owner);
		      //assert(ret);
		    }

		  return address;
		}
	    }
	}
    }

  /* nothing found! */
  return -1;
}
