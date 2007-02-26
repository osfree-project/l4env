/* $Id$ */
/**
 * \file	loader/server/src/pager.c
 *
 * \date	10/06/2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 * \brief	Application pager. Should be moved to an own L4 server. */

#include "pager.h"
#include "dm-if.h"
#include "app.h"

#include <stdio.h>
#include <stdarg.h>

#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/thread/thread.h>
#include <l4/rmgr/librmgr.h>
#include <l4/l4rm/l4rm.h>
#include <l4/exec/exec.h>
#include <l4/loader/loader-client.h>
#include <l4/dm_phys/dm_phys.h>

#ifdef DEBUG_PAGER_PF
#define debug_pf(x)	x
#else
#define debug_pf(x)
#endif

l4_threadid_t app_pager_id = L4_INVALID_ID; 	/**< pager thread */

static l4_addr_t pager_map_addr_4K = 0;		/**< map addr for 4K pages */
static l4_addr_t pager_map_addr_4M = 0;		/**< map addr for 4M pages */
static l4_addr_t ki_map_addr       = 0;		/**< address of KI page */
static l4_addr_t dummy_map_addr    = 0;		/**< address of dummy page */


/** Return <>0 if address lays inside an application area.
 *
 * \param tid		id of thread the page fault occured in
 * \param addr		address of the page fault occured where
 * \retval app_area	pointer to area the address is situated in */
static int
pf_in_app(l4_threadid_t tid, l4_addr_t addr, app_t *app, app_area_t **app_area)
{
  int i;
  app_area_t *aa;

  /* Go through all app_areas we page for the application */
  for (i=0; i<app->app_area_next_free; i++)
    {
      aa = app->app_area + i;
      if ((aa->flags & APP_AREA_VALID)
	  && (addr>=aa->beg.app && addr<aa->beg.app+aa->size))
	{
	  *app_area = aa;
	  return 1;
	}
    }
  
  return 0;
}

/** handle failed rmgr requests */
static void
rmgr_memmap_error(const char *format,...)
{
  va_list list;
  va_start(list, format);
  vprintf(format, list);
  printf("\n");
  va_end(list);
  rmgr_dump_mem();
}

/** Forward a pagefault to rmgr
 *
 * \param app		application descriptor
 * \param dw0		pagefault address
 * \param dw1		pagefault EIP
 * \retval reply	type of reply message
 * \retval log2_size	size of flexpage (4k or 4M) */
static void
forward_pf_rmgr(app_t *app, l4_umword_t *dw0, l4_umword_t *dw1, 
		void **reply, unsigned int log2_size)
{
  int error, rw = 0;
  int page_mask = ~((1<<log2_size)-1);
  unsigned int pfa = *dw0 & page_mask;
  l4_addr_t map_addr;
  l4_msgdope_t result;
  l4_umword_t dummy;

  if (*dw0 != 0xfffffffc)
    rw = 2 /* writeable */;

  /* We have to take care here to distinguish between 4k- and 4M-mappings
   * because Fiasco does not do any 4M-mappings on an address where a 
   * 4k-mapping where established anytime before. */
  map_addr = (log2_size == L4_LOG2_PAGESIZE)
	   ? pager_map_addr_4K
	   : pager_map_addr_4M;

  /* We have to unmap here to make sure the place is empty. If an application
   * requests an page twice, the second grant operation fails so the page is
   * not granted. Threre is no mechanism in the L4 API which detects grant
   * errors. */
  l4_fpage_unmap(l4_fpage(map_addr, log2_size, L4_FPAGE_RW, L4_FPAGE_MAP),
		 L4_FP_FLUSH_PAGE | L4_FP_ALL_SPACES);

  for (;;)
    {
      /* we could get l4_thread_ex_regs'd ... */
      error = l4_i386_ipc_call(rmgr_pager_id,
			       L4_IPC_SHORT_MSG, *dw0 | rw, 0,
			       L4_IPC_MAPMSG(map_addr, log2_size), 
			         &dummy, &dummy,
			       L4_IPC_NEVER, &result);
      
      if (error != L4_IPC_SECANCELED && error != L4_IPC_SEABORTED)
	break;
    }
  if (error || !l4_ipc_fpage_received(result))
    {
      if (*dw0 >= 0x40000000)
	{
	  app_msg(app, "RMGR denied mapping of adapter page at %08x eip %08x\n",
	          *dw0, *dw1);
	  enter_kdebug("app_pager");
	}
      else
	{
	  app_msg(app, "Can't handle pagefault at %08x eip %08x:", *dw0, *dw1);
	  rmgr_memmap_error("RMGR denies page at %08x "
		            "(map=%08x, error=%02x result=%08x)",
			    *dw0, map_addr, error, result.msgdope);
	  enter_kdebug("app_pager");
	}
    }

  /* grant the page to the application ... */
  *dw0 = pfa;
  *dw1 = l4_fpage(map_addr, log2_size, L4_FPAGE_RW, L4_FPAGE_GRANT).fpage;

  *reply = L4_IPC_SHORT_FPAGE;
}

