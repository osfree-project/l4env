#include <stdio.h>
#include <stdlib.h>

#include <l4/sys/types.h>
#include <l4/crtx/crt0.h>
#include <l4/env/errno.h>
#include <l4/sys/consts.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/l4rm/l4rm.h>
#include <l4/thread/thread.h>
#include <l4/util/l4_macros.h>
#include <l4/util/mb_info.h>
#include <l4/util/bitops.h>
#include <l4/dm_generic/dm_generic-server.h>
#include <l4/dm_mem/dm_mem-server.h>
#include <l4/dm_mem/dm_mem.h>

#include "dm.h"

#define MAX_DS		64
#define DEBUG		0

typedef struct
{
  l4_addr_t	addr;
  l4_size_t	size;
} ds_desc_t;

static ds_desc_t dataspaces[MAX_DS];
static l4_threadid_t dm_id;

void * /* TAG:preallocfix */
CORBA_alloc(unsigned long size)
{
  return 0;
}

void
CORBA_free(void *b)
{
}

/**
 * Allocate dataspace descriptor
 * \return dataspace descriptor index, -1 if no descriptor available
 */
static int
desc_alloc(void)
{
  int id;

  /* search free dataspace descriptor */
  for (id = 0; id < MAX_DS; id++)
    if (dataspaces[id].addr == -1)
      break;

  if (id == MAX_DS)
    {
      /* no descriptor available */
      LOG_Error("no descriptor");
      return -1;
    }

  return id;
}

/**
 * Return dataspace descriptor
 * \param  id            Dataspace id
 * \return descriptor, NULL if invalid id
 */
static ds_desc_t *
get_desc(int id)
{
  /* check id */
  if (id >= MAX_DS || dataspaces[id].addr == -1)
    {
      /* invalid dataspace id */
      LOG_Error("invalid id (%d)",id);
      return NULL;
    }

  /* done */
  return dataspaces + id;
}

/**
 * Return a new dataspace including the image of a L4 boot module
 *
 * \param  request       Flick request structure
 * \param  fname         Module file name
 * \param  flags         Flags (unused)
 * \retval ds            Dataspace including the file image
 * \retval size          Dataspace size
 * \retval _ev           Flick exception structure
 *
 * \return 0 on success, error code otherwise:
 *         - \c -L4_ENOENT     no such file
 *         - \c -L4_ENOHANDLE  no dataspace id left
 *         - \c -L4_EINVAL     invalid file
 */
l4_int32_t
dm_open(const char* fname, l4_uint32_t flags, l4_threadid_t mem_dm_id,
        l4_threadid_t client, l4dm_dataspace_t *ds, l4_size_t *ds_size)
{
  l4util_mb_info_t *mbi = (l4util_mb_info_t*)(l4_addr_t)crt0_multiboot_info;
  l4util_mb_mod_t  *mod = (l4util_mb_mod_t*)(l4_addr_t)mbi->mods_addr;
  int i, found;
  const char *fname_file = strrchr(fname, '/');

  if (!fname_file)
    fname_file = fname;
  else
    fname_file++;

  for (i=0; i<mbi->mods_count; i++)
    {
      char *f;

      if ((f = strrchr((const char*)(l4_addr_t)mod[i].cmdline, '/')))
        found = !strcmp(f + 1, fname_file);
      else
        found = !strcmp((const char*)(l4_addr_t)mod[i].cmdline, fname_file);

      if (found)
        {
	  l4_addr_t addr = mod[i].mod_start;
	  l4_addr_t size = mod[i].mod_end-mod[i].mod_start;
	  void      *ds_addr;
	  int rc;

	  *ds_size = size;

	  if (flags & L4DM_PINNED)
	    {
	      /* We cannot guarantee pinned memory for dataspaces we will
	       * potentially share (no selective unmap operation). So just
	       * the content to a dataspace of pinned memory */
	      if ((rc = l4dm_mem_open(mem_dm_id, size, 0, flags,
		                      "bmodfs ds", ds)) < 0)
		{
		  LOG("Cannot create dataspace for pinned memory");
		  return rc;
		}

	      if ((rc = l4rm_attach(ds, size, 0, L4DM_RW | L4RM_MAP,
		                    &ds_addr)))
		{
		  LOG("Cannot attach dataspace");
		  l4dm_close(ds);
		  return rc;
		}

	      memcpy(ds_addr, (void*)addr, size);

	      l4rm_detach(ds_addr);

	      if ((rc = l4dm_transfer(ds, client)))
		{
		  LOG("Cannot transfer ownership of dataspace to client");
		  l4dm_close(ds);
		  return rc;
		}

	      return 0;
	    }
	  else
	    {
	      int id;
	      ds_desc_t *dd;

	      id = desc_alloc();
	      if (id < 0)
		return -L4_ENOHANDLE;

	      dd          = dataspaces + id;
	      dd->addr    = addr;
	      dd->size    = size;
	      ds->id      = id;
	      ds->manager = dm_id;
	      return 0;
	    }
        }
    }

  LOG_Error("Object '%s' not found!", fname);

  *ds = L4DM_INVALID_DATASPACE;
  return -L4_ENOTFOUND;
}

