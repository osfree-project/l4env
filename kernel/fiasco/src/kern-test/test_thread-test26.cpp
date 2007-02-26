INTERFACE:

//
// Measure thread switch time.
//
// Intra task. Same processor and cross processor.

#include "generic_test_thread.h"

class test_thread_t : public generic_test_thread_t
{
public:
  static const unsigned num_test_threads = 2;
  static volatile unsigned data[0x1000];

  static volatile unsigned flag0, flag1;
  volatile static unsigned  perf_cnt_start,perf_cnt_end;
  volatile static long long tm_26_start, tm_26_end;
};


IMPLEMENTATION[test26]:

#include <stdio.h>
#include <flux/machine/eflags.h>
#include <flux/x86/smp/i82489.h>

#include "globals.h"
//#include "printf.h"
#include "kernel_lock.h"
#include "thread_state.h"
#include "timer.h"
#include "irq.h"
#include "thread_regs.h"
#include "watchdog.h"
#include "node.h"
//#include "back_trace.h"
//#include "cpu_guard.h"

#define NUM_RUNS 16
//#define NUM_RUNS 256

volatile unsigned test_thread_t::data[0x1000] = {};
volatile unsigned test_thread_t::flag0 = 0;
volatile unsigned test_thread_t::flag1 = 0;

volatile unsigned test_thread_t::perf_cnt_start = 0;
volatile unsigned test_thread_t::perf_cnt_end   = 0;

volatile long long test_thread_t::tm_26_start;
volatile long long test_thread_t::tm_26_end;

PUBLIC
test_thread_t::test_thread_t(space_t* space,
			     const l4_threadid_t* id, 
			     int init_prio, unsigned short mcp): 
  generic_test_thread_t(space,id,init_prio,mcp)
{
}


PUBLIC 
unsigned
test_thread_t::get_num()
{
  return num_test_threads;
}

PROTECTED
void
test_thread_t::start_test()
{
  printf("start_test : %08x, initializing test, id: %d\n",
	     this,_thr_id_cnt);
  switch(_thr_id_cnt)
    {
    case 0:  break; 
    case 1:  break;
    default: assert(0);
    }
  printf("start_test : %08x, initializing done, id: %d\n",
	     this,_thr_id_cnt);

  wait_for_init();

  node::clear_exec_cpu();
  printf("start_test : %08x, executing test\n",this);

  switch(_thr_id_cnt)
    {
    case 0: part0(); break;
    case 1: part1(); break;
    default: assert(0);
    }

  printf("start_test : %08x finished\n",this);
  finish();
}


PROTECTED void
test_thread_t::part0()
{
  thread_t *t1 = get_test_thread(1);
  
  while(t1->state() & Thread_running)
    {
      switch_to(t1);
    }

  node::curr_cpu()->init_perf_cnt(0,PERF_CNT_RETIRED_INST,0);
  for(unsigned i=0; i<NUM_RUNS; i++)
    {


      t1->state_add(Thread_running);
      asm __volatile("rdtsc" : "=A"(tm_26_start): :"memory");
      switch_to(t1);
      RDPMC(0,perf_cnt_end);
      printf("cycles for thread switch: %08x\n", (unsigned) (tm_26_end- tm_26_start));
      printf("retired instr: %08x\n", node::curr_cpu()->read_perf_cnt(0));
      printf("diff         : %08x\n", perf_cnt_end- perf_cnt_start);
    }

}

PROTECTED void
test_thread_t::part1()
{
  thread_t *t0 = get_test_thread(0);
  for(;;)
    {
      state_del(Thread_running);
      RDPMC(0,perf_cnt_start);
      switch_to(t0);
      asm __volatile("rdtsc" : "=A"(tm_26_end): :"memory");

    }
}
