/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4io/server/src/res.c
 * \brief  L4Env l4io I/O Server Resource Management Module
 *
 * \date   2007-03-23
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/rmgr/librmgr.h>
#include <l4/l4rm/l4rm.h>
#include <l4/generic_io/generic_io-server.h>  /* IDL IPC interface */
#include <l4/sigma0/sigma0.h>

/* OSKit includes */
#include <stdio.h>
#include <stdlib.h>

/* local includes */
#include "io.h"
#include "res.h"
#include "__config.h"
#include "__macros.h"

#if IO_REQUEST_PAGE
# define IO_REQ_PAGESIZE L4_PAGESIZE
# define IO_REQ_LOG2_PAGESIZE L4_LOG2_PAGESIZE
# define IO_REQ_PAGEMASK L4_PAGEMASK
# define io_req_trunc_page(a) l4_trunc_page(a)
#else
# define IO_REQ_PAGESIZE L4_SUPERPAGESIZE
# define IO_REQ_LOG2_PAGESIZE L4_LOG2_SUPERPAGESIZE
# define IO_REQ_PAGEMASK L4_SUPERPAGEMASK
# define io_req_trunc_page(a) l4_trunc_superpage(a)
#endif

#if IO_SEND_PAGE
# define IO_PAGESIZE L4_PAGESIZE
# define IO_LOG2_PAGESIZE L4_LOG2_PAGESIZE
# define IO_PAGEMASK L4_PAGEMASK
# define io_trunc_page(a) l4_trunc_page(a)
#else
# define IO_PAGESIZE L4_SUPERPAGESIZE
# define IO_LOG2_PAGESIZE L4_LOG2_SUPERPAGESIZE
# define IO_PAGEMASK L4_SUPERPAGEMASK
# define io_trunc_page(a) l4_trunc_superpage(a)
#endif

/*
 * types and module vars
 */

/** Arbitrated resources (creating an USED SPACE list).
 * \ingroup grp_res
 *
 * I/O's flat resource management scheme corresponds to the "leaves" in the
 * Linux 2.4 resource tree. Only BUSY resources emerge in these lists.
 *
 * \krishna OSKit AMM alternative?
 */
typedef struct io_res {
  struct io_res *next;  /**< next in list */
  unsigned long start;  /**< begin of used region */
  unsigned long end;    /**< end of used region */
  io_client_t *client;  /**< holder reference */
} io_res_t;

/** IO ports
 * \ingroup grp_res */
static io_res_t *io_port_res = NULL;

/** IO memory
 * \ingroup grp_res */
static io_res_t *io_mem_res = NULL;

/** Announced I/O memory resources.
 * \ingroup grp_res
 *
 * Announcements are kept in this list; i.e. current I/O memory mappings and
 * corresponding local addresses. */
typedef struct io_ares {
  struct io_ares *next;  /**< next in list */
  l4_addr_t start;       /**< begin of announced region */
  l4_addr_t end;         /**< size of announced region */
  l4_addr_t vaddr;       /**< address in io's address space */
  int       flags;       /**< flags */
} io_ares_t;

/** announced IO memory
 * \ingroup grp_res */
static io_ares_t *io_mem_ares = NULL;

/** DMA resources
 * \ingroup grp_res */
struct io_dma_res {
  int used;             /**< allocation flag */
  io_client_t *client;  /**< holder reference */
};

/** ISA DMA channels
 * \ingroup grp_res */
static struct io_dma_res isa_dma[8] = {
  {0, NULL},
  {0, NULL},
  {0, NULL},
  {0, NULL},
  {0, NULL},
  {0, NULL},
  {0, NULL},
  {0, NULL},
};

/** l4io self client structure reference */
static io_client_t *io_self;

/** BIOS32 service area 0xe0000-0xfffff */
static const l4_addr_t bios_paddr = 0xe0000;
static l4_addr_t bios_vaddr = 0;
static const l4_size_t bios_size  = 0x20000;

/** \name Generic Resource Manipulation
 *
 * @{ */

/** Find client structure. */
static io_client_t *find_client(l4_threadid_t tid)
{
  io_client_t *c;
  io_client_t tmp;

  tmp.c_l4id = tid;
  for (c = io_self; c; c = c->next)
    if (client_equal(c, &tmp))
      return c;

  return NULL;
}

