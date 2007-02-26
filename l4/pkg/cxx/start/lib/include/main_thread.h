/* -*- c++ -*- */
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
