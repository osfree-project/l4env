/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/cxx/thread.h>

namespace L4 {

  void Thread::start_cxx_thread(Thread *_this)
  { _this->execute(); }

  void Thread::kill_cxx_thread(Thread *_this)
  { _this->shutdown(); }

};