/** Generic allocate region.
 * \ingroup grp_res
 */
static int __request_region(unsigned long start, unsigned long len,
                            unsigned long max, io_res_t ** root,
                            io_client_t * c)
{
  unsigned long end = start + len - 1;
  io_res_t *tmp = NULL, *s = *root,  /* successor */
  *p = NULL;                         /* predecessor */

  /* sanity checks */
  if (end < start)
    return -L4_EINVAL;
  if (end > max)
    return -L4_EINVAL;

  /* remember: s = *root */
  for (;;)
    {
      if (!s || (end < s->start))
        {
          LOGd(DEBUG_RES, "allocating (0x%08lx-0x%08lx) for "l4util_idfmt,
               start, end, l4util_idstr(c->c_l4id));

          tmp = malloc(sizeof(io_res_t));
          Assert(tmp);
          tmp->start = start;
          tmp->end = end;
          tmp->next = s;
          tmp->client = c;
          if (!p)
            /* new res is the first */
            *root = tmp;
          else
            p->next = tmp;
          return 0;
        }
      p = s;
      if (start > p->end)
        {
          s = p->next;
          continue;
        }
      LOGd(DEBUG_RES, "(0x%08lx-0x%08lx) not available for "l4util_idfmt"",
           start, end, l4util_idstr(c->c_l4id));
      return -L4_EBUSY;         /* no slot available */
    }
};

/** Generic search function.
 * \ingroup grp_res
 *
 * \param  addr   Address to search for
 * \param  root   List to look at
 * \retval start  Start address of region
 * \retval len    Length of region
 *
 * \returns 0 if a region could be found, <0 on error
 */
static int __search_region(unsigned long addr, io_ares_t * p,
                           unsigned long *start, unsigned long *len)
{
  while (p)
    {
      if (p->start <= addr && addr <= p->end)
        {
          *start = p->start;
          *len   = p->end - p->start + 1;
          return 0;
        }
      p = p->next;
    }
  return -L4_EINVAL;
}

/** Generic release region.
 * \ingroup grp_res
 */
static int __release_region(unsigned long start, unsigned long len,
                            io_res_t ** root, io_client_t * c)
{
  unsigned long end = start + len - 1;
  io_res_t *tmp = *root, *p = NULL;     /* predecessor */

  /* remember: tmp = *root */
  for (;;)
    {
      if (!tmp)
        break;
      if (tmp->end < start)
        {
          p = tmp;
          tmp = tmp->next;
          continue;
        }
      if ((tmp->start != start) || (tmp->end != end))
        break;
#if !IORES_TOO_MUCH_POLICY
      if (!client_equal(tmp->client, c))
        {
          LOGd(DEBUG_RES, l4util_idfmt" not allowed to free "
                          l4util_idfmt"'s region",
               l4util_idstr(c->c_l4id), l4util_idstr(tmp->client->c_l4id));
          return -L4_EPERM;
        }
#endif
      if (!p)
        *root = tmp->next;
      else
        p->next = tmp->next;
      LOGd(DEBUG_RES, "freeing (0x%08lx-0x%08lx)", start, end);
      free(tmp);
      return 0;
    }
  printf("Non-existent region (0x%08lx-0x%08lx) not freed\n", start, end);
  return -L4_EINVAL;
}

/** Generic release region for all regions of a specific client.
 * \ingroup grp_res
 */
static int __release_region_client(io_res_t ** root, io_client_t * c)
{
  io_res_t *tmp, *n, *p;     /* predecessor */

  for (tmp=*root, p=NULL; tmp;)
    {
      n = tmp->next;
      if (l4_tasknum_equal(tmp->client->c_l4id, c->c_l4id))
        {
          if (!p)
            *root = tmp->next;
          else
            p->next = tmp->next;
          LOGd(DEBUG_RES, "freeing (0x%08lx-0x%08lx)", tmp->start, tmp->end);
          free(tmp);
        }
      else
	p = tmp;
      tmp = n;
    }
  return 0;
}

/**
 * Split I/O port region into fpages
 *
 * \param regions  pointer to fpage array
 *
 * We split up arbitrary I/O port regions into chunks that can be
 * processed by Fiasco (-> size-aligned and the size is a power of 2) and
 * store these into the regions array.
 */
