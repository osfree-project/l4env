#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>

#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/util/util.h>

#include <l4/cxx/main_thread.h>
#include "pipethread.h"

#include <stddef.h>
#include <stdio.h>

char LOG_tag[9]=LOG_TAG;

class Main : public L4::MainThread
{
 public:
  void run();
};

Main my_main;
L4::MainThread *main = &my_main;

struct Hub {
  PipeThread *ms,*sm;
  Hub() : ms(0), sm(0) {}
  bool valid() { return ms && sm; }
};

static Hub hubs[5];

static unsigned next_thread = 10;

void Main::run()
{
  l4_threadid_t th, thtmp;
  l4_msgdope_t result;
  l4_umword_t d1, d2;
  int err;
  unsigned port, ths, thr;
  printf("L4PointToPoint Network Hub\n");
  names_register("l4/nethub");
  while(1) {
    err = l4_ipc_wait( &th, 0, &d1, &d2, L4_IPC_NEVER, &result );
    while(!err) {
      printf("got ctl msg: cmd=%d, val=%d, from %x.%x\n", 
             d1 & 0xffff,d2, th.id.task, th.id.lthread);
      port = d2 & 0x0ff;
      ths  = (d2 >> 8) & 0x0ff;
      thr  = (d2 >> 16) & 0x0ff;
      switch(d1 & 0xff) {
      case 0: /* open slave hub */
        printf("open slave hub [port=%d] from %x.%x\n", port, th.id.task, th.id.lthread);
        if(port>=(sizeof(hubs)/sizeof(Hub))) {
          printf("non existing hub requested: %d (0-%d allowed)\n",
                 port, (sizeof(hubs)/sizeof(Hub)));
          d1 = 0;
          break;
        }
        if(!hubs[port].valid()) {
          printf("non existent slave hub requested %d\n", port);
          d1 = 0;
          break;
        }
        d2 = (hubs[port].sm->self().id.lthread & 0x0ff)
          |  ((hubs[port].ms->self().id.lthread & 0x0ff) << 8);
        
        thtmp = th;
        thtmp.id.lthread = ths;
        hubs[port].ms->snd(thtmp);
        thtmp.id.lthread = thr;
        hubs[port].sm->rcv(thtmp);
        hubs[port].ms->start();
        hubs[port].sm->start();

        d1 = 0xaffedead;
        break;
      case 1:/* open master hub */
        printf("open master hub [port=%d] from %x.%x\n", port, th.id.task, th.id.lthread);
        if(port>=(sizeof(hubs)/sizeof(Hub))) {
          printf("non existing hub requested: %d (0-%d allowed)\n",
                 port, (sizeof(hubs)/sizeof(Hub)));
          d1 = 0;
          break;
        }
        if(hubs[port].valid()) {
          printf("existent master hub requested %d\n", port);
          d1 = 0;
          break;
        }
        hubs[port].sm = new PipeThread( next_thread++ );
        hubs[port].ms = new PipeThread( next_thread++ );
        thtmp = th;
        thtmp.id.lthread = ths;
        hubs[port].sm->snd(thtmp);
        thtmp.id.lthread = thr;
        hubs[port].ms->rcv(thtmp);
        d2 = (hubs[port].ms->self().id.lthread & 0x0ff)
          |  ((hubs[port].sm->self().id.lthread & 0x0ff) << 8);

        d1 = 0xaffedead;
        break;
      default:
        d1 = 0;
        break;
      }

      err = l4_ipc_reply_and_wait( th, 0, d1, d2, 
                                   &th, 0, &d1, &d2,
                                   L4_IPC_TIMEOUT(0,1,0,0,0,0), 
                                   &result );
    }
  }
}

