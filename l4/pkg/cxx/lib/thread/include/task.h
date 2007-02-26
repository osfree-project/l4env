/* -*- c++ -*- */
/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef CXX_TASK_H__
#define CXX_TASK_H__

#include <l4/cxx/thread.h>
#include <l4/sys/types.h>

namespace L4 {
  class Task : public Thread
  {
  public:
    Task( void *stack, unsigned id );
    void start( Thread *pager, unsigned prio );
    void kill();
  public:
    static l4_threadid_t task_from_id( unsigned id );
  };

  inline
  l4_threadid_t Task::task_from_id(unsigned id)
  {
    l4_threadid_t i; 
    i.raw = 0;
    i.id.task = id;
    return i;
  }

  inline
  Task::Task( void *stack, unsigned id )
    : Thread( stack, task_from_id(id) )
  {}

};

#endif /* CXX_TASK_H__ */