/**
 * Map dataspace region.
 *
 * \param  request       Flick request structure
 * \param  ds_id         Dataspace id
 * \param  offset        Offset in dataspace
 * \param  size          Dataspace region size
 * \param  rcv_size2     Receive window size
 * \param  rcv_offs      Offset in receive window
 * \param  flags         Flags
 * \retval page          Fpage descriptor
 * \retval _ev           Flick excetion structure, unused
 *
 * \return 0 on success (created fpage), error code otherwise:
 *         - \c -L4_EINVAL       invalid dataspace id or region
 *         - \c -L4_EINVAL_OFFS  offset points beyobnd end of dataspace
 *         - \c -L4_EPERM        write access denied
 */
long
if_l4dm_generic_map_component (CORBA_Object _dice_corba_obj,
                               unsigned long ds_id,
                               unsigned long offset,
                               unsigned long size,
                               unsigned long rcv_size2,
                               unsigned long rcv_offs,
                               unsigned long flags,
                               l4_snd_fpage_t *page,
                               CORBA_Server_Environment *_dice_corba_env)
{
  ds_desc_t *dd = get_desc(ds_id);

  if (dd == NULL)
    return -L4_EINVAL;

  if (flags & L4DM_WRITE)
    {
      LOG_Error("write map request, client "l4util_idfmt" at 0x%08lx",
	      l4util_idstr(*_dice_corba_obj), offset);
      return -L4_EPERM;
    }

  if (offset > dd->size)
    {
      LOG_Error("invalid offset 0x%08lx", offset);
      return -L4_EINVAL_OFFS;
    }

  if (size > L4_PAGESIZE && !(flags & L4DM_MAP_PARTIAL))
    /* we cannot map large fpages! */
    return -L4_EINVAL;

  page->snd_base = rcv_offs;
  page->fpage    = l4_fpage(dd->addr + offset,
                            L4_LOG2_PAGESIZE,L4_FPAGE_RO,L4_FPAGE_MAP);

  return 0;
}

/**
 * Handle dataspace fault
 *
 * \param  request       Flick request structure
 * \param  ds_id         Dataspace id
 * \param  offset        Offset in dataspace
 * \retval page          Fpage descriptor
 * \retval _ev           Flick exception structure (unused)
 *
 * \return 0 on succes (created fpage), error code otherwise:
 *         - \c -L4_EINVAL       invalid dataspace id
 *         - \c -L4_EINVAL_OFFS  offset points beyond end of file
 */
long
if_l4dm_generic_fault_component (CORBA_Object _dice_corba_obj,
                                 unsigned long ds_id,
                                 unsigned long offset,
                                 l4_snd_fpage_t *page,
                                 CORBA_Server_Environment *_dice_corba_env)
{
  int rw = offset & 2;
  ds_desc_t * dd = get_desc(ds_id);

  if (dd == NULL)
    return -L4_EINVAL;

  if (rw)
    {
      LOG_Error("write pagefault "l4util_idfmt" at 0x%08lx",
		l4util_idstr(*_dice_corba_obj), offset & ~2);
      return -L4_EINVAL;
    }

  if (offset > dd->size)
    {
      LOG_Error("invalid offset 0x%08lx", offset);
      return -L4_EINVAL_OFFS;
    }

  page->snd_base = 0;
  page->fpage    = l4_fpage(dd->addr + offset, L4_LOG2_PAGESIZE,
                            L4_FPAGE_RO, L4_FPAGE_MAP);

  return 0;
}

/**
 * Close generic dataspace
 *
 * \param  request       Flick request structure
 * \param  ds_id         Dataspace id
 * \retval _ev           Flick exception structure (unused)
 *
 * \return 0 on succes, \c -L4_EINVAL if invalid dataspace id
 */
