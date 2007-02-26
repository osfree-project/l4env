#include <l4/syscalls.h>
#include <l4/kdebug.h>


void test_unmap(void)
{
  l4_fpage_t fpage;
  fpage.fpage = 0;
  l4_fpage_unmap(fpage, 0);
}

l4_threadid_t test_my_self(void)
{
  l4_threadid_t myself;
  myself = l4_myself();
  return myself;
}  

void test_switch(void)
{
  l4_threadid_t destination;
  destination = (l4_threadid_t){lh:{-1,-1}};
  l4_thread_switch(destination);
}

void test_nchief(void)
{
  l4_threadid_t destination, next_chief;
  destination = (l4_threadid_t){lh:{-1,-1}}; 
  l4_nchief(destination,  &next_chief);
}

void test_schedule(void)
{
  l4_threadid_t destination, pager, preempter;
  l4_sched_param_t old_param;
  l4_cpu_time_t time;

  destination = (l4_threadid_t){lh:{-1,-1}}; 
  pager = preempter = (l4_threadid_t){lh:{-1,-1}};

  time = l4_thread_schedule(destination, (l4_sched_param_struct_t){0,0,0,0},
			    &preempter, &pager, &old_param);
}

void ex_rex_test(void)
{
  l4_threadid_t destination, pager, preempter;
  l4_umword_t old_eip, old_esp, old_eflags;

  destination = preempter = pager = (l4_threadid_t){lh:{-1,-1}};

  l4_thread_ex_regs(destination, 0, 0, &preempter, &pager,
		    &old_eflags, &old_eip, &old_esp);
}

void test_task_new(void)
{
  static inline l4_taskid_t 
    l4_task_new(l4_taskid_t destination, l4_umword_t mcp_or_new_chief, 
		l4_umword_t esp, l4_umword_t eip, l4_threadid_t pager);
  l4_threadid_t pager;
  l4_taskid_t destination, new_task;

  destination = new_task = (l4_taskid_t){lh:{-1,-1}};
  pager = (l4_threadid_t){lh:{-1,-1}};
  new_task = l4_task_new(destination, 0, 1, 2, pager);
}
  
void main(void)
{
  l4_taskid_t new_task, destination_task;
  l4_threadid_t destination, myself, next_chief, preempter, 
  pager;
  l4_fpage_t base_fpage, fpage;
  l4_sched_param_t old_param;
  l4_umword_t old_esp, old_eip, old_eflags;
  
  /* test unmap fpage */
  base_fpage.fpage = fpage.fpage = 0;
  l4_fpage_unmap(fpage, 0);


  /* test myself */
  myself = l4_myself();
  /* 
   * If I remove the following statement, the compiler will remove the 
   * l4_myself() code due to (over) optimizing :(  ?????
   *
   * FIXED: Seems to be ok, because myself has no side effects, so the compiler
   *        is free to optimize it.
   *        Left in to allow checking.
   *
   * excepter = myself;
   */ 

  /* test switch */
  destination.lh.low = destination.lh.high = 1; 
  /* result = */
  l4_nchief(destination,  &next_chief);

  l4_thread_switch(destination);

  
  /* test schedule */
  destination.lh.low = destination.lh.high = 1; 

  l4_thread_schedule(destination, (l4_sched_param_struct_t){0,0,0,0},
		     &preempter, &pager, &old_param);

  l4_thread_ex_regs(destination, 0, 0, &preempter, &pager,
		    &old_eflags, &old_eip, &old_esp);

  new_task = l4_task_new(destination_task, 0, 1, 2, pager);
}












