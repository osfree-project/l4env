/* $Id$ */
/**
 * \file	loader/server/src/pager.c
 * \brief	Application pager. Should be moved to an own L4 server.
 *
 * \date	10/06/2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include "pager.h"
#include "dm-if.h"
#include "app.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kernel.h>
#include <l4/sys/cache.h>
#include <l4/thread/thread.h>
#include <l4/rmgr/librmgr.h>
#include <l4/l4rm/l4rm.h>
#include <l4/util/l4_macros.h>
#include <l4/loader/loader-client.h>
#include <l4/generic_ts/generic_ts.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/sigma0/sigma0.h>
#include <l4/sigma0/kip.h>

#define dbg_pf(x...)		//app_msg(app, x)
#define dbg_adap_pf(x...)	//app_msg(app, x)
#define dbg_incoming(x...)	//printf(x)

l4_threadid_t app_pager_id = L4_INVALID_ID;	/**< pager thread. */

static l4_addr_t pager_map_addr_4K = 0;		/**< map addr for 4K pages. */
static l4_addr_t pager_map_addr_4M = 0;		/**< map addr for 4M pages. */
static l4_kernel_info_t *kip;			/**< address of KI page. */
static l4_threadid_t _rmgr_pager_id;		/**< thread id of roottask pager */
#ifdef ARCH_x86
static l4_addr_t tb_stat_map_addr  = 0;		/**< address of Tbuf status. */
#endif


/** Return <>0 if address lays inside an application area.
 *
 * \param addr		address of the page fault occured where
 * \param app		application descriptor
 * \retval app_area	pointer to area the address is situated in */
static int inline
pf_in_app(l4_addr_t addr, app_t *app, app_area_t **app_area)
{
  int i;
  app_area_t *aa;

  /* Go through all app_areas we page for the application */
  for (i=0; i<app->app_area_next_free; i++)
    {
      aa = app->app_area + i;
      if ((aa->flags & APP_AREA_VALID) &&
	  addr>=aa->beg.app && addr<aa->beg.app+aa->size)
	{
	  *app_area = aa;
	  return 1;
	}
    }

  return 0;
}

/** Translate app address into here address. */
l4_addr_t fastcall
addr_app_to_here(app_t *app, l4_addr_t addr)
{
  app_area_t *aa;

  return (pf_in_app(addr, app, &aa) && aa->beg.here)
      ? aa->beg.here + addr - aa->beg.app
      : 0;
}

/** Handle failed rmgr requests. */
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

/** Forward a pagefault to rmgr.
 *
 * \param app		application descriptor
 * \param dw1		pagefault address
 * \param dw2		pagefault EIP
 * \retval reply	type of reply message
 * \retval log2_size	size of flexpage (4k or 4M) */
static void
forward_pf_rmgr(app_t *app, l4_umword_t *dw1, l4_umword_t *dw2,
		void **reply, unsigned int log2_size)
{
  int error, rw = 0;
  int page_mask = ~((1<<log2_size)-1);
  unsigned int pfa = *dw1 & page_mask;
  l4_addr_t map_addr;
  l4_msgdope_t result;
  l4_umword_t dummy;
  l4_msgtag_t tag;

  if (*dw1 != 0xfffffffc)
    rw = 2 /* writable */;

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
      tag = l4_msgtag(L4_MSGTAG_PAGE_FAULT, 0, 0, 0);
      error = l4_ipc_call_tag(_rmgr_pager_id,
			      L4_IPC_SHORT_MSG, *dw1 | rw, 0, tag,
			      L4_IPC_MAPMSG(map_addr, log2_size),
			        &dummy, &dummy,
			      L4_IPC_NEVER, &result, &tag);

      if (error != L4_IPC_SECANCELED && error != L4_IPC_SEABORTED)
	break;
    }
  if (error || !l4_ipc_fpage_received(result))
    {
      app_msg(app, "Can't handle pagefault at %08lx eip %08lx:", *dw1, *dw2);
      rmgr_memmap_error("ROOT denies page at %08x "
		        "(map=%08x, error=%02x result=%08x)",
	    		*dw1, map_addr, error, result.msgdope);
      enter_kdebug("app_pager");
      *reply = L4_IPC_SHORT_MSG;
      return;
    }

  /* grant the page to the application ... */
  *dw1   = pfa;
  *dw2   = l4_fpage(map_addr, log2_size, L4_FPAGE_RW, L4_FPAGE_GRANT).fpage;
  *reply = L4_IPC_SHORT_FPAGE;
}

