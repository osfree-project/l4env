INTERFACE:


class List_alloc
{
private:
  friend class List_alloc_sanity_guard;

  struct Mem_block
  {
    Mem_block *next;
    unsigned long size;
  };

  Mem_block *_first;
};


//----------------------------------------------------------------------------
IMPLEMENTATION [!list_alloc_debug]:

class List_alloc_sanity_guard
{};

PUBLIC inline NOEXPORT
List_alloc_sanity_guard::List_alloc_sanity_guard(List_alloc *, 
                                                 char const *)
{}

PRIVATE inline NOEXPORT
void 
List_alloc::check_overlap(void *, unsigned long )
{}

PRIVATE inline NOEXPORT
void 
List_alloc::sanity_check_list(char const *, char const *)
{}


//----------------------------------------------------------------------------
IMPLEMENTATION [list_alloc_debug]:

class List_alloc_sanity_guard
{
private:
  List_alloc *a;
  char const *func;
};

PUBLIC inline NOEXPORT
List_alloc_sanity_guard::List_alloc_sanity_guard(List_alloc *a, 
                                                 char const *func)
  : a(a), func(func)
{ a->sanity_check_list(func, "entry"); }

PUBLIC inline NOEXPORT
List_alloc_sanity_guard::~List_alloc_sanity_guard()
{ a->sanity_check_list(func, "exit"); }

PRIVATE
void 
List_alloc::check_overlap(void *b, unsigned long s)
{
  Mem_block *c = _first;
  for (;c ; c = c->next)
    {
      unsigned long x_s = (unsigned long)b;
      unsigned long x_e = x_s + s;
      unsigned long b_s = (unsigned long)c;
      unsigned long b_e = b_s + c->size;

      if (   (x_s >= b_s && x_s < b_e)
	  || (x_e > b_s && x_e <= b_e)
	  || (b_s >= x_s && b_s < x_e)
	  || (b_e > x_s && b_e <= x_e))
	{
	  printf("trying to free memory that is already free: \n"
	         "  [%lx-%lx) overlaps [%lx-%lx)\n",
		 x_s, x_e, b_s, b_e );
	  kdb_ke("List_alloc");
	}
    }
}

PRIVATE
void 
List_alloc::sanity_check_list(char const *func, char const *info)
{
  Mem_block *c = _first;
  for (;c ; c = c->next)
    {
      if (c->next)
	{
	  if (c >= c->next)
	    {
	      printf("%s: %s(%s): list order violation\n",
		     __FILE__, func, info);
	      kdb_ke("List_alloc");
	    }

	  if (((unsigned long)c) + c->size > (unsigned long)c->next)
	    {
	      printf("%s: %s(%s): list order violation\n",
		     __FILE__, func, info);
	      kdb_ke("List_alloc");
	    }
	}
    }
}


//----------------------------------------------------------------------------
IMPLEMENTATION:

#include <cstdio>
#include <cassert>
#include "kdb_ke.h"


PUBLIC
void 
List_alloc::dump()
{
  printf("List_alloc [_first=%p]\n", _first);
  Mem_block *c = _first;
  for (;c && c!=c->next ; c = c->next)
    printf("  Mem_block [this=%p size=0x%lx (%ldkB) next=%p]\n", c, c->size, 
	   (c->size+1023)/1024, c->next);

  if (c && c == c->next)
    printf("  BUG: loop detected\n");
}

PUBLIC 
List_alloc::List_alloc()
//  : _first(0) 
{}

PUBLIC inline
void 
List_alloc::init()
{ _first = 0; }
  
PRIVATE inline NOEXPORT
void
List_alloc::merge()
{
  List_alloc_sanity_guard __attribute__((unused)) guard(this, __func__);
  Mem_block *c = _first;
  while (c && c->next)
    {
      unsigned long f_start = (unsigned long)c;
      unsigned long f_end   = f_start + c->size;
      unsigned long n_start = (unsigned long)c->next;
      
      if (f_end == n_start)
	{
	  c->size += c->next->size;
	  assert(c->size >= sizeof(Mem_block));
	  c->next = c->next->next;
	  continue;
	}

      c = c->next;
    }
}

PUBLIC
void 
List_alloc::free(void *block, unsigned long size)
{
  List_alloc_sanity_guard __attribute__((unused)) guard(this, __func__);

#if 0
  if (block <= (void*)(0xf0000000))
    {
      printf("Someone tries to free BAD memory @ %p(%lx)\n", block,size);
      kdb_ke("BAD free");
    }
#endif

  check_overlap(block, size);
  
  assert(size >= sizeof(Mem_block));
  Mem_block **c = &_first;
  Mem_block *next = 0;
  
  if (*c)
    {
      while (*c && *c < block)
	c = &(*c)->next;

      next = *c;
    }
  
  assert(*c != block);
  *c = (Mem_block*)block;
  assert(*c != next);
  
  (*c)->next = next;
  (*c)->size = size;

  assert((!next) || ((unsigned long)(*c) + size <= (unsigned long)next));
  
  merge();
}

PUBLIC
void *
List_alloc::alloc(unsigned long size, unsigned align)
{
  List_alloc_sanity_guard __attribute__((unused)) guard(this, __func__);
  
  unsigned long almask = (1 << align) -1;
  Mem_block **c = &_first;
  void *b = 0;
  for (; *c; c=&(*c)->next)
    {
      unsigned long n_start = (unsigned long)(*c);
      unsigned long a_start;
      unsigned long a_size;
      
      if ((*c)->size < size)
	continue;
      
      if (!(n_start & almask))
	{
	  if ((*c)->size >= size)
	    {
	      if ((*c)->size == size)
		{
		  b = *c;
		  *c = (*c)->next;
		  return b;
		}
	      
	      Mem_block *m = (Mem_block*)(n_start + size);
	      m->next = (*c)->next;
	      m->size = (*c)->size - size;
	      assert(m->size >= sizeof(Mem_block));
	      b = *c;
	      *c = m;
	      return b;
	    }
	  
	  continue;
	}

      a_start = (n_start & ~almask) + 1 + almask;
      if (a_start - n_start >= (*c)->size)
	continue;

      a_size = (*c)->size - a_start + n_start;

      if (a_size >= size)
	{
	  if (a_size == size)
	    {
	      (*c)->size -= size;
	      return (void*)a_start;
	    }
	  Mem_block *m = (Mem_block*)(a_start + size);
	  m->next = (*c)->next;
	  m->size = a_size - size;
	  assert(m->size >= sizeof(Mem_block));
	  (*c)->size -= a_size;
	  assert((*c)->size >= sizeof(Mem_block));
	  (*c)->next = m;
	  return (void*)a_start;
	}
    }

  return 0;
}

PUBLIC
unsigned long
List_alloc::avail()
{
  List_alloc_sanity_guard __attribute__((unused)) guard(this, __FUNCTION__);
  Mem_block *c = _first;
  unsigned long a = 0;
  while (c)
    {
      a += c->size;
      c = c->next;
    }

  return a;
}

