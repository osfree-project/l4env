#ifndef HX_STD_TMPL_H__
#define HX_STD_TMPL_H__

namespace cxx {
template< int I >
class Int_to_type
{
public:
  enum { i = I };
};

template< typename T, typename U >
class Conversion
{
private:
  typedef char Small;
  class Big { char dummy[2]; };

  static Small test(U);
  static Big test(...);
  static T make_t();
public:
  enum { exists = sizeof(test(make_t())) == sizeof(Small) };
  enum { exists_2_way = exists && Conversion<U,T>::exists };
  enum { same_type = false };
};

template< typename T >
class Conversion<T, T>
{
public:
  enum { exitsis = 1, exists_2_way = 1, same_type = true };
};

};

#endif