/** Forward an I/O pagefault to rmgr.
 *
 * \param app		application descriptor
 * \param dw1		pagefault address
 * \param dw2		pagefault EIP
 * \retval reply	type of reply message
 * \retval skip_reply	don't send a reply message */
#ifdef ARCH_x86
static void
resolve_iopf_rmgr(app_t *app, l4_umword_t *dw1, l4_umword_t *dw2,
		  void **reply, int *skip_reply)
{
  int error, i;
  l4_msgdope_t result;
  l4_umword_t dummy;
  unsigned port = ((l4_fpage_t)(*dw1)).iofp.iopage;
  unsigned size = ((l4_fpage_t)(*dw1)).iofp.iosize;
  unsigned ports = 1<<size;
  static int ioports_mapped;
  static unsigned last_port = ~0;
  l4_msgtag_t tag;

  if (!(app->flags & APP_ALLOW_CLI) &&
      (port != 0 || size != L4_WHOLE_IOADDRESS_SPACE))
    {
      if (!app->iobitmap)
	{
	  if (!(app->flags & APP_MSG_IO))
	    app_msg(app, "Not allowed to perform any I/O");
	  app->flags |= APP_MSG_IO;
	  *dw2   = 0; /* hint for client that we don't map anything */
	  *reply = L4_IPC_SHORT_MSG;
	  return;
	}

      for (i=port; i<port+ports; i++)
	{
	  if (!(app->iobitmap[i/8] & (1<<(i%8))))
	    {
	      /* Do not print out a message for a scan but only
	       * for individual failures */
	      if (last_port + 1 != port)
		app_msg(app, "Not allowed to access I/O port %04x "
			     " (req %04x-%04x)", i, port, port+ports-1);
	      *dw2   = 0; /* hint for client that we don't map anything */
	      *reply = L4_IPC_SHORT_MSG;
              last_port = port;
	      return;
	    }
	}
    }

  if (ioports_mapped == 0)
    {
      /* This is the first time an I/O pagefault occured. Map in the
       * whole I/O address space */
      for (;;)
	{
	  /* we could get l4_thread_ex_regs'd ... */
          tag = l4_msgtag(L4_MSGTAG_IO_PAGE_FAULT, 0, 0, 0);
	  error = l4_ipc_call_tag
             (_rmgr_pager_id, L4_IPC_SHORT_MSG,
              l4_iofpage(0, L4_WHOLE_IOADDRESS_SPACE, 0).fpage, 0, tag,
              L4_IPC_IOMAPMSG(0, L4_WHOLE_IOADDRESS_SPACE),
              &dummy, &dummy, L4_IPC_NEVER, &result, &tag);

	  if (error != L4_IPC_SECANCELED && error != L4_IPC_SEABORTED)
	    break;
	}
      if (error || !l4_ipc_fpage_received(result))
	{
	  app_msg(app, "WARNING: Can't map I/O space, ROOT denies page (result=%08lx)",
                       result.msgdope);
	  /* never try again */
	  ioports_mapped = 2;
	  /* return in ioports_mapped == 2 block */
	}
      else
        /* successfully mapped */
        ioports_mapped = 1;
    }

  if (ioports_mapped == 2)
    {
      /* no I/O ports mapped */
      *dw1 = *dw2 = 0;
      *reply = L4_IPC_SHORT_MSG;
      return;
    }

  if (port == 0 && size == L4_WHOLE_IOADDRESS_SPACE
      && !(app->flags & APP_ALLOW_CLI))
    {
      if (*dw2 == ~0xeUL)
	{
	  /* Just a request, do not kill */
	  *dw1 = *dw2 = 0;
	  *reply = L4_IPC_SHORT_MSG;
	  return;
	}
      app_msg(app, "Task not allowed to execute cli/sti -- killing");
      if ((error = l4ts_kill_task_recursive(app->tid)))
	app_msg(app, "Error %d (%s)", error, l4env_errstr(error));
      *skip_reply = 1;
      return;
    }

  *dw1   = 0;
  *dw2   = l4_iofpage(port, size, L4_FPAGE_MAP).fpage;
  last_port = port;
}
#endif