long
if_l4dm_generic_close_component (CORBA_Object _dice_corba_obj,
                                 unsigned long ds_id,
                                 CORBA_Server_Environment *_dice_corba_env)
{
  ds_desc_t * dd = get_desc(ds_id);
  int addr_align, log2_size, fpage_size;

  if (dd == NULL)
    return -L4_EINVAL;

  dd->size = l4_round_page(dd->size);
  while (dd->size > 0)
    {
      /* calculate the largest fpage we can unmap at address addr,
       * it depends on the alignment of addr and of the size */
      addr_align = (dd->addr == 0) ? 32 : l4util_bsf(dd->addr);
      log2_size  = l4util_bsr(dd->size);
      fpage_size = (addr_align < log2_size) ? addr_align : log2_size;

      LOGd(DEBUG, "align: addr %d, size %d, fpage %d",
	   addr_align, log2_size, fpage_size);

      /* selective unmap page from client */
      l4_fpage_unmap(l4_fpage(dd->addr, fpage_size, 0, 0),
     	             (_dice_corba_obj->id.task << 8) |
                     L4_FP_FLUSH_PAGE | L4_FP_OTHER_SPACES);

      dd->addr += (1UL << fpage_size);
      dd->size -= (1UL << fpage_size);
    }

  dd->addr = -1;
  return 0;
}

/**
 * Close all dataspace of a client, not supported.
 */
long
if_l4dm_generic_close_all_component (CORBA_Object _dice_corba_obj,
                                     const l4_threadid_t *client,
                                     unsigned long flags,
                                     CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_ENOTSUPP;
}

/**
 * Share dataspace
 *
 * \param  request       Flick request structure
 * \param  ds_id         Dataspace id
 * \param  client        New client
 * \param  flags         Access rights
 * \retval _ev           Flick exception structure, unused
 *
 * \return 0 on success, \c -L4_EINVAL if invalid dataspace id
 */
long
if_l4dm_generic_share_component (CORBA_Object _dice_corba_obj,
                                 unsigned long ds_id,
                                 const l4_threadid_t *client,
                                 unsigned long flags,
                                 CORBA_Server_Environment *_dice_corba_env)
{
  ds_desc_t * dd = get_desc(ds_id);

  if (dd == NULL)
    return -L4_EINVAL;

  /* allow everything */
  return 0;
}

/**
 * Revoke access rights
 *
 * \param  request       Flick request structure
 * \param  ds_id         Dataspace id
 * \param  client        Client thread id
 * \param  flags         Access rights
 * \retval _ev           Flick exception structure
 *
 * \return 0 on success, \c -L4_EINVAL if invalid dataspace id
 */
long
if_l4dm_generic_revoke_component (CORBA_Object _dice_corba_obj,
                                  unsigned long ds_id,
                                  const l4_threadid_t *client,
                                  unsigned long flags,
                                  CORBA_Server_Environment *_dice_corba_env)
{
  ds_desc_t * dd = get_desc(ds_id);

  if (dd == NULL)
    return -L4_EINVAL;

  /* allow everything */
  return 0;
}

/**
 * Check access rights
 *
 * \param  request       Flick request structure
 * \param  ds_id         Dataspace id
 * \param  flags         Access rights
 * \retval _ev           Flick exception structure
 *
 * \return 0 on success, \c -L4_EINVAL if invalid dataspace id
 */
long
if_l4dm_generic_check_rights_component (CORBA_Object _dice_corba_obj,
                                        unsigned long ds_id,
                                        unsigned long flags,
                                        CORBA_Server_Environment *_dice_corba_env)
{
  ds_desc_t * dd = get_desc(ds_id);

  if (dd == NULL)
    return -L4_EINVAL;

  /* only allow read */
  if (flags & (L4DM_RW|L4DM_RESIZE))
    return -L4_EPERM;

  return 0;
}

/**
 * Transfer ownership
 *
 * \param  request       Flick request structure
 * \param  ds_id         Dataspace id
 * \param  new_owner     New owner
 * \param  _ev           Flick exception structur0
 *
 * \return 0 on success, \c -L4_EINVAL if invalid dataspace id
 */
long
if_l4dm_generic_transfer_component (CORBA_Object _dice_corba_obj,
                                    unsigned long ds_id,
                                    const l4_threadid_t *new_owner,
                                    CORBA_Server_Environment *_dice_corba_env)
{
  ds_desc_t * dd = get_desc(ds_id);

  if (dd == NULL)
    return -L4_EINVAL;

  /* we don't care about ownership */
  return 0;
}

/**
 * Create dataspace copy, not supported
 */
