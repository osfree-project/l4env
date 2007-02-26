#ifndef CXX_LIST_H__
#define CXX_LIST_H__

#include <l4/cxx/std_tmpl.h>

namespace cxx {

class List_item 
{
public:
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
  List_item *get_prev_item() const { return _p; }
  List_item *get_next_item() const { return _n; }
  
  void insert_prev_item(List_item *p)
  {
    p->_p->_n = this;
    List_item *pr = p->_p;
    p->_p = _p;
    _p->_n = p;
    _p = pr;
  }
  
  void insert_next_item(List_item *p)
  {
    p->_p->_n = _n;
    p->_p = this;
    _n->_p = p;
    _n = p;
  }

  void remove_me()
  {
    if (_p != this)
      {
        _p->_n = _n;
        _n->_p = _p;
      }
    _p = _n = this;
  }

  template< typename C, typename N >
  static inline C *push_back(C *head, N *p);

  template< typename C, typename N >
  static inline C *push_front(C *head, N *p);
  
  template< typename C, typename N >
  static inline C *remove(C *head, N *p);

private:
  List_item *_n, *_p;
};

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

};
#endif