/** Forward a pagefault to dataspace
 *
 * \param app		application descriptor
 * \param dw0		pagefault address
 * \param dw1		pagefault EIP
 * \retval reply	type of reply message */
static void
forward_pf_ds(app_t *app, app_area_t *aa, 
	      l4_umword_t *dw0, l4_umword_t *dw1, void **reply)
{
  int error;
  unsigned int pfa, log2_size, fpage_flags;
  l4_offs_t offset;
  l4_size_t size;
  l4_addr_t map_addr;
  l4_uint32_t flags;
  l4_addr_t fpage_addr;
  l4_addr_t fpage_size;

  do
    {
#if 1 // 4MB support on Fiasco was broken
      /* first test if we can send a 4MB page */
      pfa       = *dw0 & L4_SUPERPAGEMASK;
      
      if (   (pfa >= aa->beg.app)
	  && (pfa+L4_SUPERPAGEMASK <= aa->beg.app+aa->size))
	{
	  int ok;
	  
	  offset    = pfa - aa->beg.app;
	  size      = L4_SUPERPAGESIZE;
	  log2_size = L4_LOG2_SUPERPAGESIZE;
	  map_addr  = pager_map_addr_4M;
	  
	  if ((error = l4dm_memphys_pagesize(&aa->ds, offset,
					     L4_SUPERPAGESIZE, 
					     L4_LOG2_SUPERPAGESIZE,
					     &ok)))
	    {
	      app_msg(app, "Error %d doing memphys_pagesize\n", error);
	      return;
	    }

	  if (ok)
	    /* well, map as 4MB page */
	    break;
	}
#endif

      /* 4MB test failed, send as 4K page */
      pfa       = *dw0 & L4_PAGEMASK;
      offset    = pfa - aa->beg.app;
      size      = L4_PAGESIZE;
      log2_size = L4_LOG2_PAGESIZE;
      map_addr  = pager_map_addr_4K;
    } while (0);

  /* We have to unmap here to make sure the place is empty. If an application
   * requests an page twice, the second grant operation fails so the page is
   * not granted. Threre is no mechanism in the L4 API which detects grant
   * errors. */
  l4_fpage_unmap(l4_fpage(map_addr, log2_size, L4_FPAGE_RW, L4_FPAGE_MAP),
		 L4_FP_FLUSH_PAGE | L4_FP_ALL_SPACES);
  
  flags = L4DM_RO;
  fpage_flags = L4_FPAGE_RO;
  if (aa->type & L4_DSTYPE_WRITE)
    {
      flags |= L4DM_RW;
      fpage_flags |= L4_FPAGE_RW;
    }
  else if (*dw0 & 2)
    {
      app_msg(app, "R/W pagefault at %08x in R/O dataspace\n", *dw0 & ~3);
      enter_kdebug("app_pager");
    }

  if ((error = l4dm_map_pages(&aa->ds, offset, size, map_addr, log2_size, 0,
			      flags, &fpage_addr, &fpage_size)))
    {
      app_msg(app, "Error %d mapping page of dataspace\n", error);
      return;
    }
  
  /* grant the page to the application ... */
  *dw0 = pfa;
  *dw1 = l4_fpage(map_addr, log2_size, fpage_flags, L4_FPAGE_GRANT).fpage;

  *reply = L4_IPC_SHORT_FPAGE;
}