/**
 * \param app		application descriptor
 * \param dw1		extended sigma0 code
 * \param dw2		fpage
 * \retval reply	type of reply message */
static void
handle_extended_sigma0_request(app_t *app, l4_umword_t *dw1,
			       l4_umword_t *dw2, void **reply)
{
  int error;
  l4_addr_t map_addr = 0;
  l4_msgdope_t result;
  l4_umword_t dummy;
  l4_msgtag_t tag;
  unsigned log2_size = ((l4_fpage_t)(*dw2)).fp.size;

  /* We have to take care here to distinguish between 4k- and 4M-mappings
   * because Fiasco does not do any 4M-mappings on an address where a
   * 4k-mapping where established anytime before. */
  switch (log2_size)
    {
    case L4_LOG2_PAGESIZE:
      map_addr = pager_map_addr_4K;
      break;
    case L4_LOG2_SUPERPAGESIZE:
      map_addr = pager_map_addr_4M;
      break;
    }

  if (SIGMA0_IS_MAGIC_REQ(*dw1))
    {
      map_addr = pager_map_addr_4K;
    }

  if (!map_addr)
    {
      app_msg(app, "fpage log2_size=%d", log2_size);
      enter_kdebug("stop");
      *reply = L4_IPC_SHORT_MSG;
      return;
    }

  /* We have to unmap here to make sure the place is empty. If an application
   * requests an page twice, the second grant operation fails so the page is
   * not granted. Threre is no mechanism in the L4 API which detects grant
   * errors. */
  l4_fpage_unmap(l4_fpage(map_addr, log2_size, L4_FPAGE_RW, L4_FPAGE_MAP),
		 L4_FP_FLUSH_PAGE | L4_FP_ALL_SPACES);

  for (;;)
    {
      /* we could get l4_thread_ex_regs'd ... */
      tag = l4_msgtag(L4_MSGTAG_SIGMA0, 0, 0, 0);
      error = l4_ipc_call_tag(_rmgr_pager_id,
			      L4_IPC_SHORT_MSG, *dw1, *dw2, tag,
			      L4_IPC_MAPMSG(map_addr, log2_size),
			        &dummy, &dummy,
			      L4_IPC_NEVER, &result, &tag);

      if (error != L4_IPC_SECANCELED && error != L4_IPC_SEABORTED)
	break;
    }
  if (error || !l4_ipc_fpage_received(result))
    {
      app_msg(app, "Failed sigma0 request with dw1=%08lx dw2=%08lx:",
                   *dw1, *dw2);
      rmgr_memmap_error("ROOT (" l4util_idfmt ") denies page at %08x "
                        "(map=%08x, error=%02x result=%08x)",
                        l4util_idstr(_rmgr_pager_id),
                        *dw1, map_addr, error, result.msgdope);
      enter_kdebug("app_pager");
      *reply = L4_IPC_SHORT_MSG;
      return;
    }

  /* grant the page to the application ... */
  *dw1   = 0;
  *dw2   = l4_fpage(map_addr, log2_size, L4_FPAGE_RW, L4_FPAGE_GRANT).fpage;
  *reply = L4_IPC_SHORT_FPAGE;
}

