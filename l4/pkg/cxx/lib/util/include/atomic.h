/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef L4_CXX_ATOMIC_H__
#define L4_CXX_ATOMIC_H__

#include <l4/util/atomic.h>

extern "C" void  ____error_compare_and_swap_does_not_support_3_bytes____();
extern "C" void  ____error_compare_and_swap_does_not_support_more_than_4_bytes____();

namespace L4 
{
  template< typename X >
  inline int compare_and_swap(X volatile *dst, X old_val, X new_val)
  {
    switch (sizeof(X))
      {
      case 1: 
	return l4util_cmpxchg8((l4_uint8_t volatile*)dst, old_val, new_val);
      case 2: 
	return l4util_cmpxchg16((l4_uint16_t volatile *)dst, old_val, new_val);
      case 3: ____error_compare_and_swap_does_not_support_3_bytes____();
      case 4: 
	return l4util_cmpxchg32((l4_uint32_t volatile*)dst, old_val, new_val);
      default:
        ____error_compare_and_swap_does_not_support_more_than_4_bytes____();
      }
    return 0;
  }
};


#endif // L4_CXX_ATOMIC_H__

