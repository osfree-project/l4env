/* -*- c++ -*- */
/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef L4_CXX_STRING_H__
#define L4_CXX_STRING_H__

#include <l4/cxx/iostream.h>

namespace L4 {

  class String
  {
  public:
    String( char const *str = "" ) : _str(str) 
    {}

    unsigned length() const 
    { unsigned l; for( l=0; _str[l] ; l++); return l; }

    char const *p_str() const { return _str; }

  private:
    char const *_str;
  };

};

inline
L4::BasicOStream &operator << (L4::BasicOStream &o, L4::String const &s)
{
  o << s.p_str();
  return o;
}

#endif /* L4_CXX_STRING_H__ */
