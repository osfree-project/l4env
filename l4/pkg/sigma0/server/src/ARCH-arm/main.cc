#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/l4int.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/kernel.h>

#include <l4/cxx/main_thread.h>
#include <l4/cxx/iostream.h>
#include <l4/cxx/l4iostream.h>

#include "config.h"
#include "memmap.h"

#include <l4/crtx/crt0.h>

#define TASK_MAX (1L << 11)

//bool debug = false; 
unsigned debug = 0;


class Main : public L4::MainThread
{
public:
  void pager();
  void run();
};

Main my_main;
L4::MainThread *main = &my_main;

static Region_instance< 0x40000000, 0x010000, 16 > devs_0;
static Region_instance< 0x80000000, 0x080000, 16 > devs_1;
static Region_instance< 0x90000000, 0x070000, 16 > devs_2;
static Region_instance< 0xA0000000, 0x001000, 12 > devs_3;
static Region_instance< 0xB0000000, 0x020000, 16 > devs_4;

static Memmap memmap;

l4_kernel_info_t *kip;

static void
find_kip(void)
{
  // this is kind of magic, but the microkernel puts the KIP address
  // into r0 and this is store to the global crt0_multiboot_flag in crt0_x.S
  kip = (l4_kernel_info_t*)crt0_multiboot_flag;

  if (kip->magic == L4_KERNEL_INFO_MAGIC)
    L4::cout << "SIGMA0: KIP is at: " << L4::hex << kip << "\n";
  else
    {
      L4::cout << "KIP not found, sleeping...\n";
      while (1);
    }
}

static void
free_region(l4_addr_t start, l4_addr_t end)
{
  l4_addr_t address;

  L4::cout << "SIGMA0 free region:  "
           << L4::hex << ": " << start << ", " << end << "\n";
  for (address = start; address <= end; address += L4_PAGESIZE)
    memmap.free(address);
}

static void
alloc_region(l4_addr_t start, l4_addr_t end, l4_umword_t owner)
{
  l4_addr_t address;
  unsigned grain;

  L4::cout << "SIGMA0 alloc region: "
           << L4::hex << ": " << start << ", " << end << " owner: "
	   << L4::dec << owner << '\n';
  for (address = start; address <= end; address += L4_PAGESIZE)
    if (!memmap.alloc(address, owner, grain))
      L4::cout << "memmap.alloc failed for " << L4::hex << address
               << ' ' << L4::dec << grain << '\n';
      
}

static void 
init_memmap(void)
{

  // Attention at least one Region must be registered 
#if 0
  memmap.add_region( &devs_4 );
  memmap.add_region( &devs_3 );
  memmap.add_region( &devs_2 );
  memmap.add_region( &devs_1 );
#endif
  memmap.add_region( &devs_0 );
  
  /* initialize memory pages to "reserved" and io ports to free */
  memmap.mem_high = Memmap::MEM_MAX; //64*1024*1024;

  l4_kernel_info_mem_desc_t *km = l4_kernel_info_get_mem_descs(kip);
  unsigned nr_memdescs = l4_kernel_info_get_num_mem_descs(kip);
  unsigned i;

  for (i = 0; i < nr_memdescs; i++, km++)
    {
      if (l4_kernel_info_get_mem_desc_is_virtual(km))
        continue;
    
      l4_umword_t start, end, type;
      start = l4_kernel_info_get_mem_desc_start(km);
      end   = l4_kernel_info_get_mem_desc_end(km);
      type  = l4_kernel_info_get_mem_desc_type(km);

      if (type == l4_mem_type_conventional)
	free_region(start, end);
      else if (type == l4_mem_type_reserved)
	{
	  unsigned owner;
	  if (kip->root_eip >=start && kip->root_eip <=end)
	    owner = 4;
	  else
	    owner = 1;
	    
	  alloc_region(start, end, owner);
	}
    }

  return;
}

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

	  // this is always true due to limited range of data types
