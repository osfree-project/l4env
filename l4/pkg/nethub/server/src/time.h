/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */
#ifndef L4_NH_TIME_H__
#define L4_NH_TIME_H__

#include <l4/sys/kernel.h>
#include "types.h"

class Time
{
public:
  static void init();
  static inline u64 get_time();

private:

  static l4_kernel_info_t *kip;

};


u64 Time::get_time()
{
  return *(u64 volatile *)(&kip->clock);
}


#endif //  L4_NH_TIME_H__


