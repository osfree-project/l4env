#include <l4/sys/l4int.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>

#include <l4/cxx/iostream.h>
#include <l4/cxx/l4iostream.h>

#include <l4/cxx/base.h>

#include <l4/cxx/main_thread.h>
#include <l4/cxx/thread.h>
#include <l4/cxx/task.h>
#include <l4/cxx/l4types.h>

#include "test.h"
#include "mem.h"

class MyThread : public L4::Thread
{
public:
  MyThread();
  void run();
};

class MyTask : public L4::Task
{
public:
  MyTask( unsigned id );
  void run();
};

class Main : public L4::MainThread
{
public:
  void run();
};


Main my_main;
L4::MainThread *main = &my_main;

l4_umword_t stack[1024];
l4_umword_t stack2[1024] __attribute__((aligned(4096)));

MyThread::MyThread() : Thread( stack + 1020 ) 
{}

MyTask::MyTask( unsigned id ) : Task( stack2 + 1020, id ) 
{}

void Main::run()
{

  l4_threadid_t sigma0;
  sigma0.raw = 0;
  sigma0.id.task = 2;
  sigma0.id.lthread = 0;

  l4_msgdope_t result;
  l4_umword_t d1, d2, d3;

  L4::cout << "TEST[" << self() << "]: Hello world\n";

  enter_kdebug();  

#if 0
  L4::cout << "TEST[" << self() << "]: doing swi\n";

  asm ("swi 0x10\n");
#endif

  L4::cout << "TEST[" << self() << "]: Requesting arbitrary memory"
              " from Sigma0\n";

  l4_ipc_call(sigma0, 0, 0xfffffffc, 0, L4_IPC_MAPMSG(0x50000, 12),
              &d1, &d2, L4_IPC_NEVER, &result);

  L4::cout << "TEST[" << self() << "]: Got back: " 
           << L4::MsgDope(result) << "\n";

  Test::run_tests();
  
  MyThread nt;
  L4::cout << "TEST[" << self() << "]: Start thread " << nt.self() << "\n";
  nt.start();

  L4::cout << "TEST[" << self() << "]: send short ipc to " << nt.self() << "\n";

  if(l4_ipc_send_w3(nt.self(), 0, 0x10, 0x45, 0x78, L4_IPC_NEVER, &result)==0) {
    L4::cout << "  SND: success\n";
  } else {
    L4::cout << "  SND: error: " << L4::dec << result.md.error_code << "\n";
  }


  l4_threadid_t irq;
  
  L4::cout << "TEST[" << self() << "]: try attach to IRQ 10\n";
  
  l4_make_taskid_from_irq(10, &irq);
  l4_ipc_receive_w3(irq,0,&d1,&d2,&d3, L4_IPC_BOTH_TIMEOUT_0, &result);
  if (L4_IPC_ERROR(result) == L4_IPC_RETIMEOUT)
    L4::cout << "TEST[" << self() << "]: OK\n";
  else
    L4::cout << "TEST[" << self() << "]: FAILED (" << L4::MsgDope(result) 
             << ")\n";
  
  L4::cout << "TEST[" << self() << "]: try attach to IRQ 11\n";
  
  l4_make_taskid_from_irq(11, &irq);
  l4_ipc_receive_w3(irq,0,&d1,&d2,&d3, L4_IPC_BOTH_TIMEOUT_0, &result);
  if (L4_IPC_ERROR(result) == L4_IPC_RETIMEOUT)
    L4::cout << "TEST[" << self() << "]: OK\n";
  else
    L4::cout << "TEST[" << self() << "]: FAILED (" << L4::MsgDope(result) 
             << ")\n";

  L4::cout << "TEST[" << self() << "]: disassociate from irq 11\n";
  l4_ipc_send_w3(irq, 0, 1, 0, 0, L4_IPC_BOTH_TIMEOUT_0, &result);
  if (L4_IPC_ERROR(result))
    L4::cout << "TEST[" << self() << "]: FAILED (" << L4::MsgDope(result)
             << ")\n";
  else
    L4::cout << "TEST[" << self() << "]: OK\n";

  PagerThread pt;
  pt.start();

  L4::cout << "TEST[" << self() << "]: Try task_new\n";

  MyTask task(self().id.task +1);
  task.start( &pt, 30 );

  if(l4_ipc_send_w3(task.self(), 0, 0x11, 0x45, 0x78, L4_IPC_NEVER, &result)==0) {
    L4::cout << "  SND: success";
  } else {
    L4::cout << "  SND: error: " << L4::dec << result.md.error_code << "\n";
  }
  enter_kdebug();

}

void MyThread::run()
{
  l4_umword_t d1,d2,d3;
  l4_threadid_t other;
  l4_msgdope_t result;

  other = self();
  other.id.lthread=0;

  L4::cout << "TEST[" << self() << "]: waiting for ipc from " 
           << other << "\n";

  if(l4_ipc_receive_w3(other,0,&d1,&d2,&d3, L4_IPC_NEVER, &result)==0)
    L4::cout << "  RCV: success: " << L4::hex << d1 << ", " 
             << d2 << ", " << d3 << "\n";
  else
    L4::cout << "  RCV: error: " << L4::dec << result.md.error_code << "\n";
}

void MyTask::run()
{
  l4_umword_t d1,d2,d3;
  d1 = 125;
  l4_msgdope_t result;
  l4_threadid_t other;
  other=self();
  other.id.lthread=0;
  other.id.task=4;
  L4::cout << "TEST[" << self() << "]: waiting for ipc from " << other << "\n";
  //L4::cout << "TEST: " << d1 << "\n";

  if(l4_ipc_receive_w3(other,0,&d1,&d2,&d3, L4_IPC_NEVER, &result)==0)
    L4::cout << "  RCV: success: " << L4::hex << d1 << ", " << d2 << ", " << d3 << "\n";
  else
    L4::cout << "  RCV: error: " << L4::dec << result.md.error_code << "\n";
}