static void process_port_region(unsigned start, unsigned length,
                                l4_snd_fpage_t **regions, l4_size_t *num)
{
  unsigned log2_start, log2, log2_length;

  if (!length) return;

  /* find the largest suitable chunk, whose size is a power of 2, inside the
   * original region */
  for (log2 = l4util_log2(length); ; log2--)
    {
      log2_length = 1 << log2;
      unsigned size_mask = log2_length - 1;
      log2_start = (start + size_mask) & ~size_mask;
      if (log2_start + log2_length <= start + length) break;
    }

  /* process everything before the same way */
  process_port_region(start, log2_start - start, regions, num);

  /* now store fpage / unmap region */
  LOGd(DEBUG_RES, "I/O port chunk: 0x%04x-0x%04x [0x%04x]\n",
       log2_start, log2_start + log2_length - 1, log2_length);

  l4_fpage_t fpage = l4_iofpage(log2_start, log2, L4_FPAGE_MAP);

  (*regions)->snd_base = 0;
  (*regions)->fpage = fpage;
  (*regions)++;
  (*num)++;

  /* process everything behind the same way */
  unsigned last_start = log2_start + log2_length;
  process_port_region(last_start, start + length - last_start, regions, num);
}

/** @} */
/** \name Request/Release Interface Functions (IPC interface)
 *
 * Functions for system resource request and release.
 * @{ */

/** Request I/O port region.
 * \ingroup grp_res
 *
 * \param _dice_corba_obj   DICE corba object
 * \param addr              region start address
 * \param len               region length
 *
 * \retval _dice_corba_env  corba environment
 *
 * \return 0 on success, negative error code otherwise
 *
 * The requested region is checked for availability and mapped into client's
 * address space. Port regions are kept in a list.
 */
long
l4_io_request_region_component (CORBA_Object _dice_corba_obj,
                                l4_uint16_t addr,
                                l4_uint16_t len,
                                l4_size_t *num,
                                l4_snd_fpage_t regions[],
                                CORBA_Server_Environment *_dice_corba_env)
{
#ifndef ARCH_arm
  int err;

  /* set to 0 here to avoid copying an arbitrary number of elements,
   * when returning with an error code */
  *num = 0;

  /* size is 0 */
  if (!len) return -L4_EINVAL;

  /* out of range */
  if ((addr + len - 1) > MAX_IO_PORT) return -L4_EINVAL;

  io_client_t *c = find_client(*_dice_corba_obj);

  /* client not registered */
  if (!c) return -L4_ENOTFOUND;

  /* check availability */
  if ((err = __request_region(addr, len, MAX_IO_PORT, &io_port_res, c)))
    return err;

  LOGd(DEBUG_RES, "requested I/O ports: 0x%04x-0x%04x [0x%04x]\n",
       addr, addr + len - 1, len);

  l4_snd_fpage_t *r = regions;
  process_port_region(addr, len, &r, num);
  ASSERT(*num <= l4_io_max_fpages); /* XXX to be sure */

  /* ensure we posses the I/O ports */
  unsigned i;
  l4_threadid_t pager = rmgr_pager_id; /* FIXME should be sigma0 */
  for (i = 0; i < *num; i++)
    {
      l4_umword_t dw0, dw1;
      l4_msgdope_t result;
      int err = l4_ipc_call(pager,
                            L4_IPC_SHORT_MSG, regions[i].fpage.fpage, 0,
                            L4_IPC_IOMAPMSG(0, L4_WHOLE_IOADDRESS_SPACE), &dw0, &dw1,
                            L4_IPC_NEVER, &result);
      /* IPC error || no fpage received */
      if (err || !dw1)
          Panic("sigma0 request for I/O ports [%04x,%04x) failed (err=%d dw1=%ld)\n",
                 regions[i].fpage.iofp.iopage,
                 regions[i].fpage.iofp.iopage + regions[i].fpage.iofp.iosize,
                 err, dw1);
  }

  return 0;

#else
  return -L4_EINVAL;
#endif
}

