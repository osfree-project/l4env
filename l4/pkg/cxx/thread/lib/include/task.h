/* -*- c++ -*- */
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
