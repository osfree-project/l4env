INTERFACE:

#include "thread.h"

class generic_test_thread_t : public thread_t 
{
protected:
  virtual void      start_test() = 0;
  virtual unsigned  get_num()    = 0;
  unsigned _thr_id_cnt;

private:
  static unsigned           _id_cnt;
  static volatile unsigned  _spin_cnt;
  static volatile unsigned  _finish_cnt;
};

IMPLEMENTATION:

#include <stdio.h>
#include <assert.h>

#include "context.h"
#include "atomic.h"
#include "lock_guard.h"
#include "thread_state.h"
#include "globals.h"
#include "kernel_lock.h"
#include "timer.h"
#include "node.h"

unsigned           generic_test_thread_t::_id_cnt      = 0;
volatile unsigned  generic_test_thread_t::_spin_cnt    = 0;
volatile unsigned  generic_test_thread_t::_finish_cnt  = 0;


PROTECTED
generic_test_thread_t::generic_test_thread_t(space_t* space,
			     const l4_threadid_t* id, 
			     int init_prio, unsigned short mcp,
			     unsigned init_func = 0): 
  thread_t(space,id,init_prio,mcp)
{
  unsigned user_invoke = reinterpret_cast<unsigned>(context_t::user_invoke);
    //           run_test    = reinterpret_cast<unsigned>(generic_test_thread_t::run_test);
  if(init_func == 0)
    {
      init_func = reinterpret_cast<unsigned>(generic_test_thread_t::run_test);
    } 

  compare_and_swap(reinterpret_cast<unsigned *>(kernel_sp), 
		   user_invoke,
		   init_func);

  unsigned old_cnt, new_cnt;
  do 
    {
      old_cnt = _id_cnt;
      new_cnt = old_cnt + 1;
    }
  while(!compare_and_swap((unsigned *) &_id_cnt, old_cnt ,new_cnt));

  _thr_id_cnt = old_cnt;

}

PROTECTED static void
generic_test_thread_t::run_test()
{
  ((generic_test_thread_t *)current_thread())->start_test();
}


PROTECTED void
generic_test_thread_t::finish(unsigned number = 1)
{  
  unsigned old_cnt, new_cnt;
  do {
    old_cnt = _finish_cnt;
    new_cnt = old_cnt + number;
  } while(!compare_and_swap((unsigned*) &_finish_cnt,old_cnt, new_cnt));

  //  assertv(lock_cnt() == 0, this,lock_cnt(), state());

  if(_finish_cnt < get_num())
    {
      state_del(Thread_running);
      printf("Finished - scheduling\n");

      schedule();

      node::lock_exec_cpu();
      switch_to(node::get_idle_context());
      node::clear_exec_cpu();
      assert(0);
    }
  else
    {
      node::lock_exec_cpu();

      kernel_lock.test_and_set();

      print_summary();

      printf("Test finally finished, all threads returned\n");
      printf("Grabbing global lock and halt forever......\n");

      // wait for the watchdog
      for(;;)
	asm volatile("hlt");
    }
  assert(0);
}


PROTECTED virtual void
generic_test_thread_t::print_summary()
{
  //  smp_printf("test completed\n");
}

PROTECTED void
generic_test_thread_t::wait_for_init()
{
  unsigned old_cnt, new_cnt;
  do 
    {
      printf("wait for init: current spin_cnt: %d\n",_spin_cnt);
      old_cnt = _spin_cnt;
      new_cnt = old_cnt + 1;
    }
  while (!compare_and_swap((unsigned *)&_spin_cnt, old_cnt, new_cnt));

  printf("wait for init %p: %d from %d\n", 
	 this, new_cnt, get_num());

  if ( !in_ready_list())
    {
      ready_enqueue();
    }

  //  while(_spin_cnt < _id_cnt)
  while(_spin_cnt < get_num())
    {
      // allow interrupts , e.g. from the global lock
      node::clear_exec_cpu();
      //      schedule();

      // changed the ready queue
      if (ready_next != this)
	{
	  //	  assertv(context_t::valid_context(_ready_next),_ready_next,this);
	  switch_to(ready_next);
	}
      node::lock_exec_cpu();
      // have the idle thread init the other test threads
      if(! (_spin_cnt < get_num()))
	break;
      switch_to(node::get_idle_context());
    }
  printf("all test threads ready, id: %08x\n",id().id.lthread);
}

PROTECTED thread_t*
generic_test_thread_t::get_test_thread(unsigned thr_nr)
{
  l4_threadid_t thr_id = id();
  thr_id.id.lthread = thr_nr;
  return threadid_t(&thr_id).lookup();
}

PROTECTED void
generic_test_thread_t::set_timeout(unsigned delay,timeout_t *timeout)
{
  check(state_add(Thread_ipc_in_progress));
  timeout->set(kmem::info()->clock + delay);

  // state_change_safely might fail, if the timer went off before
  state_change_safely(~(Thread_ipc_in_progress| 
			Thread_running),
		      Thread_ipc_in_progress);
  asm __volatile__("hlt");
  schedule();
  assert(state() & Thread_running);
}

PROTECTED void
generic_test_thread_t::spin_busy(unsigned cycles)
{
  long long end_tm, curr_tm;
  unsigned tm0l, tm0h;

  asm __volatile__("rdtsc": "=a"(tm0l), "=d"(tm0h));
  end_tm = (((long long) tm0h) << 32) | tm0l;

  end_tm += cycles;

  do 
    {
      asm __volatile__("rdtsc": "=a"(tm0l), "=d"(tm0h));
      curr_tm = (((long long) tm0h) << 32) | tm0l;
    }
  while(curr_tm < end_tm);
  
}
