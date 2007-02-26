#ifndef HX_SLAB_ALLOC_H__
#define HX_SLAB_ALLOC_H__

#include <l4/cxx/std_alloc.h>
#include <l4/cxx/list.h>
#include <l4/sys/consts.h>

namespace cxx {

template< int Obj_size, int Slab_size = L4_PAGESIZE,
  int Max_free = 2, template<typename A> class Alloc = New_allocator >
class Base_slab
{
private:
  struct Free_o
  {
    Free_o *next;
  };

public:
  struct Slab_i;

private:
  struct Slab_head : public List_item
  {
    unsigned num_free;
    Free_o *free;
    Base_slab<Obj_size, Slab_size, Max_free, Alloc> *cache;

    inline Slab_head() : num_free(0), free(0), cache(0) 
    {}
  };

public:
  enum
  {
    object_size      = Obj_size,
    slab_size        = Slab_size,
    objects_per_slab = (Slab_size - sizeof(Slab_head)) / object_size,
    max_free_slabs   = Max_free,
  };

  struct Slab_store
  { 
    char _o[slab_size - sizeof(Slab_head)]; 
    Free_o *object(unsigned obj)
    { return reinterpret_cast<Free_o*>(_o + object_size * obj); }
  };

  struct Slab_i : public Slab_store, public Slab_head
  {};
  
public:
  typedef Alloc<Slab_i> Slab_alloc;

private:
  Slab_alloc _alloc;
  unsigned _num_free;
  unsigned _num_slabs;
  Slab_i *_full_slabs;
  Slab_i *_partial_slabs;
  Slab_i *_empty_slabs;

  void add_slab(Slab_i *s)
  {
    s->num_free = objects_per_slab;
    s->cache = this;

    //L4::cerr << "Slab: " << this << "->add_slab(" << s << ", size=" 
    //  << slab_size << "):" << " f=" << s->object(0) << '\n';

    // initialize free list
    Free_o *f = s->free = s->object(0);
    for (unsigned i = 0; i < objects_per_slab; ++i)
      {
	f->next = s->object(i);
	f = f->next;
      }
    f->next = 0;

    // insert slab into cache's list
    _empty_slabs = List_item::push_front(_empty_slabs, s);
    ++_num_slabs;
    ++_num_free;
  }

  bool grow()
  {
    Slab_i *s = _alloc.alloc();
    if (!s)
      return false;

    new (s) Slab_i();

    add_slab(s);
    return true;
  }

  void shrink()
  {
    if (!_alloc.can_free)
      return;

    while (_empty_slabs && _num_free > max_free_slabs)
      {
	Slab_i *s = _empty_slabs;
	_empty_slabs = List_item::remove(_empty_slabs, s);
	--_num_free;
	--_num_slabs;
	_alloc.free(s);
      }
  }

public:
  Base_slab(Slab_alloc const &alloc = Slab_alloc()) 
    : _alloc(alloc), _num_free(0), _num_slabs(0), _full_slabs(0), 
      _partial_slabs(0), _empty_slabs(0)
  {}

  ~Base_slab()
  {
    while (_empty_slabs)
      {
	Slab_i *o = _empty_slabs;
	_empty_slabs = List_item::remove(_empty_slabs, o);
	_alloc.free(o);
      }
    while (_partial_slabs)
      {
	Slab_i *o = _partial_slabs;
	_partial_slabs = List_item::remove(_partial_slabs, o);
	_alloc.free(o);
      }
    while (_full_slabs)
      {
	Slab_i *o = _full_slabs;
	_full_slabs = List_item::remove(_full_slabs, o);
	_alloc.free(o);
      }
  }

