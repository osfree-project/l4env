
IMPLEMENTATION[test]:


#include "test_thread.h"
#include "node.h"

PROTECTED  void
kernel_thread_t::init_workload()
{
  current()->state_add(Thread_running);
  printf("initializing test, cpu status : %d\n", node::test_exec_cpu());
  printf("current: %08x(%08x)\n",current(),current()->state());
  space_t *test_space = new space_t(config::test_id0.id.task);

  for(unsigned i=0; i < test_thread_t::num_test_threads; i ++)
    {
      printf("init_workload : creating test thread id: %d\n",i);
      l4_threadid_t thread_id = config::test_id0;
      thread_id.id.lthread = i;
      thread_t *test_thread = new (&thread_id) test_thread_t(test_space,
							     &thread_id,
							     config::test_prio, 
							     config::test_mcp);
      printf("initializing  ....");

      test_thread->initialize(kmem::info()->root_eip,
			      kmem::info()->root_esp,
			      sigma0_thread, 0);
      printf("done\n");
    }
  
  printf("test initialized\n");
}

