INTERFACE:

#include <cassert>

class Bitmap_base
{
private:
  unsigned long _bits[0];
};

template<int BITS>
class Bitmap : public Bitmap_base
{
private:
  char _bits[(BITS+7)/8];
};

class Buddy_base
{

protected:

  bool _debug;
  unsigned long _base;

  struct Head
  {
    Head *next;
    Head **prev_next;
    unsigned long index;

    void unlink()
    { 
      assert (prev_next);

      if (next)
	next->prev_next = prev_next;
      *prev_next = next;
    }

    static void link(Head **h, void *b, unsigned idx)
    {
      Head *n = (Head*)b;
      n->index = idx;
      n->next = *h;
      n->prev_next = h;
      if (*h)
        (*h)->prev_next = &(n->next);
      *h = n;
    }
  };
};

template< int MIN_LOG2_SIZE, int NUM_SIZES, int MAX_MEM >
class Buddy_t_base : public Buddy_base
{
public:
  enum
  {
    Min_log2_size = MIN_LOG2_SIZE,
    Min_size = 1UL << MIN_LOG2_SIZE,
    Num_sizes = NUM_SIZES,
	Max_size = Min_size << NUM_SIZES,
    Max_mem = MAX_MEM,
  };

private:
  Head *_free[Num_sizes];
  Bitmap<(Max_mem+Min_size-1)/Min_size> _free_map;
};


class Buddy_alloc : public Buddy_t_base<10, 8, 64*1024*1024>
{
};


//----------------------------------------------------------------------------
IMPLEMENTATION:

#include <cstdio>
#include <cassert>
#include "kdb_ke.h"

PUBLIC inline
void
Bitmap_base::clear_bit(unsigned long bit)
{
  unsigned long idx = bit / (sizeof(unsigned long) * 8);
  unsigned long b   = bit % (sizeof(unsigned long) * 8);
  _bits[idx] &= ~(1UL << b);
}

PUBLIC inline
void
Bitmap_base::set_bit(unsigned long bit)
{
  unsigned long idx = bit / (sizeof(unsigned long) * 8);
  unsigned long b   = bit % (sizeof(unsigned long) * 8);
  _bits[idx] |= (1UL << b);
}

PUBLIC inline
unsigned long
Bitmap_base::operator [] (unsigned long bit) const
{
  unsigned long idx = bit / (sizeof(unsigned long) * 8);
  unsigned long b   = bit % (sizeof(unsigned long) * 8);
  return _bits[idx] & (1UL << b);
}

PRIVATE
template<int A, int B, int M> 
inline
Buddy_base::Head *
Buddy_t_base<A,B,M>::buddy(void *block, unsigned long index, Head **new_block)
{
  //printf("buddy(%p, %ld)\n", block, index);
  unsigned long const size = Min_size << index;
  unsigned long const n_size = size << 1;
  if (index + 1 >= Num_sizes)
    return 0;
  unsigned long b = (unsigned long)block;
  unsigned long _buddy = b & ~(n_size-1);
  *new_block = (Head*)_buddy;
  if (_buddy == b)
    _buddy += size;

  Head * const _buddy_h = (Head*)_buddy;

  if (_free_map[(_buddy - _base)/Min_size] && _buddy_h->index == index)
    return _buddy_h;

  return 0;
}

PUBLIC
template<int A, int B, int M>
inline
void
Buddy_t_base<A,B,M>::free(void *block, unsigned long size)
{
  assert ((unsigned long)block >= _base);
  assert ((unsigned long)block - _base < Max_mem);
  assert (!_free_map[((unsigned long)block - _base) / Min_size]);
  //bool _b = 0;
  //if (_debug) printf("Buddy::free(%p, %ld)\n", block, size);
  unsigned size_index = 0;
  while (((unsigned long)Min_size << size_index) < size)
    ++size_index;

  assert (size == (unsigned long)Min_size << size_index);

  while (size_index < Num_sizes)
    {
      Head *n, *b;
      b = buddy(block, size_index, &n);
      if (b)
	{
	//if (!_b && _debug) dump();
	//if (_debug) printf("  found buddy %p (n=%p size=%ld)\n", b, n, size_index+1);
	  b->unlink();
	  block = n;
	  ++size_index;
	  //_b = 1;
	}
      else
	break;
    }

  //printf("  link free %p\n", block);
  Head::link(_free + size_index, block, size_index);
  _free_map.set_bit(((unsigned long)block - _base) / Min_size);
  //if (_b && _debug) dump();
}


PUBLIC
template<int A, int B, int M>
void
Buddy_t_base<A,B,M>::add_mem(void *b, unsigned long size)
{
  unsigned long start = (unsigned long)b;
  unsigned long al_start;
  al_start = (start + Min_size - 1) & ~(Min_size -1);

  //printf("Buddy::add_mem(%p, %lx): al_start=%lx; _base=%lx\n", b, size, al_start, _base);

  // _debug = 0;
  if (size <= al_start-start)
    return;

  size -= (al_start-start);
  size &= ~(Min_size -1);

  while (size)
    {
      free((void*)al_start, Min_size);
      al_start += Min_size;
      size -= Min_size;
    }
  // _debug = 1;
  //dump();
}



PRIVATE
template<int A, int B, int M>
inline
void
Buddy_t_base<A,B,M>::split(Head *b, unsigned size_index, unsigned i)
{
  //unsigned si = size_index;
  //printf("Buddy::split(%p, %d, %d)\n", b, size_index, i);
  for (; i > size_index; ++size_index)
    {
      unsigned long buddy = (unsigned long)b + (Min_size << size_index);
      Head::link(_free + size_index, (void*)buddy, size_index);
      _free_map.set_bit((buddy - _base) / Min_size);
    }

  //if (si!=i) dump();
}

PUBLIC
template<int A, int B, int M>
inline
void *
Buddy_t_base<A,B,M>::alloc(unsigned long size)
{
  unsigned size_index = 0;
  while (((unsigned long)Min_size << size_index) < size)
    ++size_index;

  //printf("Buddy::alloc(%ld): size_index=%d ", size, size_index);
  //

  for (unsigned i = size_index; i < Num_sizes; ++i)
    {
      Head *f = _free[i];
      if (f)
	{
	  f->unlink();
	  split(f, size_index, i);
	  _free_map.clear_bit(((unsigned long)f - _base) / Min_size);
	  //printf(" =%p\n", f);
	  return f;
	}
    }
  return 0;
}

PUBLIC
template< int A, int B, int M >
void 
Buddy_t_base<A,B,M>::dump() const
{
  printf("Buddy_alloc [%d,%d]\n", Min_size, Num_sizes);
  for (unsigned i = 0; i < Num_sizes; ++i)
    {
      unsigned c = 0;
      Head *h = _free[i];
      printf("  [%d] %p(%ld)", Min_size << i, h, h?h->index:0UL);
      while (h)
	{
	  h = h->next;
	  if (c < 5)
	    printf(" -> %p(%ld)", h, h?h->index:0UL);
	  else
	    {
	      printf(" ...");
	      break;
	    }

	  ++c;
	}
      printf("\n");
    }
}

PUBLIC
void
Buddy_base::init(unsigned long base)
{ _base = base; }

PUBLIC
template< int A, int B, int M >
unsigned long
Buddy_t_base<A,B,M>::avail() const
{
  unsigned long a = 0;
  for (unsigned i = 0; i < Num_sizes; ++i)
    {
      Head *h = _free[i];
      while (h)
	{
	  a += (Min_size << i);
	  h = h->next;
	}
    }
  return a;
}