/** Release I/O port region.
 * \ingroup grp_res
 *
 * \param _dice_corba_obj   DICE corba object
 * \param addr              region start address
 * \param len               region length
 *
 * \retval _dice_corba_env  corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
long
l4_io_release_region_component (CORBA_Object _dice_corba_obj,
                                l4_uint16_t addr,
                                l4_uint16_t len,
                                CORBA_Server_Environment *_dice_corba_env)
{
  l4_size_t num = 0;
  l4_snd_fpage_t regions[l4_io_max_fpages];
  l4_snd_fpage_t *r = regions;

  io_client_t *c = find_client(*_dice_corba_obj);

  /* client not registered */
  if (!c) return -L4_ENOTFOUND;

  process_port_region(addr, len, &r, &num);
  ASSERT(*num <= l4_io_max_fpages); /* XXX to be sure */

  /* unmap region -- XXX from *all* clients */
  unsigned i;
  for (i = 0; i < num; i++)
    l4_fpage_unmap(regions[i].fpage, L4_FP_FLUSH_PAGE | L4_FP_OTHER_SPACES);

  return (__release_region(addr, len, &io_port_res, c));
}

/** Request I/O memory region.
 * \ingroup grp_res
 *
 * \param _dice_corba_obj    DICE corba object
 * \param addr               region start address
 * \param len                region length
 * \param region             fpage descriptor for memory region
 *
 * \retval _dice_corba_env   corba environment
 *
 * \return 0 on success, negative error code otherwise
 *
 * The requested region is checked for availability and mapped into client's
 * address space. Any policy regarding memory region has to be checked in this
 * function.
 *
 * Memory regions are kept in a list.
 */
long
l4_io_request_mem_region_component (CORBA_Object _dice_corba_obj,
                                    unsigned long addr,
                                    unsigned long len,
                                    unsigned long flags,
                                    l4_snd_fpage_t *region,
                                    CORBA_Server_Environment *_dice_corba_env)
{
  int error, size;
  unsigned int start = addr;
  unsigned int end = addr + len - 1;
  l4_addr_t vaddr;
  io_client_t *c = find_client(*_dice_corba_obj);
  io_ares_t *p = io_mem_ares;

  /* init snd fpage with zero-sized fpage */
  region->snd_base = 0;
  region->fpage.raw = 0;

  if (!c)
    /* client not registered */
    return -L4_ENOTFOUND;

  /* find/check announcement */
  for (;;)
    {
      if (!p)
        {
          LOGdL(DEBUG_ERRORS, "requested (0x%08lx-0x%08lx) not announced",
                addr, addr + len - 1);
          return -L4_EINVAL;
        }
      if ((start >= p->start) && (end <= p->end))
        break;
      p = p->next;
    }

  /* check availability */
  if ((error = __request_region(addr, len, MAX_IO_MEMORY, &io_mem_res, c)))
    return error;

  size = len >> IO_LOG2_PAGESIZE;
  if (len > (size << IO_LOG2_PAGESIZE))
    size++;


  /* p->vaddr points to a io-page-aligned address even if addr doesn't start
   * there! */
  vaddr   = io_trunc_page(p->vaddr);

  /* build fpage - we can map the entire region */
  size = (len & IO_PAGEMASK) ? nLOG2(len) : IO_LOG2_PAGESIZE;

  region->snd_base = addr; /* the requested physical address */
  region->fpage = l4_fpage(vaddr, size, L4_FPAGE_RW, L4_FPAGE_MAP);

  switch (flags)
    {
    case L4IO_MEM_WRITE_COMBINED:
      region->fpage.fp.cache = L4_FPAGE_BUFFERABLE;
      break;
    case L4IO_MEM_CACHED:
      region->fpage.fp.cache = L4_FPAGE_CACHEABLE;
      break;
    default:
      region->fpage.fp.cache = L4_FPAGE_UNCACHEABLE;
      break;
    }

  LOGd(DEBUG_RES, "sending fpage {snd_base=%lx, 0x%08lx, 0x%08lx}",
       region->snd_base,
       (unsigned long)region->fpage.fp.page << 12,
       1UL << region->fpage.fp.size);

  /* done */
  return 0;
}

