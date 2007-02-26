INTERFACE:

//
// P4 - Load/store tagging examined.
//


#include "generic_test_thread.h"

class test_thread_t : public generic_test_thread_t
{
public:
  static const unsigned num_test_threads = 1;

  volatile static unsigned  perf_cnt_start,perf_cnt_end;
  volatile static unsigned  perf_cnt_start1,perf_cnt_end1;
  volatile static unsigned  cnt0,cnt1;
  volatile static long long tm_37_start, tm_37_end;
  volatile static bool      start_flag;
};


IMPLEMENTATION[test37]:

#include <flux/machine/eflags.h>
#include <flux/x86/smp/i82489.h>
#include <flux/machine/seg.h>


#include "globals.h"
#include "printf.h"
#include "kernel_lock.h"
#include "thread_state.h"
#include "timer.h"
#include "irq.h"
#include "thread_regs.h"
#include "watchdog.h"
#include "node.h"
#include "back_trace.h"
#include "jdb.h"
#include "cpu_guard.h"
#include "kmem_alloc.h"
#include "cpu.h"

//#define NUM_RUNS 16
//#define NUM_RUNS 256
#define NUM_RUNS 8192
//#define NUM_RUNS 16384
//#define NUM_RUNS 262144

//#define DO_TRACE_TEST 1
// #define  DO_DEQUEUE 1

//#define REPLAY_EVENT 1

volatile unsigned test_thread_t::perf_cnt_start  = 0;
volatile unsigned test_thread_t::perf_cnt_end    = 0;
volatile unsigned test_thread_t::perf_cnt_start1 = 0;
volatile unsigned test_thread_t::perf_cnt_end1   = 0;

volatile unsigned test_thread_t::cnt0   = 0;
volatile unsigned test_thread_t::cnt1   = 0;

volatile long long test_thread_t::tm_37_start;
volatile long long test_thread_t::tm_37_end;

volatile bool      test_thread_t::start_flag;

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
    case 0:  init0(); break; 
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
    default: assert(0);
    }

  printf("start_test : %08x finished\n",this);
  finish();
}

PROTECTED void
test_thread_t::init0()
{
  node::curr_cpu()->disable_timer();
}


ia32_ds_area_t ds_area __attribute__((aligned(64)));



