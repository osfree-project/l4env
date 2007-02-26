/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef L4_NH_NET_H__
#define L4_NH_NET_H__

namespace Net 
{

#ifdef BIG_ENDIAN
  template< typename T >
  static T n_to_h( T val )
  {
    return val;
  }
#else
  template< typename T >
  inline static T n_to_h( T val )
  {
    T tmp = 0;
    for( unsigned i=0; i<sizeof(T); i++ )
      tmp |= ((val >> (8*i)) & 0x0ff) << ((sizeof(T)-i-1)*8);
    return tmp;
  }
#endif

  template< typename T >
  inline static T h_to_n( T val )
  {
    return n_to_h(val);
  }

};

#endif //L4_NH_NET_H__

