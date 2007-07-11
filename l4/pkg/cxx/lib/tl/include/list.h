#ifndef CXX_LIST_H__
#define CXX_LIST_H__

#include <l4/cxx/std_tmpl.h>
#include <l4/cxx/std_alloc.h>
#include <l4/cxx/std_ops.h>

namespace cxx {
/*
 * Classes: List_item, List<D, Alloc>
 */

/**
 * Basic list item.
 * Basic item that can be member of a doubly linked, cyclic list.
 */
class List_item 
{
public:
  /**
   * Iterator for a list of ListItem-s.
   * The Iterator iterates til it finds the first element again.
   */
  class Iter
  {
  public:
    Iter(List_item *c, List_item *f) : _c(c), _f(f) {}
    Iter(List_item *f = 0) : _c(f), _f(f) {}
    
    List_item *operator * () const { return _c; }
    List_item *operator -> () const { return _c; }
    Iter &operator ++ () 
    {
      if (!_f)
	_c = 0;
      else
        _c = _c->get_next_item(); 

      if (_c == _f) 
	_c = 0; 
      
      return *this; 
    }
    
    Iter operator ++ (int) 
    { Iter o = *this; operator ++ (); return o; }
      
    Iter &operator -- () 
    {
      if (!_f)
	_c = 0;
      else
        _c = _c->get_prev_item(); 

      if (_c == _f) 
	_c = 0; 
      
      return *this; 
    }
    
    Iter operator -- (int) 
    { Iter o = *this; operator -- (); return o; }

    /** Remove item pointed to by iterator, and return pointer to element. */
    List_item *remove_me()
    {
      if (!_c)
	return 0;

      List_item *l = _c;
      operator ++ ();
      l->remove_me();

      if (_f == l)
	_f = _c;

      return l;
    }

  private:
    List_item *_c, *_f;
  };
  
  /**
   * Iterator for derived classes from ListItem.
   * Allows direct access to derived classes by * operator.
   *
   * Example:
   * class Foo : public ListItem
   * {
   * public:
   *   typedef T_iter<Foo> Iter;
   *   ...
   * };
   */
  template< typename T, bool Poly = false>
  class T_iter : public Iter
  {
  private:
    static bool const P = !Conversion<const T*, const List_item *>::exists 
      || Poly;
    
    static List_item *cast_to_li(T *i, Int_to_type<true>)
    { return dynamic_cast<List_item*>(i); }

    static List_item *cast_to_li(T *i, Int_to_type<false>)
    { return i; }

    static T *cast_to_type(List_item *i, Int_to_type<true>)
    { return dynamic_cast<T*>(i); }

    static T *cast_to_type(List_item *i, Int_to_type<false>)
    { return static_cast<T*>(i); }

  public:

    template< typename O >
    explicit T_iter(T_iter<O> const &o) : Iter(o) { dynamic_cast<T*>(*o); }

    //TIter(CListItem *f) : Iter(f) {}
    T_iter(T *f = 0) : Iter(cast_to_li(f, Int_to_type<P>())) {}
    T_iter(T *c, T *f) 
    : Iter(cast_to_li(c, Int_to_type<P>()),
	cast_to_li(f, Int_to_type<P>())) 
    {}
    
    inline T *operator * () const 
    { return cast_to_type(Iter::operator * (),Int_to_type<P>()); }
    inline T *operator -> () const
    { return operator * (); }
    
    T_iter<T, Poly> operator ++ (int) 
    { T_iter<T, Poly> o = *this; Iter::operator ++ (); return o; }
    T_iter<T, Poly> operator -- (int) 
    { T_iter<T, Poly> o = *this; Iter::operator -- (); return o; }
    T_iter<T, Poly> &operator ++ () { Iter::operator ++ (); return *this; }
    T_iter<T, Poly> &operator -- () { Iter::operator -- (); return *this; }
    inline T *remove_me();
  };

  
  List_item() : _n(this), _p(this) {}
  
protected:
  List_item(List_item const &o) : _n(this), _p(this) {}
  
public:
  /** Get previous item. */
  List_item *get_prev_item() const { return _p; }

  /** Get next item. */
  List_item *get_next_item() const { return _n; }
 
  /** Insert item p before this item. */
  void insert_prev_item(List_item *p)
  {
    p->_p->_n = this;
    List_item *pr = p->_p;
    p->_p = _p;
    _p->_n = p;
    _p = pr;
  }
  
  /** Insert item p after this item. */
  void insert_next_item(List_item *p)
  {
    p->_p->_n = _n;
    p->_p = this;
    _n->_p = p;
    _n = p;
  }

