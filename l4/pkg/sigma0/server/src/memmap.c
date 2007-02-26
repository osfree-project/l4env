#include <stdio.h>
#include <unistd.h>

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>

#include "globals.h"
#include "config.h"

#include "memmap.h"

owner_t __memmap[MEM_MAX/L4_PAGESIZE];
__superpage_t __memmap4mb[SUPERPAGE_MAX];
owner_t __iomap[IO_MAX];

vm_offset_t mem_high;

l4_kernel_info_t *l4_info;

static void find_free(l4_umword_t *d1, l4_umword_t *d2, owner_t owner);

void
pager(void)
{
  l4_umword_t d1, d2, pfa;
  void *desc;
  int err;
  l4_threadid_t t;
  l4_msgdope_t result;

  /* we (the main thread of this task) will serve as a pager for our
     children. */

  /* now start serving the subtasks */
  for (;;)
    {
      err = l4_i386_ipc_wait(&t, 0, &d1, &d2, L4_IPC_NEVER, &result);

      while (!err)
	{
	  /* we received a paging request here */
	  /* handle the sigma0 protocol */
	  
	  if (debug)
	    printf("SIGMA0: received d1=0x%x, d2=0x%x "
		   "from thread=%x\n", d1, d2, t.lh.low);

	  desc = L4_IPC_SHORT_MSG;

	  if (t.id.task > O_MAX)
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
		  find_free(&d1, &d2, t.id.task);
		  
		  if (d2 != 0)	/* found a free page? */
		    {
		      memmap_alloc_page(d1, t.id.task);
		      desc = L4_IPC_SHORT_FPAGE;

		      if (t.id.task < ROOT_TASKNO) /* sender == kernel? */
			{
			  d2 |= 1; /* kernel wants page granted */
			  if (jochen)
			    d2 &= ~2; /* work around bug in Jochen's L4 */
			}
		    }
		}
	      else if (d1 == 1 && (d2 & 0xff) == 1)
		{
		  /* kernel info page requested */
		  d1 = 0;
		  d2 = l4_fpage((l4_umword_t) l4_info, 
				L4_LOG2_PAGESIZE, 0, 0).fpage;
		  desc = L4_IPC_SHORT_FPAGE;
		}
	      else if (d1 == 1 && (d2 & 0xff) == 0
		       && t.id.task < ROOT_TASKNO)
		{
		  /* kernel requests recommended kernel-internal RAM
                     size (in number of pages) */
		  d1 = mem_high / 8 / L4_PAGESIZE;
		}
	      else if (pfa < 0x40000000)
		{
		  /* map a specific page */
		  if ((d1 & 1) && (d2 == (L4_LOG2_SUPERPAGESIZE << 2)))
		    {
		      /* superpage request */
		      if (memmap_alloc_superpage(pfa, t.id.task))
			{
			  d1 &= L4_SUPERPAGEMASK;
			  d2 = l4_fpage(d1, L4_LOG2_SUPERPAGESIZE, 1, 0).fpage;
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
		  /* failing a superpage allocation, try a single page
                     allocation */
		  if (memmap_alloc_page(d1, t.id.task))
		    {
		      d1 &= L4_PAGEMASK;
		      d2 = l4_fpage(d1, L4_LOG2_PAGESIZE, 1, 0).fpage;
		      desc = L4_IPC_SHORT_FPAGE;
		    }
		  else if (pfa >= (l4_info->semi_reserved.low & L4_PAGEMASK)
			   && pfa < l4_info->semi_reserved.high)
		    {
		      /* adapter area, page faults are OK */
		      d1 &= L4_PAGEMASK;
		      d2 = l4_fpage(d1, L4_LOG2_PAGESIZE, 1, 0).fpage;
		      desc = L4_IPC_SHORT_FPAGE;
		    }
		}
	      else if (pfa >= 0x40000000 && pfa < 0xC0000000 && !(d1 & 1))
		{
		  /* request physical 4 MB area at pfa + 0x40000000 */

		  pfa = (pfa & L4_SUPERPAGEMASK);

		  if (memmap_alloc_superpage(pfa + 0x40000000, t.id.task))
		    {
		      /* map the superpage to the subtask */

		      d1 = 0;

		      /* Not only does the Sigma0 interface specify
                         that requests between 0x40000000 and
                         0xC0000000 are satisfied with memory from
                         0x80000000 to 0xffffffff -- the kernel
                         interface is also so kludged.  This is to
                         allow I/O mappings to start with addresses
                         like 0xf0... . */
		      d2 = l4_fpage(pfa, /* maps pfa + 0x40000000 */
				    L4_LOG2_SUPERPAGESIZE, 
				    1, 0).fpage;
		      desc = L4_IPC_SHORT_FPAGE;
		    }
		}

	      else if( l4_is_io_page_fault(d1) )
		{
		  /* handle IO page faults
		     sender sends:
					 4    16    4   6   2
				       +---+------+---+---+--+
			    d1         | F | port | 0 | s |~~|
				       +---+------+---+---+--+
				       +---------------------+
			    d2         |    EIP (ignored)    |
				       +---------------------+
		  */
		  unsigned i, port, size, end, got_all;
		  port = ((l4_fpage_t)(d1)).iofp.iopage;
		  size = ((l4_fpage_t)(d1)).iofp.iosize;
		  end = port + (1L << size);
		  if( end > IO_MAX )
		    end = IO_MAX;
		  got_all = 1;
		  for(i = port; i < end; i++)
		    {
		      if(iomap_alloc_port(i, t.id.task))
			continue;
		      got_all = 0;
		      break;
		    }
		  if(got_all)
		    {
		      d2 = l4_iofpage(port, size, 0).fpage;
		      d1 = 0;
		      desc = L4_IPC_SHORT_FPAGE;
		    }
		  else
		    { 
		      d1 = d2 = 0;
		      desc = L4_IPC_SHORT_MSG;
		    }
		}
	      else 
		{
		  printf("SIGMA0: can't handle d1=0x%x, d2=0x%x "
			 "from thread=%x\n", d1, d2, t.lh.low);

		  /* unknown request */
		  d1 = d2 = 0;
		}
	    }

	reply:

	  if (debug)
	    printf("SIGMA0: sending d1=0x%x, d2=0x%x "
		   "to thread=%x\n", d1, d2, t.lh.low);

	  /* send reply and wait for next message */
	  err = l4_i386_ipc_reply_and_wait(t, desc, d1, d2,
					   &t, 0, &d1, &d2,
					   L4_IPC_TIMEOUT(0,1,0,0,0,0), 
					   /* snd timeout = 0 */
					   &result);
					   
	}
    }
}

