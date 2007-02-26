#ifndef DICE_TESTSERVER_H
#define DICE_TESTSERVER_H

#include <l4/thread/thread.h>
#include <l4/rmgr/librmgr.h>

#define random_CORBA_char_ptr  (const CORBA_char_ptr)"Hello World! This is a test string.\n"

// #define DO_DEBUG

#ifdef DO_DEBUG
#include <l4/log/l4log.h>
#endif

static void pager_thread(void *parameters)
{
  l4_threadid_t src;
  l4_umword_t pfa, eip, snd_base, fp;
  l4_msgdope_t result;
  char c;

  l4thread_started(0);
#ifdef DO_DEBUG
  LOG("pager_thread started\n");
#endif

  while (1)
    {
      l4_i386_ipc_wait(&src, L4_IPC_SHORT_MSG, &pfa, &eip, L4_IPC_NEVER, &result);
#ifdef DO_DEBUG
      LOG("(1) PF from %x.%x at 0x%08x\n", src.id.task, src.id.lthread, eip);
#endif
      while (1)
	{
	  snd_base = pfa & L4_PAGEMASK;
	  fp = snd_base | 0x30;
	  if (pfa & 2)
		{
		  /* touch pf address rw */
		  asm volatile ("orl $0, (%0)" 
						: /* nothing out */
						: "r" (pfa)
						);
		  
		  fp |= 2;
		}
	  else
		{
		  /* touch pf address ro */
		  c = *(volatile char *)pfa;
		}


#ifdef DO_DEBUG
	  LOG("PF reply with snd_base=0x%08x and fp=0x%08x\n",snd_base,fp);
#endif

	  l4_i386_ipc_reply_and_wait(src, L4_IPC_SHORT_FPAGE, snd_base, fp,
	                             &src, L4_IPC_SHORT_MSG, &pfa, &eip,
				     L4_IPC_NEVER, &result);
#ifdef DO_DEBUG
	  LOG("(2) PF from %x.%x at 0x%08x (eip 0x%08x)\n", src.id.task, src.id.lthread, pfa, eip);
#endif
	  if (L4_IPC_IS_ERROR(result))
	    {
#ifdef DO_DEBUG
	      LOG("%s: IPC error (0x%08x)\n", __PRETTY_FUNCTION__,result.msgdope);
#endif
	      break;
	    }
	}
    }
}

unsigned long pager_stack[1024];

l4_threadid_t start_pager_thread(void) __attribute__((unused));

l4_threadid_t start_pager_thread(void)
{
  l4thread_t addr;
#ifdef DO_DEBUG
  LOG("start pager thread (0x%08x)\n", pager_thread);
#endif
  addr = l4thread_create(pager_thread, 0, L4THREAD_CREATE_SYNC);
  return l4thread_l4_id(addr);
}
      
#endif // DICE_TESTSERVER_H