/** Map kernel info page */
static int
map_kernel_info_page(void)
{
  int error;
  l4_uint32_t rm_area;
  l4_msgdope_t result;
  l4_snd_fpage_t fpage;

  if ((error = l4rm_area_reserve(L4_PAGESIZE, L4RM_LOG2_ALIGNED,
			         &ki_map_addr, &rm_area)))
    {
      printf("Error %d reserving 4K for KI page\n", error);
      return error;
    }
  
  error = l4_i386_ipc_call(rmgr_pager_id, L4_IPC_SHORT_MSG, 1, 1,
                           L4_IPC_MAPMSG((l4_umword_t)ki_map_addr,
                                         L4_LOG2_PAGESIZE),
                           &fpage.snd_base, &fpage.fpage.fpage,
                           L4_IPC_NEVER, &result);
  if (error || !l4_ipc_fpage_received(result))
    {
      printf("Can't map KI page (map=%08x, error=%02x result=%08x)\n",
	     ki_map_addr, error, result.msgdope);
      return error;
    }

  return 0;
}

/** Map dummy page */
static int
map_dummy_page(void)
{
  int error;
  l4dm_dataspace_t ds;

  if ((error = create_ds(app_dm_id, L4_PAGESIZE, &dummy_map_addr,
			 &ds, "loader dummy page")))
    {
      printf("Error %d creating dummy page ds\n", error);
      return error;
    }
  
  memset((void*)dummy_map_addr, 0, L4_PAGESIZE);

  return 0;
}


extern void my_pc_reset(void);

/** Pager thread for the application.
 *
 * \param data		pointer to parameter struct */
