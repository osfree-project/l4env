/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux26/lib/src/slab.c
 * \brief  Very simple emulation of linux slabs (kmem_cache)
 *
 * \author Marek Menzer <mm19@os.inf.tu-dresden.de>
 *
 * Original by Gerd Grie�bach <gg5@os.inf.tu-dresden.de> and
 *             Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/** \ingroup mod_mm
 * \defgroup mod_mm_slab Slab Caches
 *
 * This simple slab cache emulation was introduced for the Linux USB drivers.
 * There are some limitations/constraints:
 *
 * - the size of slab-objects is restricted to L4_PAGESIZE (because of l4slab)
 * - name, flags, dtor of slab caches are ignored
 *
 * Requirements: (additionally to \ref pg_req)
 *
 * - L4Env slab library (libslab.a)
 */

/* L4 */
#include <l4/slab/slab.h>
#include <l4/log/l4log.h>
#include <l4/dm_mem/dm_mem.h>

#include <l4/dde_linux/dde.h>

/* Linux */
#include <linux/slab.h>

/* local */
#include "__config.h"
#include "internal.h"

/** Grow slab cache (allocate new memory for slabs)
 *  - one page (L4_PAGESIZE) will be added
 */
static void * alloc_grow (l4slab_cache_t * cache, void **data)
{
  void *memp;

  LOGd_Enter(DEBUG_SLAB);

  if (!(memp = l4dm_mem_allocate (L4_PAGESIZE, L4DM_PINNED | L4RM_MAP)))
    {
      Panic ("dde: kmem_caches can't grow");
    }

  return memp;
}

#if 0
void alloc_release(l4slab_cache_t *cache, void *page, void *data)
{
//printf("alloc_release\n");
 l4dm_mem_release(page);
}
#endif

#define CACHE_NAMELEN 20  /** max name length for a slab cache */

/** local kmem_cache to L4 slab_cache mapping */
struct kmem_cache_s
{
  l4slab_cache_t *l4slab_cache;

  /* constructor func */
  void (*ctor)(void *, kmem_cache_t *, unsigned long);
  /* de-constructor func */
  void (*dtor)(void *, kmem_cache_t *, unsigned long);

  char name[CACHE_NAMELEN];
};


/** Create slab cache
 * \ingroup mod_mm_slab
 *
 * It's from mm/slab.c
 *
 * constraints:
 * - max size L4_PAGESIZE
 * - name, flags, dtor are ignored
 */
kmem_cache_t * kmem_cache_create (const char *name, size_t size,
                                  size_t offset, unsigned long flags,
                                  void (*ctor) (void *, kmem_cache_t *, unsigned long),
                                  void (*dtor) (void *, kmem_cache_t *, unsigned long))
{
  kmem_cache_t *kcache;

  LOGd_Enter(DEBUG_SLAB);

  if (!name)
    {
      LOG_Error ("kmem_cache name required");
      return NULL;
    }
  if(dtor){
      LOG_Error("No destructors supported!");
      return 0;
  }

  kcache = vmalloc (sizeof (kmem_cache_t));
  kcache->l4slab_cache = vmalloc (sizeof (l4slab_cache_t));
  if (l4slab_cache_init (kcache->l4slab_cache, size, 0, alloc_grow, NULL))
    {
      LOG_Error ("Couldn't get l4slab_cache");
      return NULL;
    }

  strcpy (kcache->name, name);

  kcache->ctor = ctor;
  kcache->dtor = dtor;

  return kcache;
}

/** Finalize slab cache
 * \ingroup mod_mm_slab */
int kmem_cache_destroy (kmem_cache_t * kcache)
{
  LOGd_Enter(DEBUG_SLAB);

  l4slab_destroy (kcache->l4slab_cache);
  vfree (kcache->l4slab_cache);
  vfree (kcache);

  return 0;  // l4slab_destroy never fails
}

/** Allocate slab in cache
 * \ingroup mod_mm_slab */
void * kmem_cache_alloc (kmem_cache_t * kcache, int flags)
{
  void *p = l4slab_alloc (kcache->l4slab_cache);

  if (kcache->ctor)
    kcache->ctor(p, kcache, flags);

  return p;
}

/** Free slab in cache
 * \ingroup mod_mm_slab */
void kmem_cache_free (kmem_cache_t * kcache, void *objp)
{
  l4slab_free (kcache->l4slab_cache, objp);
}