PROTECTED void
test_thread_t::part0()
{
  lock_guard_t<cpu_guard_t> guard(&cpu_guard);

  long *pebs_page = reinterpret_cast<long *>(kmem_alloc::page_alloc());

  unsigned d,ia32_dbg_ctrl, ia32_misc_enable, ia32_ds_area;
  volatile unsigned eax,edx,ecx;
  unsigned eax1,edx1;
  volatile int j = 0;
  unsigned iq_cnt0_0 = 0, iq_cnt0_1 = 0;


  printf("pebs buffer=%08x\n",pebs_page);

  for(unsigned i =0; i< 256; i++)
    {
      pebs_page[i] = 0;
    }

  ds_area.pebs_buffer_base         = reinterpret_cast<long>(pebs_page);
  ds_area.pebs_index               = ds_area.pebs_buffer_base;
  ds_area.pebs_absolute_maximum    = ds_area.pebs_buffer_base + 80 * 20 + 1;
  ds_area.pebs_interrupt_threshold = ds_area.pebs_buffer_base + 80 * 15 + 1;
  //  ds_area.pebs_counter_reset       = -20L;
  ds_area.pebs_counter_reset       = -1L;
  //  ds_area.pebs_counter_reset       = 0;

  

  WRMSR(IA32_DS_AREA,&ds_area,0);
  RDMSR(IA32_DS_AREA,eax,edx);
  printf("IA32_DS_AREA %08x:%08x\n",eax,edx);


  dword_t escrmsr; 
  qword_t escr, cccr; 
  unsigned cnt;
#ifdef REPLAY_EVENT
  cnt = current()->exec_cpu()->get_perfctr(&escrmsr, &escr, &cccr, 
					   P4_PERF_REPLAY_EVENT, 16, 0);
#else
  cnt = current()->exec_cpu()->get_perfctr(&escrmsr, &escr, &cccr, 
					   P4_PERF_UOP_TYPE, 15, 0);

   if (cnt != ~0)
    {
      printf("cnt=%d\n",cnt);
      printf("escr=%08x, %08x:%08x\n", escrmsr, escr);
      escr_t *p0 = (escr_t*) &escr;
      //      p0->tag_value = 1;
      printf("sel: %2x, mask: %4x, tval: %1x, ten: %1x, usr/os: %x\n",
	     p0->event_select, p0->event_mask, p0->tag_value, p0->tag_enable,
	     (p0->os << 1) | p0->usr);
      cccr_t *p1 = (cccr_t *) &cccr;
      printf("ovl: %d, cas: %d, pmi: %d, force: %d, edge: %d, thr: %1x,"
	     " compl: %d, cmp: %d, escr: %1x, en: %d\n",
	     p1->ovf, p1->cascade, p1->ovf_pmi, p1->force_ovf, p1->edge, p1->threshold,
	     p1->complement, p1->compare, p1->escr_select, p1->enable);



      WRMSR64(escrmsr, escr);
      WRMSR64(IA32_COUNTER_BASE + cnt, 0);
      WRMSR64(IA32_CCCR_BASE + cnt, cccr);
    }
   else
    {
      printf("Could not allocate counter\n");
      return ;
    }
   cnt = current()->exec_cpu()->get_perfctr(&escrmsr, &escr, &cccr, 
					    P4_PERF_FRONT_EVENT, 16, 0);
#endif


   if (cnt != ~0)
    {
      current()->exec_cpu()->disable_pcint();

      printf("cnt=%d\n",cnt);
      printf("escr=%08x, %08x:%08x\n", escrmsr, escr);
      escr_t *p0 = (escr_t*) &escr;
      printf("sel: %2x, mask: %4x, tval: %1x, ten: %1x, usr/os: %x\n",
	     p0->event_select, p0->event_mask, p0->tag_value, p0->tag_enable,
	     (p0->os << 1) | p0->usr);
      cccr_t *p1 = (cccr_t *) &cccr;
      p1->ovf_pmi   = 0;
      p1->force_ovf = 0;
      printf("ovl: %d, cas: %d, pmi: %d, force: %d, edge: %d, thr: %1x,"
	     " compl: %d, cmp: %d, escr: %1x, en: %d\n",
	     p1->ovf, p1->cascade, p1->ovf_pmi, p1->force_ovf, p1->edge, p1->threshold,
	     p1->complement, p1->compare, p1->escr_select, p1->enable);


      WRMSR64(escrmsr, escr);
      WRMSR64(IA32_COUNTER_BASE + cnt, 0);
      WRMSR64(IA32_COUNTER_BASE + cnt, -40);
      WRMSR64(IA32_CCCR_BASE + cnt, cccr);
    }
   else
    {
      printf("Could not allocate counter\n");
      return ;
    }


   RDMSR(IA32_MISC_ENABLE,eax,edx);
   printf("IA32_MISC_ENABLE %08x:%08x\n",eax,edx);
#ifdef REPLAY_EVENT
   RDMSR(IA32_PEBS_ENABLE,eax,edx);
   eax |= IA32_PEBS_ENABLE_PEBS;
   eax |= IA32_PEBS_UOP_TAG | 1;
   //   eax &= ~IA32_PEBS_UOP_TAG;
   WRMSR(IA32_PEBS_ENABLE,eax,edx);

   WRMSR(IA32_PEBS_MATRIX_VERT,1,0);

#else
   //   WRMSR(IA32_PEBS_ENABLE, IA32_PEBS_ENABLE_PEBS,0);
#endif

#ifdef MSR_DEBUG
   RDMSR(IA32_PEBS_ENABLE, eax, edx);
   printf("IA32_PEBS_ENABLE %08x:%08x\n",eax,edx);
#endif

#if 0
   WRMSR64(IA32_COUNTER_BASE + 16, -3);
   WRMSR64(IA32_COUNTER_BASE + 16, -3L);
#endif
   WRMSR(IA32_COUNTER_BASE + 16, -1, 0x3ff);

   RDMSR(IA32_COUNTER_BASE + 16, eax, edx);

   asm volatile("    leal %3, %%edx       \n"
		"1:                       \n"
		"    nop                  \n"
		"    nop                  \n"
		"    nop                  \n"
		"    nop                  \n"
		"    nop                  \n"
		"    nop                  \n"
		"    nop                  \n"
		"    nop                  \n"
		"    nop                  \n"
		"    nop                  \n"
		"    nop                  \n"
		"    nop                  \n"
		"    nop                  \n"
		"    nop                  \n"
		"    nop                  \n"
		"    nop                  \n"
		"    nop                  \n"
		"    nop                  \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		//		"    wbinvd               \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		//		"    wbinvd               \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		//		"    wbinvd               \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		//		"    wbinvd               \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		//		"    wbinvd               \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		//		"    wbinvd               \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		//		"    wbinvd               \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		//		"    wbinvd               \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		//		"    wbinvd               \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		//		"    wbinvd               \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		//		"    wbinvd               \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		//		"    wbinvd               \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		//		"    wbinvd               \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		//		"    wbinvd               \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		//		"    wbinvd               \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		//		"    wbinvd               \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		//		"    wbinvd               \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		"    movl (%%edx), %%eax  \n"
		//		"    wbinvd               \n"
		"    movl (%%edx), %%eax  \n"
		"    nop                  \n"
		"    nop                  \n"
		"    nop                  \n"
		//		"    loop 1b              \n"
		: "=a"(eax), "=c"(ecx),"=d"(edx)
		: "m"(d),"1"(10));
        

   memory_barrier();
   WRMSR(IA32_CCCR_BASE + 16, 0,0);
   RDMSR(IA32_COUNTER_BASE + 16, eax1, edx1);

   RDMSR(IA32_PEBS_ENABLE,eax,edx);
   eax &= ~IA32_PEBS_ENABLE_PEBS;
   WRMSR(IA32_PEBS_ENABLE,eax,edx);

   printf("cnt16: %08x:%08x, %08x:%08x\n",eax,edx,eax1,edx1);

   RDMSR(IA32_COUNTER_BASE + 16,eax,edx);
   printf("cnt16: %08x:%08x\n",edx,eax);

   RDMSR(IA32_COUNTER_BASE + 12,eax,edx);
   printf("cnt12: %08x:%08x\n",edx,eax);

  jdb_enter_kdebug("done");

  asm volatile ("": :"m"(j));
}


