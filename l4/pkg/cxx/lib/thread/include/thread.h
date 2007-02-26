/* -*- c++ -*- */
/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef CXX_THREAD_H__
#define CXX_THREAD_H__

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>

namespace L4 {
  using ::l4_threadid_t;

  class Thread
  {
  public:

    enum State
    {
      Dead    = 0,
      Running = 1,
      Stopped = 2,
    };

    Thread( bool initiate );
    Thread( void *stack );
    Thread( void *stack, unsigned tid );
    Thread( void *stack, l4_threadid_t id );
    virtual ~Thread();
    void execute() asm ("L4_Thread_execute");
    virtual void run() = 0;
    virtual void shutdown() asm ("L4_Thread_shutdown");
    void start();
    void stop();
    
    l4_threadid_t self() const
    { return _self; }

    State state() const
    { return _state; }
      
    static void set_start_thread( unsigned tno ) 
    { _lthread = tno; }

    static void start_cxx_thread( Thread *_this ) 
      asm ("L4_Thread_start_cxx_thread");

    static void kill_cxx_thread(Thread *_this)
      asm ("L4_Thread_kill_cxx_thread");

    static void set_pager( l4_threadid_t p )
    { _pager = p; }

  private:
    l4_threadid_t _self;
    State _state;

  protected:
    void *_stack;

  private:
    static unsigned _lthread;
    static l4_threadid_t _pager;
    static l4_threadid_t _preempter;
    static l4_threadid_t _master;

  };
  
  inline
  Thread::Thread( bool /*initiate*/ ) : _self(l4_myself()), _state(Running)
  {
    _master = self();
    _lthread = _master.id.lthread + 1;
    l4_umword_t dummy;
    l4_thread_ex_regs(self(), (l4_umword_t)-1,
                      (l4_umword_t)-1,
                      &_preempter, &_pager, &dummy, &dummy, &dummy );
  }

  inline
  Thread::Thread( void *stack ) : _self(_master), _state(Dead), _stack(stack)
  { _self.id.lthread = _lthread++; }

  inline
  Thread::Thread( void *stack, unsigned tid ) 
    : _self(_master), _state(Dead), _stack(stack)
  { _self.id.lthread = tid; }

  inline
  Thread::Thread( void *stack, l4_threadid_t id ) 
    : _self(id), _state(Dead), _stack(stack) 
  {}

};

#endif  /* CXX_THREAD_H__ */