/** Search for I/O memory region.
 * \ingroup grp_res
 *
 * \param  _dice_corba_obj   DICE corba object
 * \param  addr              Address to search for
 *
 * \retval start             Start with memory region
 * \retval len               Length of memory region
 * \retval _dice_corba_env   corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
long
l4_io_search_mem_region_component (CORBA_Object _dice_corba_obj,
                                   unsigned long addr,
                                   unsigned long *start,
                                   l4_size_t *len,
                                   CORBA_Server_Environment *_dice_corba_env)
{
  return __search_region(addr, io_mem_ares,
                         (unsigned long *)start, (unsigned long *)len);
}

/** Release I/O memory region.
 * \ingroup grp_res
 *
 * \param _dice_corba_obj   DICE corba object
 * \param addr              region start address
 * \param len               region length
 *
 * \retval _dice_corba_env  corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
long
l4_io_release_mem_region_component (CORBA_Object _dice_corba_obj,
                                    unsigned long addr,
                                    unsigned long len,
                                    CORBA_Server_Environment *_dice_corba_env)
{
  int error, size;
  io_client_t *c = find_client(*_dice_corba_obj);
  unsigned int start = addr;
  unsigned int end = addr + len - 1;
  io_ares_t *p = io_mem_ares;
  l4_fpage_t region;

  if (!c)
    /* client not registered */
    return -L4_ENOTFOUND;

  if ((error = __release_region(addr, len, &io_mem_res, c)))
    return error;

  /* find announcement */
  for (;;)
    {
      if ((start >= p->start) && (end <= p->end))
        break;
      p = p->next;
    }

  /* build fpage for region */
  size = (len & IO_PAGEMASK) ? nLOG2(len) : IO_LOG2_PAGESIZE;
  region = l4_fpage(io_trunc_page(p->vaddr), size, 0, 0);

  /* unmap region */
  l4_fpage_unmap(region, L4_FP_FLUSH_PAGE | L4_FP_OTHER_SPACES);

  /* done */
  return 0;
}

/** Request ISA DMA Channel
 * \ingroup grp_res
 *
 * \param _dice_corba_obj   DICE corba object
 * \param channel           ISA DMA channel
 *
 * \retval _dice_corba_env  corba environment
 *
 * \return 0 on success, negative error code otherwise
 *
 * \todo Implementation completion! For now it's deferred.
 */
long
l4_io_request_dma_component (CORBA_Object _dice_corba_obj,
                             unsigned long channel,
                             CORBA_Server_Environment *_dice_corba_env)
{
  io_client_t *c = find_client(*_dice_corba_obj);

  if (!c)
    /* client not registered */
    return -L4_ENOTFOUND;

  /* sanity checks */
  if (!(channel < MAX_ISA_DMA))
    return -L4_EINVAL;
  if (isa_dma[channel].used)
    return -L4_EINVAL;

  /* allocate it */
  isa_dma[channel].used = 1;
  isa_dma[channel].client = c;

  return 0;
}

/** Release ISA DMA Channel.
 * \ingroup grp_res
 *
 * \param _dice_corba_obj   DICE corba object
 * \param channel           ISA DMA channel
 *
 * \retval _dice_corba_env  corba environment
 *
 * \return 0 on success, negative error code otherwise
 *
 * \todo Implementation completion! For now it's deferred.
 */
long
l4_io_release_dma_component (CORBA_Object _dice_corba_obj,
                             unsigned long channel,
                             CORBA_Server_Environment *_dice_corba_env)
{
  io_client_t *c = find_client(*_dice_corba_obj);

  if (!c)
    /* client not registered */
    return -L4_ENOTFOUND;

  /* sanity checks */
  if (!(channel < MAX_ISA_DMA))
    return -L4_EINVAL;
  if (!isa_dma[channel].used)
    return -L4_EINVAL;
#if !IORES_TOO_MUCH_POLICY
  if (!client_equal(isa_dma[channel].client, c))
    {
      LOGd(DEBUG_RES, l4util_idfmt" not allowed to release "
                      l4util_idfmt"'s DMA channel",
           l4util_idstr(c->c_l4id), 
           l4util_idstr(isa_dma[channel].client->c_l4id));
      return -L4_EPERM;
    }
#endif
  /* release it */
  isa_dma[channel].used = 0;
  isa_dma[channel].client = NULL;

  return 0;
}

