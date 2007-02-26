/* -*- c++ -*- */
/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef L4_CXX_MAIN_THREAD_H__
#define L4_CXX_MAIN_THREAD_H__

#include <l4/cxx/thread.h>

namespace L4 {
  class MainThread : public Thread
  {
  public:
    MainThread() : Thread(true) 
    {}
  };
};

#endif /* L4_CXX_MAIN_THREAD_H__ */