  /** Remove this item from the list. */
  void remove_me()
  {
    if (_p != this)
      {
        _p->_n = _n;
        _n->_p = _p;
      }
    _p = _n = this;
  }

  /** 
   * Append item to a list.
   * Convinience function for empty-head corner case.
   * \param h pointer to the current list head.
   * \param p pointer to new item.
   * \return the pointer to the new head.
   */
  template< typename C, typename N >
  static inline C *push_back(C *head, N *p);

  /** 
   * Prepend item to a list.
   * Convinience function for empty-head corner case.
   * \param head pointer to the current list head.
   * \param p pointer to new item.
   * \return the pointer to the new head.
   */
  template< typename C, typename N >
  static inline C *push_front(C *head, N *p);
  
  /** 
   * Remove item from a list.
   * Convinience function for remove-head corner case.
   * \param head pointer to the current list head.
   * \param p pointer to the item to remove.
   * \return the pointer to the new head.
   */
  template< typename C, typename N >
  static inline C *remove(C *head, N *p);

private:
  List_item *_n, *_p;
};


/* IMPLEMENTATION -----------------------------------------------------------*/
template< typename C, typename N >
C *List_item::push_back(C *h, N *p)
{
  if (!p)
    return h;
  if (!h)
    return p;
  h->insert_prev_item(p);
  return h;
}

template< typename C, typename N >
C *List_item::push_front(C *h, N *p)
{
  if (!p)
    return h;
  if (h)
    h->insert_prev_item(p);
  return p;
}

template< typename C, typename N >
C *List_item::remove(C *h, N *p)
{
  if (!p)
    return h;
  if (!h)
    return 0;
  if (h == p)
    {
      if (p == p->_n)
	h = 0;
      else
        h = static_cast<C*>(p->_n);
    }
  p->remove_me();

  return h;
}

template< typename T, bool Poly >
inline
T *List_item::T_iter<T, Poly>::remove_me()
{ return cast_to_type(Iter::remove_me(), Int_to_type<P>()); }


/**
 * Doubly linked list, with internal allocation.
 * Container for items of type D, implemented by a doubly linked list.
 * Alloc defines the allocator policy.
 */
template< typename D, template<typename A> class Alloc = New_allocator >
class List
{
private:
  class E : public List_item
  {
  public:
    E(D const &d) : data(d) {}
    D data;
  };
  

public:
  class Node : private E
  {};
  
  typedef Alloc<Node> Node_alloc;

  /**
   * Iterator.
   * Forward and backward iteratable.
   */
  class Iter
  {
  private:
    List_item::T_iter<E> _i;

  public:
    Iter(E *e) : _i(e) {}

    D &operator * () const  { return (*_i)->data; }
    D &operator -> () const { return (*_i)->data; }

    Iter operator ++ (int) 
    { Iter o = *this; operator ++ (); return o; }
    Iter operator -- (int) 
    { Iter o = *this; operator -- (); return o; }
    Iter &operator ++ () { ++_i; return *this; }
    Iter &operator -- () { --_i; return *this; }

    /** operator for testing validity (syntactiaclly equal to pointers) */
    operator E* () const { return *_i; }
  };

  List(Alloc<Node> const &a = Alloc<Node>()) : _h(0), _l(0), _a(a) {}

  /** Add element at the end of the list. */
  void push_back(D const &d)
  { 
    void *n = _a.alloc();
    if (!n) return;
    _h = E::push_back(_h, new (n) E(d));
    ++_l;
  }

  /** Add element at the beginning of the list. */
  void push_front(D const &d)
  { 
    void *n = _a.alloc();
    if (!n) return;
    _h = E::push_front(_h, new (n) E(d));
    ++_l;
  }

  /** Remove element pointed to by the iterator. */
  void remove(Iter const &i)
  { E *e = i; _h = E::remove(_h, e); --_l; _a.free(e); }

  /** Get the length of the list. */
  unsigned long size() const { return _l; }
 
  /** Random access. Complexity is O(n). */
  D const &operator [] (unsigned long idx) const
  { Iter i = _h; for (; idx && *i; ++i, --idx); return *i; }

  /** Random access. Complexity is O(n). */
  D &operator [] (unsigned long idx)
  { Iter i = _h; for (; idx && *i; ++i, --idx); return *i; }

  /** Get iterator for the list elements. */
  Iter items() { return Iter(_h); }

private:
  E *_h;
  unsigned _l;
  Alloc<Node> _a;
};


};

#endif

