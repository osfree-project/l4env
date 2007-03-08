/* -*- c++ -*- */
#ifndef MEM_H
#define MEM_H
#include <l4/sys/types.h>
#include <l4/cxx/thread.h>


class PagerThread : public L4::Thread 
{
public:
  PagerThread();
  void run();
};

#endif // MEM_H
