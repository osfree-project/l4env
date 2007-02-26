#ifndef HX_TL_MINMAX_H__
#define HX_TL_MINMAX_H__

namespace cxx
{
  template< typename T1, typename T2 >
  inline 
  T1 min(T1 a, T2 b) { return a<b?a:b;}
  
  template< typename T1, typename T2 >
  inline 
  T1 max(T1 a, T2 b) { return a>b?a:b;}
};

#endif
 