static void
app_pager_thread(void *data)
{
  int rw, error, fpage_rw;
  l4_uint32_t rm_area;
  l4_umword_t dw1, dw2;
  void *reply;
  l4_addr_t send_addr;
  l4_msgdope_t result;
  l4_threadid_t src_tid;

  if ((error = l4rm_area_reserve(L4_PAGESIZE, L4RM_LOG2_ALIGNED,
			         &pager_map_addr_4K, &rm_area)))
    {
      printf("Error %d reserving 4K map area\n", error);
      enter_kdebug("app_pager");
    }

  if ((error = l4rm_area_reserve(L4_SUPERPAGESIZE, L4RM_LOG2_ALIGNED,
			         &pager_map_addr_4M, &rm_area)))
    {
      printf("Error %d reserving 4M map area\n", error);
      enter_kdebug("app_pager");
    }

  /* shake hands with creator */
  l4thread_started(NULL);

  for (;;)
    {
      /* wait for a page fault */
      int error = l4_i386_ipc_wait(&src_tid,
				   L4_IPC_SHORT_MSG, &dw1, &dw2,
				   L4_IPC_NEVER, &result);
      while (!error)
	{
	  int skip_reply = 0;
	  app_t *app;
	  app_area_t *aa;

#ifdef DEBUG_PAGER
	  printf("pf: %x.%x pfa=%08x eip=%08x\n",
	      src_tid.id.task, src_tid.id.lthread, dw1, dw2);
#endif
	  reply = L4_IPC_SHORT_MSG;
	  
	  /* page fault belonging to one of our tasks? */
	  if ((app = task_to_app(src_tid)))
	    {
	      rw    = dw1 & 2;
	      reply = L4_IPC_SHORT_FPAGE;
	     
	      if ((dw1 == 0xfffffffc)
		  && !(app->flags & APP_NOSIGMA0))

		{
		  /* XXX sigma0 protocol: free page requested. We should
		   * deliver a page of an dataspace pool here. */
		  forward_pf_rmgr(app, &dw1, &dw2, &reply, L4_LOG2_PAGESIZE);
		  app_msg(app, "GOT 0xfffffffc REQUEST, RMGR SENT %08x %08x\n",
		      dw1, dw2);
		}
	      else if ((dw1 >= 0x40000000)
		       && !(app->flags & APP_NOSIGMA0))
		{
		  /* sigma0 protocol: adapter space requested. */
    		  debug_pf(printf("PF (%c) %08x in adapter space. "
				  "Forwarding to RMGR.\n",
				  dw1 & 2 ? 'w' : 'r', dw1 & ~3));
	      
		  /* forward pf in adapter space to RMGR */
		  forward_pf_rmgr(app, &dw1, &dw2, &reply, 
				  L4_LOG2_SUPERPAGESIZE);
		}
	      else if (pf_in_app(src_tid, dw1, app, &aa))
		{
		  /* consider section attributes (ro or rw) */

		  if (aa->flags & APP_AREA_PAGE)
		    {
		      /* forward pagefault to dataspace manager */
		      debug_pf(printf("PF (%c) %08x in app area %08x-%08x. "
				      "Forwarding to ds.\n",
				      dw1 & 2 ? 'w' : 'r', dw1 & ~3, 
				      aa->beg.app, aa->beg.app+aa->size));
		      
		      forward_pf_ds(app, aa, &dw1, &dw2, &reply);
		    }
		  else
		    {
		      /* Handle pagefault ourself since we own the dataspace. */
		      send_addr = aa->beg.here+(dw1 & L4_PAGEMASK)-aa->beg.app;
		      
		      debug_pf(printf("PF (%c) %08x in app area %08x-%08x. "
				      "Sending %08x.\n",
				      dw1 & 2 ? 'w' : 'r', dw1 & ~3, 
				      aa->beg.app, aa->beg.app+aa->size, 
				      send_addr));

		      /* We can simply touch the target address here because
		       * our region mapper will abort with error if the page
		       * cannot be mapped properly. */
		      if (aa->type & L4_DSTYPE_WRITE)
			{
			  fpage_rw = L4_FPAGE_RW;
			  *((volatile char*)send_addr) |= 0;
			}
		      else
			{
			  fpage_rw = L4_FPAGE_RO;
			  *(volatile char*)send_addr;
			}

		      dw1 &= L4_PAGEMASK;
	    	      dw2 = l4_fpage(send_addr, L4_LOG2_PAGESIZE,
				     fpage_rw, L4_FPAGE_MAP).fpage;
		    }
		}
	      else if (dw1 == 1 && (dw2 & 0xff) == 1)
		{
		  /* sigma0 protocol: KI page requested */
		  debug_pf(printf("PF (%c) %08x in KI page. Sending KI page.\n",
				  dw1 & 2 ? 'w' : 'r', dw1 & ~3));
	      
		  /* pf in KI page -> request read-only from rmgr */
		  dw1 = 0;
		  dw2 = l4_fpage(ki_map_addr, L4_LOG2_PAGESIZE,
				 L4_FPAGE_RO, L4_FPAGE_MAP).fpage;
		}
	      else if (dw1 >= 0x0009F000 && dw1 <= 0x000BFFFF)
		{
		  /* graphics memory requested */
		  if (app->flags & APP_NOVGA)
		    {
		      debug_pf(printf("PF (%c) %08x in video memory. "
				      "Sending dummy page.\n",
				  dw1 & 2 ? 'w' : 'r', dw1 & ~3));
		      
		      dw1 &= L4_PAGEMASK;
		      dw2 = l4_fpage(dummy_map_addr, L4_LOG2_PAGESIZE,
				     L4_FPAGE_RW, L4_FPAGE_MAP).fpage;
		    }
		  else
		    {
		      debug_pf(printf("PF (%c) %08x in video memory. "
				      "Forwarding to RMGR.\n",
				      dw1 & 2 ? 'w' : 'r', dw1 & ~3));
		      forward_pf_rmgr(app, &dw1, &dw2, &reply, 
				      L4_LOG2_PAGESIZE);
		    }
		}
	      else if (dw1 >= 0x000C0000 && dw1 <= 0x000FFFFF)
		{
		  /* BIOS requested */
		  debug_pf(printf("PF (%c) %08x in BIOS area. "
				  "Forwarding to RMGR.\n",
				  dw1 & 2 ? 'w' : 'r', dw1 & ~3));
		  
		  forward_pf_rmgr(app, &dw1, &dw2, &reply, L4_LOG2_PAGESIZE);
		}
	      else if (dw1 == 0x3ffff000)
		{
		  sm_exc_t exc;
	      
		  /* XXX Special hook: L4Linux wants to reboot. */
		  debug_pf(printf("PF (%c) %08x. Reboot request. "
				  "Killing task\n",
				  dw1 & 2 ? 'w' : 'r', dw1 & ~3));

		  if ((error =
			l4loader_app_kill(app->env->loader_id,
					  src_tid.id.task, 0, &exc)))
		    {
		      app_msg(app, "Error %d (%s) killing task %x",
				   error, l4env_errstr(error), src_tid.id.task);
		    }
		  else
		    {
		      /* success, don't reply because the sender does not
		       * exists anymore ... */
		      skip_reply = 1;
		      if (app->flags & APP_REBOOTABLE)
			{
			  app_msg(app, "Serving reboot request");
			  my_pc_reset();
			}
		    }
		}
	      else
		{
		  /* unknown pagefault */
		  debug_pf(printf("PF (%c) %08x. Can't handle.\n",
				  dw1 & 2 ? 'w' : 'r', dw1 & ~3));

		  /* check for double page faults */
		  if (dw1 == app->last_pf && dw2 == app->last_pf_eip)
		    {
		      app_msg(app, "Double PF (%c) at %08x eip %08x (%x.%02x)",
				   dw1 & 2 ? 'w' : 'r', dw1 & ~3, dw2, 
				   src_tid.id.task, src_tid.id.lthread);
		      enter_kdebug("loader pager");
		    }

		  app->last_pf     = dw1;
		  app->last_pf_eip = dw2;
		  
		  /* send nothing */
	      	  dw1 = dw2 = 0;
    		  reply = L4_IPC_SHORT_MSG;
		}
	    } /* if (task_to_app()) */
	  
	  error = 0;
	  
	  if (skip_reply)
	    break;
	  else
	    {
	      error = l4_i386_ipc_reply_and_wait(
		  			src_tid, reply, dw1, dw2,
					&src_tid, L4_IPC_SHORT_MSG, &dw1, &dw2,
					L4_IPC_TIMEOUT(0,1,0,0,0,0),
					&result);
	    }
	   
	  /* send error while granting? flush fpage! */
	  if (   (error==L4_IPC_SETIMEOUT)
	      && (reply==L4_IPC_SHORT_FPAGE)
	      && (dw2 & 1))
	    {
	      l4_fpage_unmap((l4_fpage_t)dw2, 
			     L4_FP_FLUSH_PAGE | L4_FP_ALL_SPACES);
	    }

	}
      
      if (error)
	printf("IPC error %02x in application's pager\n", error);
    }
}

/** Create the application's pager.
 *
 * \param pager_id	L4 thread ID of pager
 * \return		0 on success */
int
start_app_pager(void)
{
  int error;
  l4thread_t  l4_thread;

  /* map KI page */
  if ((error = map_kernel_info_page()))
    return error;

  if ((error = map_dummy_page()))
    return error;

  if ((l4_thread = l4thread_create((l4thread_fn_t)app_pager_thread, NULL,
				   L4THREAD_CREATE_SYNC)) < 0)
    {
      printf("Error %d creating pager thread\n", l4_thread);
      return l4_thread;
    }

  app_pager_id = l4thread_l4_id(l4_thread);

  return 0;
}

