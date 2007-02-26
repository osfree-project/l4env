#ifndef MEMMAP_H
#define MEMMAP_H

#include <string.h>
#include <assert.h>
#include <flux/machine/types.h>
#include <l4/sys/compiler.h>
#include <l4/sys/types.h>
#include <l4/sys/kernel.h>

#include "globals.h"

#define MEM_MAX (256UL*1024UL*1024UL) /* max RAM we handle */
#define SUPERPAGE_MAX (1024)	/* no of superpages in the system */
#define IO_MAX L4_IOPORT_MAX

typedef struct {
  l4_uint16_t free_pages;
  owner_t owner;
  l4_uint8_t padding;
} __attribute__((packed)) __superpage_t;

extern owner_t __memmap[MEM_MAX/L4_PAGESIZE];
extern __superpage_t __memmap4mb[SUPERPAGE_MAX];
extern vm_offset_t mem_high;
extern owner_t __iomap[IO_MAX];

extern l4_kernel_info_t *l4_info;

void pager(void) L4_NORETURN;

L4_INLINE void map_init(void);
L4_INLINE int memmap_free_page(vm_offset_t address, owner_t owner);
L4_INLINE int memmap_alloc_page(vm_offset_t address, owner_t owner);
L4_INLINE owner_t memmap_owner_page(vm_offset_t address);
L4_INLINE int memmap_free_superpage(vm_offset_t address, owner_t owner);
L4_INLINE int memmap_alloc_superpage(vm_offset_t address, owner_t owner);
L4_INLINE owner_t memmap_owner_superpage(vm_offset_t address);
L4_INLINE unsigned memmap_nrfree_superpage(vm_offset_t address);

L4_INLINE int iomap_alloc_port(unsigned port, owner_t owner);


L4_INLINE void map_init(void)
{
  unsigned i;
  __superpage_t *s;

  memset(__memmap, O_RESERVED, MEM_MAX/L4_PAGESIZE);
  for (s = __memmap4mb, i = 0; i < SUPERPAGE_MAX; i++, s++)
    *s = (__superpage_t) { 0, O_RESERVED, 0 };
  memset(__iomap, O_FREE, IO_MAX);
}

L4_INLINE int memmap_free_page(vm_offset_t address, owner_t owner)
{
  owner_t *m = __memmap + address/L4_PAGESIZE;
  __superpage_t *s = __memmap4mb + address/L4_SUPERPAGESIZE;

  if ((address & L4_PAGEMASK) >= MEM_MAX)
    return 1;
  if (s->owner == O_FREE)
    return 1;			/* already free */
  if (s->owner != O_RESERVED)
    return 0;
  if (*m == O_FREE)
    return 1;			/* already free */
  if (*m != owner)
    return 0;

  *m = O_FREE;
  s->free_pages++;

  if (s->free_pages == L4_SUPERPAGESIZE/L4_PAGESIZE)
    s->owner = O_FREE;
  return 1;
}

L4_INLINE int memmap_alloc_page(vm_offset_t address, owner_t owner)
{
  owner_t *m = __memmap + address/L4_PAGESIZE;
  __superpage_t *s = __memmap4mb + address/L4_SUPERPAGESIZE;

  if ((address & L4_PAGEMASK) >= MEM_MAX)
    return 0;
  if (*m == owner)
    return 1;			/* already allocated */
  if (*m != O_FREE)
    return 0;

  if (s->owner == O_FREE)
    s->owner = O_RESERVED;
  else if (s->owner != O_RESERVED)
    return 0;

  s->free_pages--;
  *m = owner;
  return 1;
}

L4_INLINE owner_t memmap_owner_page(vm_offset_t address)
{
  owner_t *m = __memmap + address/L4_PAGESIZE;
  __superpage_t *s = __memmap4mb + address/L4_SUPERPAGESIZE;

  if ((address & L4_PAGEMASK) >= MEM_MAX)
    return O_RESERVED;

  return s->owner == O_RESERVED ? *m : s->owner;
}

L4_INLINE int memmap_free_superpage(vm_offset_t address, owner_t owner)
{
  __superpage_t *s = __memmap4mb + address/L4_SUPERPAGESIZE;

  if (s->owner == O_FREE)
    return 1;			/* already free */
  if (s->owner != owner)
    return 0;

  s->owner = O_FREE;
  s->free_pages = L4_SUPERPAGESIZE/L4_PAGESIZE;

  /* assume __memmap[] is correctly set up here (all single pages
     marked free) */
  return 1;
}

L4_INLINE int memmap_alloc_superpage(vm_offset_t address, owner_t owner)
{
  __superpage_t *s = __memmap4mb + address/L4_SUPERPAGESIZE;

  if (s->owner == owner)
    return 1;			/* already allocated */
  if (s->owner != O_FREE)
    {
      /* check if all pages inside belong to the new owner already */

      owner_t *m, *sm;
      vm_size_t already_have = 0;

      if (s->owner != O_RESERVED)
	return 0;

      address &= L4_SUPERPAGEMASK;
      if (address >= MEM_MAX)
	return 0;

      for (sm = m = __memmap + address/L4_PAGESIZE; 
	   m < sm + L4_SUPERPAGESIZE/L4_PAGESIZE;
	   m++)
	{
	  if (! (*m == O_FREE || *m == owner))
	    return 0;		/* don't own superpage exclusively */

	  if (*m == owner)
	    already_have += L4_PAGESIZE;
	}

      /* ok, we're the exclusive owner of this superpage */
      memset(sm, O_FREE, L4_SUPERPAGESIZE/L4_PAGESIZE);      

    }
  
  s->owner = owner;
  return 1;
}

L4_INLINE owner_t memmap_owner_superpage(vm_offset_t address)
{
  __superpage_t *s = __memmap4mb + address/L4_SUPERPAGESIZE;

  return s->owner;
}

L4_INLINE unsigned memmap_nrfree_superpage(vm_offset_t address)
{
  __superpage_t *s = __memmap4mb + address/L4_SUPERPAGESIZE;

  if (s->owner == O_FREE || s->owner == O_RESERVED)
    return s->free_pages;

  return 0;
}

L4_INLINE int iomap_alloc_port(unsigned port, owner_t owner)
{
  owner_t *p = __iomap + port;

  if (*p == owner)
    return 1;			/* already allocated */
  if (*p != O_FREE)
    return 0;

  *p = owner;
  return 1;
}

#endif
