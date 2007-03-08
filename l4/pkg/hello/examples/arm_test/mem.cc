#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/cxx/base.h>

#include <l4/cxx/iostream.h>
#include <l4/cxx/l4iostream.h>

#include "globals.h"
#include "mem.h"

int use_superpages;
l4_mword_t bad_pagefault;

static unsigned char pager_stack[STACKSIZE] __attribute__((aligned(4096)));

PagerThread::PagerThread() 
  : Thread( pager_stack + STACKSIZE - 4, PAGER_THREAD )
{}

void PagerThread::run()
{
  l4_threadid_t src;
  l4_umword_t pfa,pc,snd_base,fp;
  l4_msgdope_t result;
  extern char _stext;
  extern char _etext;
  extern char _end;

  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  for(;;)
    {
      l4_ipc_wait(&src,L4_IPC_SHORT_MSG,&pfa,&pc,L4_IPC_NEVER,&result);
      for(;;)
	{
#if DO_DEBUG
          L4::cout << "pager: recvd: pfa=" << L4::hex << pfa << " pc=" << pc 
                   << " from thread " << src << "\n";
#endif
	  int skip_send = 0;

          snd_base = pfa & L4_PAGEMASK;
          fp = snd_base | (L4_LOG2_PAGESIZE << 2);
	  
	  /* sanity check */
      	  if ((pfa < (unsigned)&_stext 
               || pfa > (unsigned)&_end))
            {
              bad_pagefault = pfa;
              skip_send = 1;
            }
	  if (pfa & 2)
    	    fp |= 2;
          
	  if (skip_send)
	    {
	      // don't answer the client to prevent endless pingpong
	      l4_ipc_wait(&src,L4_IPC_SHORT_MSG,&pfa,&pc,
                          L4_IPC_NEVER,&result);
	    }
	  else
	    {
#if DO_DEBUG
              L4::cout << "pager: send: d1=" << L4::hex << snd_base << " fp=" << fp 
                       << " to thread " << src << "\n";
#endif
	      l4_ipc_reply_and_wait(src,L4_IPC_SHORT_FPAGE,snd_base,fp,
                                    &src,L4_IPC_SHORT_MSG,&pfa,&pc,
                                    L4_IPC_NEVER,&result);
	    }
	  if (L4_IPC_IS_ERROR(result))
	    {
              L4::cout << "pager: IPC error (dope=0x" << L4::hex << result.msgdope 
                       << ") serving pfa=" << pfa << " pc=" << pc << " from " << src << "\n";
	      break;
	    }
	}
    }
}
