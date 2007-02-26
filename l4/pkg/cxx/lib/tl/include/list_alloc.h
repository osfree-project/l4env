#ifndef CXX_LIST_ALLOC_H__
#define CXX_LIST_ALLOC_H__
namespace cxx {

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

  inline void check_overlap(void *, unsigned long );
  inline void sanity_check_list(char const *, char const *);
  inline void merge();

public:
  List_alloc() : _first(0) {}
  inline void free(void *block, unsigned long size);
  inline void *alloc(unsigned long size, unsigned align);
  inline unsigned long avail();
};

#if !defined (CXX_LIST_ALLOC_SANITY)
class List_alloc_sanity_guard
{
public:
  List_alloc_sanity_guard(List_alloc *, char const *)
  {}

};


void 
List_alloc::check_overlap(void *, unsigned long )
{}

void 
List_alloc::sanity_check_list(char const *, char const *)
{}

#else

class List_alloc_sanity_guard
{
private:
  List_alloc *a;
  char const *func;
  
public:
  List_alloc_sanity_guard(List_alloc *a, char const *func)
    : a(a), func(func)
  { a->sanity_check_list(func, "entry"); }
  
  ~List_alloc_sanity_guard()
  { a->sanity_check_list(func, "exit"); }
};

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
	  L4::cerr << "List_alloc(FATAL): trying to free memory that "
					  "is already free: \n  ["
	    << (void*)x_s << '-' << (void*)x_e << ") overlaps ["
	    << (void*)b_s << '-' << (void*)b_e << ")\n";
	}
    }
}

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
	      L4::cerr << "List_alloc(FATAL): " << func << '(' << info 
		<< "): list oerder violation\n";
	    }

	  if (((unsigned long)c) + c->size > (unsigned long)c->next)
	    {
	      L4::cerr << "List_alloc(FATAL): " << func << '(' << info 
		<< "): list oerder violation\n";
	    }
	}
    }
}

#endif

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
	  c->next = c->next->next;
	  continue;
	}

      c = c->next;
    }
}

void 
List_alloc::free(void *block, unsigned long size)
{
  List_alloc_sanity_guard __attribute__((unused)) guard(this, __func__);

  check_overlap(block, size);
  
  Mem_block **c = &_first;
  Mem_block *next = 0;
  
  if (*c)
    {
      while (*c && *c < block)
	c = &(*c)->next;

      next = *c;
    }
  
  *c = (Mem_block*)block;
  
  (*c)->next = next;
  (*c)->size = size;

  merge();
}

void *
List_alloc::alloc(unsigned long size, unsigned align)
{
  List_alloc_sanity_guard __attribute__((unused)) guard(this, __func__);
  
  unsigned long almask = align?(align -1):0;
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
	  (*c)->size -= a_size;
	  (*c)->next = m;
	  return (void*)a_start;
	}
    }

  return 0;
}

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


}

#endif

