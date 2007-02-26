#include "pipethread.h"

#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>

#include <stdio.h>
#include <stddef.h>

void *PipeThread::operator new( size_t ) 
{ 
  static unsigned next_free_pt = 0;
  static char pt_mem[sizeof(PipeThread)*10];
  if(next_free_pt<(sizeof(pt_mem)/sizeof(PipeThread)))
    return ((PipeThread*)pt_mem) + (next_free_pt++); 
  else {
    printf("ERROR: Non more pipe-threads available\n");
    return 0;
  }
}

PipeThread::PipeThread( unsigned tid) 
  : Thread( stack + 1023, tid ), _rcv(L4_INVALID_ID), _snd(L4_INVALID_ID)
{
  printf("create worker @%p with stack @%p\n", this, stack +1023 );
}

void PipeThread::run()
{
  printf("hub thread %x.%x running\n", self().id.task, self().id.lthread );
  l4_umword_t d1,d2;
  l4_threadid_t other;
  l4_msgdope_t result, res1;
  struct {
    l4_fpage_t fp;
    l4_msgdope_t size;
    l4_msgdope_t snd;
    l4_umword_t w[2];
    l4_strdope_t packet;
  } msg;

  int err;

  printf("%x.%x wait for _snd=%x.%x _rcv=%x.%x\n", 
         self().id.task, self().id.lthread,_snd.id.task,_snd.id.lthread,
         _rcv.id.task,_rcv.id.lthread );


  while(1) {
    msg.packet.rcv_size = sizeof(msg_buffer);
    msg.packet.rcv_str  = (l4_umword_t)msg_buffer;
    msg.size = L4_IPC_DOPE(2,1);
    msg.snd  = L4_IPC_DOPE(2,1);
    while((err = l4_ipc_receive( _snd, 0, &d1, &d2, L4_IPC_NEVER, &result )))
      printf("error receiving from receiver\n");

    printf("%x.%x: got empty buffer from %x.%x\n",
           self().id.task,self().id.lthread,
           _snd.id.task, _snd.id.lthread );
    printf("  rcv data from %x.%x\n", _rcv.id.task, _rcv.id.lthread );
    while(1) {
      err = l4_ipc_wait( &other, &msg, &d1, &d2, L4_IPC_NEVER, &res1 );
      if(!err && other.id.task == _rcv.id.task) {
        printf("pipe-thread: received %x, %x, len=%d\n",d1,d2,msg.packet.snd_size);
        err = l4_ipc_send( other, 0, 0, 0, L4_IPC_TIMEOUT(0,1,0,1,0,0), &result );
        if(res1.md.strings!=1) {
          printf("error sender sent message with wrong number of indirect strings\n");
        } else {
          msg.packet.snd_str  = (l4_umword_t)msg_buffer;
          msg.size = L4_IPC_DOPE(2,1);
          msg.snd  = L4_IPC_DOPE(2,1);
          err = l4_ipc_send( _snd, &msg, 0, 0, L4_IPC_TIMEOUT(0,1,0,1,0,0), &result );
          break;
        }
      } else {
        printf("error: pipe-thread receive error\n");
      }
    }
  }
}
