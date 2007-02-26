#include <l4/cxx/task.h>
#include <l4/cxx/iostream.h>
#include <l4/cxx/l4iostream.h>

#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/types.h>

namespace L4 {

  void Task::start( Thread *pager, unsigned prio )
  {
    l4_msgdope_t res;
    *(((l4_umword_t*)_stack)--) = (l4_umword_t)this;
    l4_task_new(self(), prio, 
                (l4_umword_t)_stack, 
                (l4_umword_t)start_cxx_thread, 
                pager->self() );

    if(l4_ipc_send(self(), 0, 0, 0, L4_IPC_NEVER, &res )!=0)
      L4::cerr << "ERROR: (master) error while thread handshake: "
               << self() << "\n";
  }

};