  void *alloc()
  {
    Slab_i **free = &_partial_slabs;
    if (!(*free))
      free = &_empty_slabs;

    if (!(*free) && !grow())
      return 0;

    Slab_i *s = *free;
    Free_o *o = s->free;
    s->free = o->next;

    if (free == &_empty_slabs)
      {
	_empty_slabs = List_item::remove(_empty_slabs, s);
        --_num_free;
      }

    --(s->num_free);

    if (!s->free)
      _full_slabs = List_item::push_front(_full_slabs, s);
    else if (free == &_empty_slabs)
      _partial_slabs = List_item::push_front(_partial_slabs, s);

    //L4::cerr << this << "->alloc(): " << o << ", of " << s << '\n';

    return o;
  }

  void free(void *_o)
  {
    if (!_o)
      return;

    unsigned long addr = (unsigned long)_o;
    addr = (addr / slab_size) * slab_size;
    Slab_i *s = (Slab_i*)addr;

    if (s->cache != this)
      return;

    Free_o *o = reinterpret_cast<Free_o*>(_o);

    o->next = s->free;
    s->free = o;

    bool was_full = false;

    if (!s->num_free)
      {
	_full_slabs = List_item::remove(_full_slabs, s);
	was_full = true;
      }
    
    ++(s->num_free);

    if (s->num_free == objects_per_slab)
      {
	if (!was_full)
	  _partial_slabs = List_item::remove(_partial_slabs, s);
	  
	_empty_slabs = List_item::push_front(_empty_slabs, s);
	++_num_free;
	if (_num_free > max_free_slabs)
	  shrink();
	
	was_full = false;
      }

    if (was_full) 
      _partial_slabs = List_item::push_front(_partial_slabs, s);
    
    //L4::cerr << this << "->free(" << _o << "): of " << s << '\n';
  }

  unsigned total_objects() const { return _num_slabs * objects_per_slab; }
  unsigned free_objects() const
  {
    if (!_empty_slabs)
      return 0;

    return _num_free * objects_per_slab; // XXX forgot partial slabs
  }
    
  
};

template<typename Type, int Slab_size = L4_PAGESIZE,
  int Max_free = 2, template<typename A> class Alloc = New_allocator > 
class Slab : public Base_slab<sizeof(Type), Slab_size, Max_free, Alloc>
{
private:
  typedef Base_slab<sizeof(Type), Slab_size, Max_free, Alloc> Base_type;
public:
  Slab(typename Base_type::Slab_alloc const &alloc 
      = typename Base_type::Slab_alloc())
    : Base_slab<sizeof(Type), Slab_size, Max_free, Alloc>(alloc) {}

  Type *alloc() 
  { 
    return (Type*)Base_slab<sizeof(Type), Slab_size,
      Max_free, Alloc>::alloc(); 
  }
  
  void free(Type *o) 
  { Base_slab<sizeof(Type), Slab_size, Max_free, Alloc>::free(o); }
};

template< int Obj_size, int Slab_size = L4_PAGESIZE,
  int Max_free = 2, template<typename A> class Alloc = New_allocator >
class Base_slab_static
{
private:
  typedef Base_slab<Obj_size, Slab_size, Max_free, Alloc> _A;
  static _A _a;
public:
  enum
  {
    object_size      = Obj_size,
    slab_size        = Slab_size,
    objects_per_slab = _A::objects_per_slab,
    max_free_slabs   = Max_free,
  };
  void *alloc() { return _a.alloc(); }
  void free(void *p) { _a.free(p); }
  unsigned total_objects() const { return _a.total_objects(); }
  unsigned free_objects() const { return _a.free_objects(); }
};

template< int _O, int _S, int _M, template<typename A> class Alloc >
typename Base_slab_static<_O,_S,_M,Alloc>::_A 
  Base_slab_static<_O,_S,_M,Alloc>::_a; 

template<typename Type, int Slab_size = L4_PAGESIZE,
  int Max_free = 2, template<typename A> class Alloc = New_allocator > 
class Slab_static 
: public Base_slab_static<sizeof(Type), Slab_size, Max_free, Alloc>
{
public:
  Type *alloc() 
  { 
    return (Type*)Base_slab_static<sizeof(Type), Slab_size,
      Max_free, Alloc>::alloc(); 
  }
};

};

#endif