/** Forward a pagefault to dataspace manager.
 *
 * \param app		application descriptor
 * \param aa		application area descriptor
 * \param dw1		pagefault address
 * \param dw2		pagefault EIP
 * \retval reply	type of reply message */
static void
forward_pf_ds(app_t *app, app_area_t *aa,
	      l4_umword_t *dw1, l4_umword_t *dw2, void **reply)
{
  int error;
  unsigned int pfa, log2_size, fpage_flags;
  l4_offs_t offset;
  l4_size_t size;
  l4_addr_t map_addr;
  l4_uint32_t flags;
  l4_addr_t fpage_addr;
  l4_size_t fpage_size;

  do
    {
      /* first test if we can send a 4MB page */
      pfa = *dw1 & L4_SUPERPAGEMASK;

      if (!(aa->flags & APP_AREA_NOSUP) &&
	  (pfa >= aa->beg.app && pfa+L4_SUPERPAGEMASK <= aa->beg.app+aa->size))
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
	      app_msg(app, "Error %d doing memphys_pagesize", error);
	      return;
	    }

	  if (ok)
	    /* well, map as 4MB page */
	    break;
	}

      /* 4MB test failed, send as 4K page */
      pfa       = *dw1 & L4_PAGEMASK;
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
  else if (*dw1 & 2)
    {
      app_msg(app, "R/W pagefault at %08lx in R/O dataspace", *dw1 & ~3);
      enter_kdebug("app_pager");
    }

  if ((error = l4dm_map_pages(&aa->ds, offset, size, map_addr, log2_size, 0,
			      flags, &fpage_addr, &fpage_size)))
    {
      app_msg(app, "Error %d mapping page of dataspace", error);
      return;
    }

  /* grant the page to the application ... */
  *dw1 = pfa;
  *dw2 = l4_fpage(map_addr, log2_size, fpage_flags, L4_FPAGE_GRANT).fpage;

  *reply = L4_IPC_SHORT_FPAGE;
}

/** Map kernel info page from rmgr. */
static int
map_kernel_info_page(void)
{
#ifdef ARCH_x86
  int error;
  l4_uint32_t rm_area;
#endif

  if (!l4sigma0_kip_map(L4_INVALID_ID))
    {
      printf("Cannot map KI page\n");
      return -L4_ENOMEM;
    }

#ifdef ARCH_x86
  if (is_fiasco())
    {
      if ((error = l4rm_area_reserve(L4_PAGESIZE, L4RM_LOG2_ALIGNED,
				     &tb_stat_map_addr, &rm_area)))
	{
	  printf("Error %d reserving 4K for tbuf status page\n", error);
	  return error;
	}

      if ((l4sigma0_map_tbuf(_rmgr_pager_id, tb_stat_map_addr)))
	{
	  printf("Can't map tbuf status page\n");
	  l4rm_area_release(rm_area);
	  tb_stat_map_addr = 0;
	}
    }
#endif

  return 0;
}