/** Release all regions of a client.
 * \ingroup grp_res
 *
 * \param _dice_corba_obj   DICE corba object
 * \param channel           ISA DMA channel
 *
 * \retval _dice_corba_env  corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
long
l4_io_release_client_component (CORBA_Object _dice_corba_obj,
                                const l4_threadid_t *client,
                                CORBA_Server_Environment *_dice_corba_env)
{
  io_client_t *c = find_client(*client);
  l4_uint32_t channel;

  if (!c)
    return -L4_ENOTFOUND;

  __release_region_client(&io_mem_res, c);
  __release_region_client(&io_port_res, c);
  for (channel=0; channel<MAX_ISA_DMA; channel++)
    {
      if (isa_dma[channel].used &&
	  client_equal(isa_dma[channel].client, c))
        {
          /* release it */
          isa_dma[channel].used = 0;
          isa_dma[channel].client = NULL;
        }
    }

  return 0;
}

/** @} */
/** \name Region Specific Interface Functions (internal callbacks)
 *
 * Functions for system resource request, release, and announcement.
 *
 * \krishna We assume single-threading here and above!
 *
 * \todo Rethink release() and implement if appropriate.
 * @{ */

/** Request I/O port region.
 * \ingroup grp_res
 */
int callback_request_region(unsigned long addr, unsigned long len)
{
  io_client_t *c = io_self;

  return (__request_region(addr, len, MAX_IO_PORT, &io_port_res, c));
}

/** Request I/O memory region.
 * \ingroup grp_res
 */
int callback_request_mem_region(unsigned long addr, unsigned long len)
{
  io_client_t *c = io_self;

  return (__request_region(addr, len, MAX_IO_MEMORY, &io_mem_res, c));
}

/** Announce I/O memory region.
 * \ingroup grp_res
 *
 * On region announcement I/O has to request the given physical memory region
 * from an appropriate pager.
 *
 * \krishna Hopefully we'll get no doubled announcements - I don't check.
 *
 * \todo sanity checks
 */
void callback_announce_mem_region(unsigned long addr, unsigned long len)
{
  int error;
  l4_addr_t vaddr = 0;
  l4_uint32_t vaddr_area;
  l4_size_t size;

  io_ares_t **s = &io_mem_ares;
  io_ares_t *p = io_mem_ares;
  l4_threadid_t pager = SIGMA0_ID;

  /* reserve area */
  size = len >> IO_REQ_LOG2_PAGESIZE;
  if (len > (size << IO_REQ_LOG2_PAGESIZE))
    size++;

  /* check for earlier mapping of region */
  while (p)
    {
      if (io_req_trunc_page(p->start) <= addr
          && io_req_trunc_page(p->end + IO_REQ_PAGESIZE -1) - 1 >= addr + len - 1)
        break;
      p = p->next;
    }

  if (p)
    {
      /* area was mapped before */
      LOGd(DEBUG_RES, "reuse mapping from (0x%08lx-0x%08lx); mapped to 0x%08lx",
           p->start, p->end, p->vaddr);
      vaddr = io_req_trunc_page(p->vaddr);
    }


  else
    {
      /* map area */
      error = l4rm_area_reserve(size * IO_REQ_PAGESIZE, L4RM_LOG2_ALIGNED,
                                &vaddr, &vaddr_area);
      if (error)
        {
          Panic("no area for memory region announcement (%d)\n", error);
        }

      /* request sigma0 mappings to area */
      if ((error = l4sigma0_map_iomem(pager, io_req_trunc_page(addr),
                                      vaddr, size * IO_REQ_PAGESIZE, 0)))
        {
          switch (error)
            {
              case -2: Panic("sigma0 request IPC error");
              case -3: Panic("sigma0 request for phys addr %08lx failed", addr);
            }
        }
    }

  /* consider in-page offset */
  vaddr += addr & (IO_REQ_PAGESIZE - 1);

  /* new announced regions list entry */
  while (*s) s = &((*s)->next);

  p = malloc(sizeof(io_ares_t));
  Assert(p);
  p->next = NULL;
  p->start = addr;
  p->end = addr + len - 1;
  p->vaddr = vaddr;
  p->flags = 0;

  *s = p;

  LOGd(DEBUG_RES, "(0x%08lx-0x%08lx) was announced; mapped to 0x%08lx",
       p->start, p->end, p->vaddr);
}

