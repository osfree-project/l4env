#ifndef STD_OPS_H__
#define STD_OPS_H__

namespace cxx {

template< typename Obj >
struct Lt_functor
{
  bool operator () (Obj const &l, Obj const &r) const
  { return l < r; }
};

};

#endif

