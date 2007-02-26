/* -*- c++ -*- */
#ifndef CXX_THREAD_H__
#define CXX_THREAD_H__

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>

namespace L4 {
  using ::l4_threadid_t;

  class Thread
  {
  public:
    Thread( bool initiate );
    Thread( void *stack );
    Thread( void *stack, unsigned tid );
    Thread( void *stack, l4_threadid_t id );
    virtual ~Thread();
    void execute() asm ("L4_Thread_execute");
    virtual void run() = 0;
    virtual void shutdown();
    void start();
    l4_threadid_t self() { return _self; }

    static void set_start_thread( unsigned tno ) 
    { _lthread = tno; }

    static void start_cxx_thread( Thread *_this ) 
      asm ("L4_Thread_start_cxx_thread");

    static void kill_cxx_thread();

  private:
    l4_threadid_t _self;

  protected:
    void *_stack;

  private:
    static unsigned _lthread;
    static l4_threadid_t _pager;
    static l4_threadid_t _preempter;
    static l4_threadid_t _master;

  };


  inline
  Thread::Thread( bool /*initiate*/ ) : _self(l4_myself())
  {
    _master = self();
    _lthread = _master.id.lthread + 1;
    l4_umword_t dummy;
    l4_thread_ex_regs(self(), (l4_umword_t)-1,
                      (l4_umword_t)-1,
                      &_preempter, &_pager, &dummy, &dummy, &dummy );
  }

  inline
  Thread::Thread( void *stack ) : _self(_master), _stack( stack )
  {
    _self.id.lthread = _lthread++;
  }

  inline
  Thread::Thread( void *stack, unsigned tid ) : _self(_master), _stack( stack )
  {
    _self.id.lthread = tid;
  }

  inline
  Thread::Thread( void *stack, l4_threadid_t id ) 
    : _self(id), _stack(stack) 
  {}

};





#endif  /* CXX_THREAD_H__ */
