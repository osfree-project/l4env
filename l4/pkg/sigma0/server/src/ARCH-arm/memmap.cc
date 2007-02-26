#include <l4/sys/types.h>

#include "memmap.h"

bool Memmap::free(l4_addr_t address, char owner)
{
  Page *p = atop(address);
  SuperPage *s = atos(address);

  if(!s || !p)
    return false;
  if(s->is_free())
    return true;
  if(!s->is_reserved())
    return false;
  if(p->is_free())
    return true;
  if(p->owner() != owner)
    return false;

  p->free();  
  s->inc_free();

  return true;
}

bool Memmap::free_superpage(l4_addr_t address, char owner)
{
  SuperPage *s = atos(address);

  if(!s)
    return false;
  if(s->is_free())
    return true;
  if(s->owner() != owner)
    return false;

  s->free();  

  return true;
}

char Memmap::alloc_r( unsigned long addr, char owner, unsigned &grain )
{
  Region *r = regions->find(addr);
  if (!r)
    return 2;

  grain = r->grain();
  
  if (r->owner(addr)==owner)
    return 1;
  if (r->is_free(addr))
    {
      r->owner(owner);
      return 1;
    }
  return 0;
}

bool Memmap::alloc(l4_addr_t address, char owner, unsigned &grain)
{ 
  char ra = alloc_r(address, owner, grain);
  if (ra==0)
    return false;
  else if (ra==1)
    return true;

  grain = 12; // 4k Pages

  Page *p = atop(address);
  SuperPage *s = atos(address);

  if(p->owner() == owner)
    return true;
  if(!p->is_free())
    return false;

  if(s->is_free())
    s->reserve();
  else if(!s->is_reserved())
    return false;
    
  s->dec_free();
  p->change_owner(owner);

  return true;
}

bool Memmap::alloc_superpage(l4_addr_t address, char owner)
{
  SuperPage *s = atos(address);

  if(s->owner() == owner)
    return true;
  if(!s->is_free()) 
    { 
      if (!s->is_reserved())
        return false;

      Page *p, *sp;

      for (p = sp = atop(address); 
           p < sp + L4_SUPERPAGESIZE/L4_PAGESIZE;
           p++)
        {
          if (!p->is_free() || !(p->owner() == owner))
            return false;		/* don't own superpage exclusively */

        }

      for (p = sp = atop(address); 
           p < sp + L4_SUPERPAGESIZE/L4_PAGESIZE;
           p++)
        p->free();
    }

  s->change_owner(owner);

  return true;
}

char Memmap::owner(l4_addr_t address) const
{
  Region *r = regions->find(address);
  if (r)
    return r->owner(address);
  
  Page const *p = atop(address);
  SuperPage const *s= atos(address);
  if(!p || !s)
    return 1;

  if(s->is_reserved())
    return p->owner();
  else
    return s->owner();
}

char Memmap::owner_superpage(l4_addr_t address) const
{
  SuperPage const *s= atos(address);
  if(!s)
    return 1;
  else
    return s->owner();
}

l4_uint16_t Memmap::nr_free(l4_addr_t address) const
{
  SuperPage const *s = atos(address);
  if(s && (s->is_reserved() || s->is_free()))
    return s->nr_free();
  else
    return 0;
}


void Memmap::find_free(l4_umword_t *d1, l4_umword_t *d2, char _owner)
{
  /* XXX this routine should be in the memmap_*() interface because we
     don't know about quotas here.  we can easily select a free page
     which we later can't allocate because we're out of quota. */

  /* for kernel tasks, start looking at the back */
  l4_addr_t address = _owner < L4_ROOT_TASKNO 
    ? (LOWER_LIMIT + mem_high - 1) & L4_SUPERPAGEMASK
    : LOWER_LIMIT;

  for (;;)
    {
      if (nr_free(address) != 0)
        {
          if (_owner < L4_ROOT_TASKNO) address += L4_SUPERPAGESIZE - L4_PAGESIZE;

          for (;;)
            {
              if (owner(address) != 0)
                {
                  if (_owner < L4_ROOT_TASKNO) address -= L4_PAGESIZE;
                  else address += L4_PAGESIZE;

                  continue;
                }
	      
              /* found! */
              *d1 = address;
              *d2 = l4_fpage(address, L4_LOG2_PAGESIZE, 1, 0).raw;
	      
              return;
            }
        }

      if (_owner < L4_ROOT_TASKNO)
        {
          if (address <= LOWER_LIMIT) break;
          address -= L4_SUPERPAGESIZE;
        }
      else
        {
          address += L4_SUPERPAGESIZE;
          if (address >= LOWER_LIMIT + mem_high) break;
        }
    }

  /* nothing found! */
  *d1 = *d2 = 0;
}