int
is_fiasco(void)
{
  return (l4sigma0_kip_version() == L4SIGMA0_KIP_VERSION_FIASCO);
}

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
  l4_msgtag_t tag;

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

  _rmgr_pager_id = rmgr_pager_id();

  /* shake hands with creator */
  l4thread_started(NULL);

  for (;;)
    {
      /* wait for a page fault */
      int error = l4_ipc_wait_tag(&src_tid,
			          L4_IPC_SHORT_MSG, &dw1, &dw2,
		   	          L4_IPC_NEVER, &result, &tag);
      while (!error)
	{
	  int skip_reply = 0;
	  app_t *app;
	  app_area_t *aa;

	  reply = L4_IPC_SHORT_MSG;

	  if (!l4_msgtag_is_page_fault(tag)
              && !l4_msgtag_is_io_page_fault(tag)
              && !l4_msgtag_is_sigma0(tag))
            {
              printf("Cannot handle IPC type %ld from "
                     l4util_idfmt" (%lx,%lx)\n",
                     l4_msgtag_label(tag), l4util_idstr(src_tid),
                     dw1, dw2);
              if (l4_msgtag_is_exception(tag))
                printf("  Exception at %lx\n", l4_utcb_exc_pc(l4sys_utcb_get()));
              skip_reply = 1;
            }
          else
	  /* page fault belonging to one of our tasks? */
	  if ((app = task_to_app(src_tid)))
	    {
	      rw    = dw1 & 2;
	      reply = L4_IPC_SHORT_FPAGE;

	      if (l4_msgtag_is_sigma0(tag)
                  && SIGMA0_IS_MAGIC_REQ(dw1)
                  && !(app->flags & APP_NOSIGMA0))
		handle_extended_sigma0_request(app, &dw1, &dw2, &reply);

	      else if (l4_msgtag_is_sigma0(tag)
                       && (dw1 == 0xfffffffc)
                       && !(app->flags & APP_NOSIGMA0))
		{
		  /* XXX sigma0 protocol: free page requested. We should
		   * deliver a page of a dataspace pool here. */
		  forward_pf_rmgr(app, &dw1, &dw2, &reply, L4_LOG2_PAGESIZE);
		  app_msg(app, "GOT 0xfffffffc REQUEST, ROOT SENT %08lx %08lx",
		      dw1, dw2);
		}
#ifdef ARCH_x86
	      else if (l4_msgtag_is_io_page_fault(tag)
	               && l4_is_io_page_fault(dw1)
                       && !(app->flags & APP_NOSIGMA0))
                  resolve_iopf_rmgr(app, &dw1, &dw2, &reply, &skip_reply);
#endif /* ARCH_x86 */
	      else if (l4_msgtag_is_page_fault(tag)
                       && pf_in_app(dw1, app, &aa))
		{
		  /* consider section attributes (ro or rw) */
		  if (aa->flags & APP_AREA_PAGE)
		    {
		      /* forward pagefault to dataspace manager */
		      dbg_pf("PF (%c,eip=%08lx) %08lx in app area %08lx-%08lx. "
			     "Forwarding to ds.",
			     dw1 & 2 ? 'w' : 'r', dw2, dw1 & ~3,
			     aa->beg.app, aa->beg.app+aa->size);

		      forward_pf_ds(app, aa, &dw1, &dw2, &reply);
		    }
		  else
		    {
		      /* Handle pagefault ourself since we own the dataspace. */
		      send_addr = aa->beg.here+(dw1 & L4_PAGEMASK)-aa->beg.app;

		      dbg_pf("PF (%c,eip=%08lx) %08lx in app area %08lx-%08lx. "
			     "Sending %08lx.",
			     dw1 & 2 ? 'w' : 'r', dw2, dw1 & ~3,
			     aa->beg.app, aa->beg.app+aa->size,
			     send_addr);

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
	      else if (l4_msgtag_is_sigma0(tag)
                       && dw1 == 1 && (dw2 & 0xff) == 1)
		{
		  /* sigma0 protocol: KI page requested */
		  dbg_pf("PF (%c, eip=%08lx) %08lx in KI page. Sending KI page.",
			  dw1 & 2 ? 'w' : 'r', dw2, dw1 & ~3);

		  /* pf in KI page -> request read-only from rmgr */
		  dw1 = 0;
		  dw2 = l4_fpage((l4_addr_t)kip, L4_LOG2_PAGESIZE,
				 L4_FPAGE_RO, L4_FPAGE_MAP).fpage;
		}
#ifdef ARCH_x86
	      else if (l4_msgtag_is_sigma0(tag)
                       && dw1 == 1 && (dw2 & 0xff) == 0xff)
		{
		  /* sigma0 protocol: Tbuf status page requested */
		  dbg_pf("PF (%c, eip=%08lx) %08lx in Tbuf status page. Sending.",
			  dw1 & 2 ? 'w' : 'r', dw2, dw1 & ~3);

		  if (!tb_stat_map_addr)
		    enter_kdebug("tbuf status page");

		  /* pf in KI page -> request read-only from rmgr */
		  dw1 = 0;
		  dw2 = l4_fpage(tb_stat_map_addr, L4_LOG2_PAGESIZE,
				 L4_FPAGE_RO, L4_FPAGE_MAP).fpage;
		}
	      else if (l4_msgtag_is_page_fault(tag)
                       && dw1 >= 0x0009F000 && dw1 <= 0x000BFFFF
                       && app->flags & APP_ALLOW_VGA)
		{
		  /* graphics memory requested */
                  dbg_pf("PF (%c, eip=%08lx) %08lx in video memory. "
                         "Forwarding to ROOT.",
                         dw1 & 2 ? 'w' : 'r', dw2, dw1 & ~3);
                  forward_pf_rmgr(app, &dw1, &dw2, &reply, L4_LOG2_PAGESIZE);
		}
	      else if (l4_msgtag_is_page_fault(tag)
                       && dw1 >= 0x000C0000 && dw1 <= 0x000FFFFF
		       && app->flags & APP_ALLOW_BIOS)
		{
		  /* BIOS requested */
		  dbg_pf("PF (%c, eip=%08lx) %08lx in BIOS area. "
                         "Forwarding to ROOT.",
                         dw1 & 2 ? 'w' : 'r', dw2, dw1 & ~3);
		  forward_pf_rmgr(app, &dw1, &dw2, &reply, L4_LOG2_PAGESIZE);
		}
#endif
	      else
		{
		  /* unknown fault */
		  dbg_pf("Fault (%c, eip=%08lx, type=%ld) %08lx. Can't handle.",
			  dw1 & 2 ? 'w' : 'r', l4_msgtag_label(tag),
                          dw2, dw1 & ~3);

		  /* check for double page faults */
		  if (dw1 == app->last_pf && dw2 == app->last_pf_eip)
		    {
		      app_msg(app, "Double PF (%c) at %08lx eip %08lx ("
			           l4util_idfmt")",
				   dw1 & 2 ? 'w' : 'r', dw1 & ~3, dw2,
				   l4util_idstr(src_tid));
		      enter_kdebug("Double PF, 'g' for kill");
		      if ((error = l4ts_kill_task_recursive(app->tid)))
			app_msg(app, "Error %d (%s)",
				     error, l4env_errstr(error));
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

	  if (reply == L4_IPC_SHORT_FPAGE)
	    {
	      l4_fpage_t fp;
	      fp.raw = dw2;
	      l4_sys_cache_clean_range(fp.fp.page << L4_LOG2_PAGESIZE,
                                       (fp.fp.page << L4_LOG2_PAGESIZE) + (1 << fp.fp.size));
	    }
	  error = l4_ipc_reply_and_wait_tag(src_tid, reply, dw1, dw2,
					l4_msgtag(0, 0, 0, 0), &src_tid, L4_IPC_SHORT_MSG, &dw1, &dw2,
					L4_IPC_SEND_TIMEOUT_0,
					&result, &tag);

	  /* send error while granting? flush fpage! */
	  if (error==L4_IPC_SETIMEOUT && reply==L4_IPC_SHORT_FPAGE && (dw2 & 1))
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
 * \return		0 on success */
int
start_app_pager(void)
{
  int error;
  l4thread_t  l4_thread;

  /* map KI page */
  if ((error = map_kernel_info_page()))
    return error;

  if ((l4_thread = l4thread_create_long(L4THREAD_INVALID_ID,
					app_pager_thread, "loader.pager",
					L4THREAD_INVALID_SP,
					L4THREAD_DEFAULT_SIZE,
					L4THREAD_DEFAULT_PRIO,
					0, L4THREAD_CREATE_SYNC)) < 0)
    {
      printf("Error %d creating pager thread\n", l4_thread);
      return l4_thread;
    }

  app_pager_id = l4thread_l4_id(l4_thread);

  return 0;
}