static void find_free(l4_umword_t *d1, l4_umword_t *d2, owner_t owner)
{
  /* XXX this routine should be in the memmap_*() interface because we
     don't know about quotas here.  we can easily select a free page
     which we later can't allocate because we're out of quota. */

  /* for kernel tasks, start looking at the back */
  vm_offset_t address = owner < ROOT_TASKNO 
    ? (mem_high - 1) & L4_SUPERPAGEMASK
    : 0;

  for (;;)
    {
      if (memmap_nrfree_superpage(address) != 0)
	{
	  if (owner < ROOT_TASKNO) address += L4_SUPERPAGESIZE - L4_PAGESIZE;

	  for (;;)
	    {
	      if (memmap_owner_page(address) != O_FREE)
		{
		  if (owner < ROOT_TASKNO) address -= L4_PAGESIZE;
		  else address += L4_PAGESIZE;

		  continue;
		}
	      
	      /* found! */
	      *d1 = address;
	      *d2 = l4_fpage(address, L4_LOG2_PAGESIZE, 1, 0).fpage;
	      
	      return;
	    }
	}

      if (owner < ROOT_TASKNO)
	{
	  if (address == 0) break;
	  address -= L4_SUPERPAGESIZE;
	}
      else
	{
	  address += L4_SUPERPAGESIZE;
	  if (address >= mem_high) break;
	}
    }

  /* nothing found! */
  *d1 = *d2 = 0;
}