long
if_l4dm_generic_copy_component (CORBA_Object _dice_corba_obj,
                                unsigned long ds_id,
                                unsigned long src_offs,
                                unsigned long dst_offs,
                                unsigned long num,
                                unsigned long flags,
                                const char* name,
                                l4dm_dataspace_t *copy,
                                CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_ENOTSUPP;
}

/**
 * Set dataspace name, not supported
 */
long
if_l4dm_generic_set_name_component (CORBA_Object _dice_corba_obj,
                                    unsigned long ds_id,
                                    const char* name,
                                    CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_ENOTSUPP;
}

/**
 * Get dataspace name, not supported
 */
long
if_l4dm_generic_get_name_component (CORBA_Object _dice_corba_obj,
                                    unsigned long ds_id,
                                    char* *name,
                                    CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_ENOTSUPP;
}

/**
 * Show dataspace info, not supported
 */
long
if_l4dm_generic_show_ds_component (CORBA_Object _dice_corba_obj,
                                   unsigned long ds_id,
                                   CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_ENOTSUPP;
}

/*
 * Request info about specific dataspace, not supported
 */
long
if_l4dm_mem_info_component (CORBA_Object _dice_corba_obj,
                            unsigned long ds_id,
                            l4_size_t *size,
                            l4_threadid_t *owner,
                            char* *name,
                            l4_uint32_t *next_id,
                            CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_ENOTSUPP;
}

/**
 * List dataspaces, not supported
 */
void
if_l4dm_generic_list_component (CORBA_Object _dice_corba_obj,
                                const l4_threadid_t *owner,
                                unsigned long flags,
                                CORBA_Server_Environment *_dice_corba_env)
{
  printf("not implemented\n");
  return;
}


long
if_l4dm_mem_open_component (CORBA_Object _dice_corba_obj,
                            unsigned long size,
                            unsigned long align,
                            unsigned long flags,
                            const char* name,
                            l4dm_dataspace_t *ds,
                            CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_ENOTSUPP;
}

long
if_l4dm_mem_size_component (CORBA_Object _dice_corba_obj,
                            unsigned long ds_id,
                            l4_size_t *size,
                            CORBA_Server_Environment *_dice_corba_env)
{
  ds_desc_t * dd = get_desc(ds_id);

  if (dd == NULL)
    return -L4_EINVAL;

  *size = dd->size;
  return 0;
}


long
if_l4dm_mem_resize_component (CORBA_Object _dice_corba_obj,
                              unsigned long ds_id,
                              unsigned long new_size,
                              CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_ENOTSUPP;
}

long
if_l4dm_mem_phys_addr_component (CORBA_Object _dice_corba_obj,
                                 unsigned long ds_id,
                                 unsigned long offset,
                                 l4_size_t size,
                                 unsigned long *paddr,
                                 l4_size_t *psize,
                                 CORBA_Server_Environment *_dice_corba_env)
{
  ds_desc_t * dd = get_desc(ds_id);

  if (dd == NULL)
    return -L4_EINVAL;

  if (offset > dd->size)
    return -L4_EINVAL_OFFS;

  *paddr = dd->addr + offset;
  *psize = dd->size - offset;
  if (size != L4DM_WHOLE_DS && *psize > size)
    *psize = size;
  return 0;
}

long
if_l4dm_mem_is_contiguous_component (CORBA_Object _dice_corba_obj,
                                     unsigned long ds_id,
                                     long *is_cont,
                                     CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_ENOTSUPP;
}

long
if_l4dm_mem_lock_component (CORBA_Object _dice_corba_obj,
                            unsigned long ds_id,
                            unsigned long offset,
                            unsigned long size,
                            CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_ENOTSUPP;
}

long
if_l4dm_mem_unlock_component (CORBA_Object _dice_corba_obj,
                              unsigned long ds_id,
                              unsigned long offset,
                              unsigned long size,
                              CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_ENOTSUPP;
}

static void
dm_if_thread(void *dummy)
{
  if_l4dm_mem_server_loop(0);
}

void
dm_start(void)
{
  int i;
  l4thread_t t;

  for (i = 0; i < MAX_DS; i++)
    dataspaces[i].addr = -1;

  if ((t = l4thread_create_named(dm_if_thread, ".dm", 0,
                                 L4THREAD_CREATE_ASYNC)) < 0)
    {
      printf("Error %d (%s) creating dataspace manager thread\n",
             t, l4env_errstr(t));
      exit(-2);
    }

  dm_id = l4thread_l4_id(t);
}
