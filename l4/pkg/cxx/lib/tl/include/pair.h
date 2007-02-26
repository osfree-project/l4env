#ifndef CXX_PAIR_H__
#define CXX_PAIR_H__

namespace cxx {

template< typename A, typename B >
struct Pair
{
  typedef A F_type;
  typedef B S_type;
  A f;
  B s;
  Pair(A const &f, B const &s) : f(f), s(s) {}
  Pair() {}
};

template< typename Cmp, typename Typ >
struct Pair_compare
{
  Cmp const &_cmp;
  Pair_compare(Cmp const &cmp = Cmp()) : _cmp(cmp) {}

  bool operator () (Typ const &l, Typ const &r) const
  { return _cmp(l.f,r.f); }
};

}

template< typename OS, typename A, typename B >
inline
OS &operator << (OS &os, cxx::Pair<A,B> const &p)
{
  os << p.f << ';' << p.s;
  return os;
}

#endif