static struct device_inclusion_list
{
  unsigned short vendor;
  unsigned short device;
  struct device_inclusion_list *next;
} *device_handle_inclusion_list,  /* if nonempty, the device must be
                                     listed here to be handled. */
  *device_handle_exclusion_list;  /* if the above is empty, the device
                                     must not be listed here to be
                                     taken care of. */

/** Check if we should handle this specific PCI device
 * \ingroup grp_res
 *
 * This is checked against the parameters the user provided on startup.
 *
 * \retval 1  yes, we should allocated/handle this device
 * \retval 0  no, do not handle this device
 */
int callback_handle_pci_device(unsigned short vendor, unsigned short device)
{
  struct device_inclusion_list *list;

  if (device_handle_inclusion_list)
    {
      for (list=device_handle_inclusion_list; list; list=list->next)
        if (list->vendor==vendor && list->device==device)
          return 1;
      return 0;
    }
  for (list=device_handle_exclusion_list; list; list=list->next)
    if (list->vendor==vendor && list->device==device)
      return 0;
  return 1;
}

/** parse a 'vendor:device' pair into nums
 *
 * \retval 0 - ok, invalid format else
 */
static int parse_device_pair(const char *s, unsigned short *vendor,
                             unsigned short *device)
{
  char *t;
  *vendor = strtoul(s, &t, 16);
  if (*t != ':')
    return -L4_EINVAL;
  s = t + 1;
  *device = strtoul(s, &t, 16);
  if (*t != 0)
     return -L4_EINVAL;
  return 0;
}

/** add a new inclusion-entry
 *
 * \retval 0 ok, l4env-error-code else
 */
int add_device_inclusion(const char *s)
{
  static unsigned short vendor, device;
  static struct device_inclusion_list *elem;

  if (!parse_device_pair(s, &vendor, &device))
    {
      if ((elem=malloc(sizeof(struct device_inclusion_list)))==0)
        return -L4_ENOMEM;

      elem->vendor = vendor;
      elem->device = device;
      elem->next = device_handle_inclusion_list;
      device_handle_inclusion_list = elem;
      LOG("taking care of device %04x:%04x\n", elem->vendor, elem->device);
      return 0;
    }
  return -L4_EINVAL;
}

/** add a new inclusion-entry
 *
 * \retval 0           ok
 * \retval -L4_EINVAL  invalid format in parameter
 * \retval -L4_ENOMEM  out of mem
 */
int add_device_exclusion(const char *s)
{
  static unsigned short vendor, device;
  static struct device_inclusion_list *elem;

  if (!parse_device_pair(s, &vendor, &device))
    {
      if ((elem=malloc(sizeof(struct device_inclusion_list)))==0)
        return -L4_ENOMEM;

      elem->vendor = vendor;
      elem->device = device;
      elem->next = device_handle_exclusion_list;
      device_handle_exclusion_list = elem;
      LOG("ignoring device %04x:%04x\n", elem->vendor, elem->device);

      return 0;
    }
  return -L4_EINVAL;
}

#ifndef ARCH_arm
/** Map the BIOS32 service area
 *
 * \param vaddr  virtual address on successfull mapping (undefined on errors!)
 *
 * \return 0 on success, negative error code otherwise
 *
 * As L4RM does no implicit pf propagation anymore we need to explicitly map
 * the BIOS32 service area for lib-pci.
 */
int bios_map_area(unsigned long *ret_vaddr)
{
  int error;
  l4_umword_t size  = bios_size;
  l4_umword_t vaddr;
  l4_umword_t paddr = bios_paddr;
  l4_umword_t dw0, dw1;
  l4_msgdope_t result;
  l4_threadid_t pager = rmgr_pager_id;
  l4_uint32_t vaddr_area;  /* ??? */

  /* reserve area at L4RM */
  error = l4rm_area_reserve(size, L4RM_LOG2_ALIGNED,
                            &vaddr, &vaddr_area);
  if (error)
    Panic("no area for BIOS32 (%d)\n", error);

  *ret_vaddr = bios_vaddr = vaddr;

  /* request memory from sigma0/RMGR */
  while (size)
    {
      error = l4_ipc_call(pager,
                          L4_IPC_SHORT_MSG, paddr, 0,
                          L4_IPC_MAPMSG(vaddr, L4_LOG2_PAGESIZE), &dw0, &dw1,
                          L4_IPC_NEVER, &result);
      /* IPC error || no fpage received */
      if (error || !dw1)
          Panic("sigma0 request for phys addr %p failed (err=%d dw1=%ld)\n",
                (void*)paddr, error, dw1);

      paddr += L4_PAGESIZE;
      vaddr += L4_PAGESIZE;
      size  -= L4_PAGESIZE;
    }
  return 0;
}
#endif