#if 0
          if (t.id.task > Memmap::O_MAX)
            {
              /* OOPS.. can't map to this sender. */
              d1 = d2 = 0;
            }
          else			/* valid requester */
#endif
            {
              pfa = d1 & 0xfffffffc;

              d1 &= ~2;		/* the L4 kernel seems to get this bit wrong */

              if (d1 == 0xfffffffc)
                {
                  /* map any free page back to sender */
                  memmap.find_free(&d1, &d2, t.id.task);
		  
                  if (d2 != 0)	/* found a free page? */
                    {
		      unsigned gr;
                      memmap.alloc(d1, t.id.task, gr);
                      desc = L4_IPC_SHORT_FPAGE;

                      if (t.id.task < L4_ROOT_TASKNO) /* sender == kernel? */
                        {
                          d2 |= 1; /* kernel wants page granted */
                        }
                    }
                }
              else 
                if (d1 == 1 && (d2 & 0xff) == 1)
                  {
                    /* kernel info page requested */
                    d1 = 0;
                    d2 = l4_fpage((l4_umword_t) kip, 
                                  L4_LOG2_PAGESIZE, 0, 0).raw;
                    desc = L4_IPC_SHORT_FPAGE;
                  }
                else 
                  if (d1 == 1 && (d2 & 0xff) == 0
                      && t.id.task < L4_ROOT_TASKNO)
                    {
                      /* kernel requests recommended kernel-internal RAM
                         size (in number of pages) */
                      d1 = memmap.mem_high / 8 / L4_PAGESIZE;
                    }
                  else if (pfa>=Ram_base && pfa < (Ram_base +0x4000000))
                    {
                      if (debug>2) 
			L4::cout << "/* map a specific page */\n";
                      if ((d1 & 1) && (d2 == (L4_LOG2_SUPERPAGESIZE << 2)))
                        {
                          if (debug>2)
			    L4::cout << "/* superpage request */\n";
                          if (memmap.alloc_superpage(pfa, t.id.task))
                            {
                              if (debug>2)
				L4::cout << "ok\n";
                              d1 &= L4_SUPERPAGEMASK;
                              d2 = l4_fpage(d1, L4_LOG2_SUPERPAGESIZE, 1, 0).raw;
                              desc = L4_IPC_SHORT_FPAGE;

                              /* flush the superpage first so that
                                 contained 4K pages which already have
                                 been mapped to the task don't disturb the
                                 1MB mapping */
                              l4_fpage_unmap(l4_fpage(d1, L4_LOG2_SUPERPAGESIZE,
                                                      0, 0),
                                             L4_FP_FLUSH_PAGE|L4_FP_OTHER_SPACES);

                              goto reply;
                            }
                        }
                      //puts("/* failing a superpage allocation, try a single page allocation */");
		      unsigned gr;
                      if (memmap.alloc(d1, t.id.task, gr))
                        {
  			  if (debug>2)
  			    L4::cout << "ok\n";
                          d1 &= ~((1<<gr) - 1);
                          d2 = l4_fpage(d1, gr, 1, 0).raw;
                          desc = L4_IPC_SHORT_FPAGE;
                        }
#if 0
		      else
		      {
			static l4_umword_t _last_d1;
		        L4::cout << "failed memmap.alloc(" << L4::hex << d1
			         << ", " << t << ", " << gr << ") for "
				 << t << "\n";
                        d1 = d2 = 0;
			//if (_last_d1 == d1)
			//  enter_kdebug("check");
			_last_d1 = d1;
		      }
#endif
#if 0
                      else if (pfa >= (kip->semi_reserved.low & L4_PAGEMASK)
                               && pfa < kip->semi_reserved.high)
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

		      enter_kdebug("guckst du hier");

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
                                         L4_IPC_SEND_TIMEOUT_0, 
                                         /* snd timeout = 0 */
                                         &result);
          
        }
    }
}

void Main::run() 
{
  L4::cout << "SIGMA0[" << self() << "]: Hello World!\n";
  find_kip();
  init_memmap();
  pager();
}
