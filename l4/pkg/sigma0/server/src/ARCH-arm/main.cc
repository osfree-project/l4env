#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/l4int.h>
#include <l4/sys/kdebug.h>

#include <l4/cxx/main_thread.h>
#include <l4/cxx/iostream.h>
#include <l4/cxx/l4iostream.h>

#include "memmap.h"

#define TASK_MAX (1L << 11)

class Main : public L4::MainThread
{
public:
  void pager();
  void run();
};

Main my_main;
L4::MainThread *main = &my_main;

static Memmap memmap;

static void 
init_memmap(void)
{
  l4_addr_t address;
  
  /* initialize memory pages to "reserved" and io ports to free */
  memmap.mem_high = 64*1024*1024;

  /* free all non-reserved memory: first, free all, then reserve stuff */
  for (address = 0xc0000000; 
       address < 0xc0080000; 
       address += L4_PAGESIZE)
    memmap.free(address);
  
  /* mark root task as free */
  for (address = 0xc00c0000;
       address < 0xc0100000;
       address += L4_PAGESIZE)
    memmap.free(address);

  for (address = 0xc0200000;
       address < 0xc4000000;
       address += L4_PAGESIZE)
    memmap.free(address);

  return;
}

bool debug = false; 
//bool debug = true;

void Main::pager()
{
  l4_umword_t d1, d2, d3, pfa;
  void *desc;
  int err;
  l4_threadid_t t;
  l4_msgdope_t result;

  /* we (the main thread of this task) will serve as a pager for our
     children. */

  /* now start serving the subtasks */
  for (;;)
    {
      err = l4_ipc_wait_w3(&t, 0, &d1, &d2, &d3, L4_IPC_NEVER, &result);

      while (!err)
        {
          /* we received a paging request here */
          /* handle the sigma0 protocol */

          if (debug)
            L4::cout << "SIGMA0: received d1=0x" << L4::hex << d1 
                     << ", d2=0x" << d2 << ", d3=0x" << d3 << L4::dec 
                     << " from thread=" << t << "\n";

          desc = L4_IPC_SHORT_MSG;

          if (t.id.task > Memmap::O_MAX)
            {
              /* OOPS.. can't map to this sender. */
              d1 = d2 = 0;
            }
          else			/* valid requester */
            {
              pfa = d1 & 0xfffffffc;

              d1 &= ~2;		/* the L4 kernel seems to get this bit wrong */

              if (d1 == 0xfffffffc)
                {
                  /* map any free page back to sender */
                  memmap.find_free(&d1, &d2, t.id.task);
		  
                  if (d2 != 0)	/* found a free page? */
                    {
                      memmap.alloc(d1, t.id.task);
                      desc = L4_IPC_SHORT_FPAGE;

                      if (t.id.task < L4_ROOT_TASKNO) /* sender == kernel? */
                        {
                          d2 |= 1; /* kernel wants page granted */
                        }
                    }
                }
              else 
#if 0
                if (d1 == 1 && (d2 & 0xff) == 1)
                  {
                    /* kernel info page requested */
                    d1 = 0;
                    d2 = l4_fpage((l4_umword_t) l4_info, 
                                  L4_LOG2_PAGESIZE, 0, 0).raw;
                    desc = L4_IPC_SHORT_FPAGE;
                  }
                else 
#endif
                  if (d1 == 1 && (d2 & 0xff) == 0
                      && t.id.task < L4_ROOT_TASKNO)
                    {
                      /* kernel requests recommended kernel-internal RAM
                         size (in number of pages) */
                      d1 = memmap.mem_high / 8 / L4_PAGESIZE;
                    }
                  else if (pfa>=0xc0000000 && pfa < 0xc4000000)
                    {
                      //puts("/* map a specific page */");
                      if ((d1 & 1) && (d2 == (L4_LOG2_SUPERPAGESIZE << 2)))
                        {
                          //puts("/* superpage request */");
                          if (memmap.alloc_superpage(pfa, t.id.task))
                            {
                              //puts("ok");
                              d1 &= L4_SUPERPAGEMASK;
                              d2 = l4_fpage(d1, L4_LOG2_SUPERPAGESIZE, 1, 0).raw;
                              desc = L4_IPC_SHORT_FPAGE;

                              /* flush the superpage first so that
                                 contained 4K pages which already have
                                 been mapped to the task don't disturb the
                                 4MB mapping */
                              l4_fpage_unmap(l4_fpage(d1, L4_LOG2_SUPERPAGESIZE,
                                                      0, 0),
                                             L4_FP_FLUSH_PAGE|L4_FP_OTHER_SPACES);

                              goto reply;
                            }
                        }
                      //puts("/* failing a superpage allocation, try a single page allocation */");
                      if (memmap.alloc(d1, t.id.task))
                        {
                          //puts("ok");
                          d1 &= L4_PAGEMASK;
                          d2 = l4_fpage(d1, L4_LOG2_PAGESIZE, 1, 0).raw;
                          desc = L4_IPC_SHORT_FPAGE;
                        }
#if 0
                      else if (pfa >= (l4_info->semi_reserved.low & L4_PAGEMASK)
                               && pfa < l4_info->semi_reserved.high)
                        {
                          /* adapter area, page faults are OK */
                          d1 &= L4_PAGEMASK;
                          d2 = l4_fpage(d1, L4_LOG2_PAGESIZE, 1, 0).raw;
                          desc = L4_IPC_SHORT_FPAGE;
                        }
#endif
                    }
                  else 
                    {
                      L4::cout << "SIGMA0: can't handle d1=0x" << L4::hex << d1
                               << ", d2=0x" << d2 << ", d3=0x" << d3 << L4::dec
                               << " from thread=" << t << "\n";

                      /* unknown request */
                      d1 = d2 = 0;
                    }
            }

        reply:
          if (debug)
            L4::cout << "SIGMA0: sending d1=0x" << L4::hex << d1 
                     << ", d2=0x" << d2 << ", d3=0x" << d3 << L4::dec
                     << " to thread=" << t << "\n";

          /* send reply and wait for next message */
          err = l4_ipc_reply_and_wait_w3(t, desc, d1, d2, d3, 
                                         &t, 0, &d1, &d2, &d3,
                                         L4_IPC_TIMEOUT(0,1,0,0,0,0), 
                                         /* snd timeout = 0 */
                                         &result);
          
        }
    }
}

void Main::run() 
{
  L4::cout << "SIGMA0[" << self() << "]: Hello World!\n";
  init_memmap();
  pager();
}