/** BIOS32 service area-specific address translation
 *
 * \param paddr  physical address
 *
 * \return corresponding virtual address, or NULL on error
 */
void * bios_phys_to_virt(unsigned long paddr)
{
  if ((paddr >= bios_paddr) && (paddr < bios_paddr+bios_size))
    return (void*)(bios_vaddr + (paddr - bios_paddr));
  else
    {
//      Panic("BIOS32 address mapping unknown for %p; area is %p-%p",
//            (void*)paddr, (void*)bios_paddr, (void*)(bios_paddr+bios_size-1));
      return NULL;
    }
}

/** @} */
/** Resource Module Initialization.
 * \ingroup grp_res
 *
 * \param c  l4io self client structure reference
 *
 * \return 0 on success, negative error code otherwise
 *
 * Initialize reserved resources: IRQ I/O ports, ...
 *
 * \krishna This list is not complete ...
 */
int io_res_init(io_client_t *c)
{
#ifndef ARCH_arm
  int err;

  /* save self reference */
  io_self = c;

  /* DMA controller #1 */
  if ((err = __request_region(0, 0x20, MAX_IO_PORT, &io_port_res, c)))
    goto err;
  /* DMA controller #2 */
  if ((err = __request_region(0xc0, 0x20, MAX_IO_PORT, &io_port_res, c)))
    goto err;
  /* DMA page regs */
  /* XXX I/O port 0x80 may be used for io_delay in clients */
  if ((err = __request_region(0x80, 0x10, MAX_IO_PORT, &io_port_res, c)))
    goto err;

  /* PIC #1 */
  if ((err = __request_region(0x20, 0x20, MAX_IO_PORT, &io_port_res, c)))
    goto err;
  /* PIC #2 */
  if ((err = __request_region(0xa0, 0x20, MAX_IO_PORT, &io_port_res, c)))
    goto err;

  /* Timer */
  if ((err = __request_region(0x40, 0x20, MAX_IO_PORT, &io_port_res, c)))
    goto err;

  /* FPU */
  if ((err = __request_region(0xf0, 0x10, MAX_IO_PORT, &io_port_res, c)))
    goto err;

  /* allocate ISA DMA CASCADE */
  isa_dma[4].used = 1;
  isa_dma[4].client = c;

  return 0;

err:
  Panic("claiming reserved resource failed (%d)\n", err);
  return err;

#else
  return 0;
#endif
}

/*
 * DEBUGGING functions
 */
static void list_regions(void)
{
  io_res_t *p = io_port_res;

  while (p)
    {
      printf("  port (0x%04lx - 0x%04lx)          "l4util_idfmt" %s\n",
             p->start, p->end,
             l4util_idstr(p->client->c_l4id), p->client->name);
      p = p->next;
    }
}

static void list_mem_regions(void)
{
  io_res_t *p = io_mem_res;

  while (p)
    {
      printf("memory (0x%08lx - 0x%08lx)  "l4util_idfmt" %s\n",
             p->start, p->end,
             l4util_idstr(p->client->c_l4id), p->client->name);
      p = p->next;
    }
}

static void list_amem_regions(void)
{
  io_ares_t *p = io_mem_ares;

  while (p)
    {
      printf("memory (0x%08lx - 0x%08lx)  ANNOUNCED (0x%08lx)\n",
             p->start, p->end, p->vaddr);
      p = p->next;
    }
}

static void list_dma(void)
{
  int i;

  for (i = 0; i < MAX_ISA_DMA; i++)
    if (isa_dma[i].used)
      printf("   DMA  %d                         "l4util_idfmt" %s\n",
           i, l4util_idstr(isa_dma[i].client->c_l4id), isa_dma[i].client->name);
    else
      printf("   DMA  %d                         "l4util_idfmt" UNUSED\n",
           i, l4util_idstr(L4_NIL_ID));
}

void list_res(void)
{
  list_regions();
  list_amem_regions();
  list_mem_regions();
  list_dma();
}
