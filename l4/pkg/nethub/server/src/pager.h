/* -*- c++ -*- */

/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef NH_PAGER_H__
#define NH_PAGER_H__

#include <l4/cxx/thread.h>

class Region;

class Pager : public L4::Thread
{
public:
  Pager();
  void run();

  void add_region( Region *r );
  void del_region( Region *r );

  Region const *find_region( unsigned long a ) const;

private:
  char stack[1024];
  Region *_r[10];
};

#endif  //NH_PAGER_H__
