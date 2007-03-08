#include "test.h"
#include <l4/cxx/thread.h>
#include <l4/cxx/task.h>
#include <l4/cxx/iostream.h>
#include <l4/cxx/l4iostream.h>
#include <l4/cxx/l4types.h>

#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>

using namespace L4;

class ShortIPCTest : public Test
{
public:
  enum {
    ipc_rounds = 10,
  };
        
  ShortIPCTest() : Test("Short IPC") {}
  bool run();
};

static l4_umword_t pong_stack[1024];

class PongThread : public L4::Thread
{
public:
  PongThread() : Thread( pong_stack + 1023 ), pong_result(false) {}
  void run();
  bool pong_result;

  static l4_threadid_t ping_id;
};

l4_threadid_t PongThread::ping_id;

void PongThread::run()
{
  //  l4_threadid_t src;
  l4_msgdope_t res;
  l4_umword_t w1,w2;
  unsigned rounds;
  for(rounds = 0; rounds < ShortIPCTest::ipc_rounds; rounds++)
    {
      //      L4::cout << "[rcv " << ping_id << "] ";
      if(l4_ipc_receive(ping_id, 0, &w1, &w2, 
                        L4_IPC_TIMEOUT(0,0,122,9,0,0), &res) != 0)
        {
          L4::cout << "(pong rcv[" << rounds << "]: " << (MsgDope)res  << ") ";
          return;
        }
      //      L4::cout << "[snd " << ping_id << "] ";
      if(l4_ipc_send(ping_id, 0, w1+1, w2+10, 
                     L4_IPC_TIMEOUT(122,9,0,0,0,0), &res) != 0)
        {
          L4::cout << "(pong snd[" << rounds << "]: " << (MsgDope)res  << ") ";
          return;
        }
    }
  pong_result = true;
  l4_ipc_send(ping_id, 0, 0, 0, 
              L4_IPC_TIMEOUT(122,9,0,0,0,0), &res);
}

bool ShortIPCTest::run()
{
  PongThread::ping_id = l4_myself();
  PongThread pong;
  pong.start();
  unsigned rounds;
  l4_umword_t w1=0, w2=2;
  l4_umword_t rw1, rw2;
  l4_msgdope_t res;
  for(rounds = 0; rounds < ipc_rounds; rounds++)
    {
      //      L4::cout << "[call " << pong.self() << "] ";
      if(l4_ipc_call(pong.self(), 
                     0, w1, w2, 
                     0, &rw1, &rw2,
                     L4_IPC_TIMEOUT(122,9,122,9,0,0), &res) != 0)
        {
          L4::cout << "(ping[" << rounds << "]: " << (MsgDope)res << ") ";
          return false;
        }
      if((rw1!=(w1+1)) || (rw2!=(w2+10)))
        {
          L4::cout << "(ipc error: got [" << rw1 << ", " 
                   << rw2 << "] instead of [" << w1+1 << ", " << w2+10 << "]) ";
          return false;
        }
      w1 = rw2; w2 = rw1;
    }
  
  l4_ipc_receive(pong.self(), 0, &w1, &w2, 
                 L4_IPC_TIMEOUT(122,9,122,9,0,0), &res );
  
  return pong.pong_result;
}


ShortIPCTest t1;
