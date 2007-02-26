/* $Id$ */
/*****************************************************************************/
/**
 * \file   slab/examples/test/main.c
 * \brief  Slab allocator tests
 *
 * \date   2006-12-18
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4/L4Env includes */
#include <stdio.h>
#include <string.h>
#include <l4/slab/slab.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/util/list_alloc.h>

/* Log tag */
char LOG_tag[9] = "slab_te";

/* heap */
static char backing_store[512*1024] __attribute__ (( aligned(L4_PAGESIZE) ));
static l4la_free_t *heap;

/*****************************************************************************
 *** simple heap
 *****************************************************************************/

#define OBJ_SIZE 64

/*****************************************************************************/
/**
 * \brief  Slab cache grow callback
 *
 * \param  cache         Slab cache descriptor
 * \retval data          User data pointer
 *
 * \return Pointer to new page
 */
/*****************************************************************************/
static void *
__grow_simple(l4slab_cache_t * cache, void ** data)
{
  void * page;

  // allocate page
  page = l4la_alloc(&heap, L4_PAGESIZE, L4_LOG2_PAGESIZE);
  if (page == NULL)
    LOG_Error("__grow_simple: page allocation failed!\n");

  printf("__grow_simple: got page at %p\n",page);

  return page;
}

/*****************************************************************************/
/**
 * \brief  Slab cache release callback
 *
 * \param cache          Slab cache descriptor
 * \param page           Page address
 * \param data           User data pointer
 */
/*****************************************************************************/
static void
__release_simple(l4slab_cache_t * cache, void * page, void * data)
{
  printf("__release_simple: release page at %p\n",page);

  // free page
  l4la_free(&heap, page, L4_PAGESIZE);
}

/*****************************************************************************/
/**
 * \brief Main
 */
/*****************************************************************************/
static void
simple_heap(void)
{
  l4slab_cache_t slab;
  int ret;
  void * objp;

  // initialize heap
  printf("simple_heap: heap @ %p\n", backing_store);
  memset(backing_store, 0, sizeof(backing_store));
  l4la_init(&heap);
  l4la_free(&heap, backing_store, sizeof(backing_store));

  // initialize slab
  ret = l4slab_cache_init(&slab,OBJ_SIZE,0,__grow_simple,__release_simple);
  if (ret < 0)
    {
      LOG_Error("simple_heap: slab cache initializstion failed: %s (%d)",
                l4env_errstr(ret),ret);
      return;
    }

  printf("simple_heap: after init\n"); l4slab_dump_cache(&slab, 0);

  // allocate objects
  objp = l4slab_alloc(&slab);

  printf("simple_heap: after alloc\n"); l4slab_dump_cache(&slab, 0);

  printf("simple_heap: got obj at %p\n",objp);

  // release object
  l4slab_free(&slab,objp);

  printf("simple_heap: after free\n"); l4slab_dump_cache(&slab, 0);

  // destroy slab
  l4slab_destroy(&slab);

  printf("simple_heap: after destroy\n"); l4slab_dump_cache(&slab, 0);
}

/*****************************************************************************
 *** grow-only heap
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  grow callback
 *
 * \param  cache         Slab cache descriptor
 * \retval data          User data pointer
 *
 * \return Pointer to new page
 */
/*****************************************************************************/
static void *
__grow(l4slab_cache_t * cache, void ** data)
{
  void *p;

  p = l4la_alloc(&heap, L4_PAGESIZE, L4_LOG2_PAGESIZE);

  if (p == NULL)
    printf("__grow: heap overflow!\n");
  else
    printf("__grow: got page at %p\n", p);

  // done
  return p;
}

/*****************************************************************************/
/**
 * \brief Grow-only heap
 */
/*****************************************************************************/
static void
grow_only_heap(void)
{
  int ret;
  void * page;
  l4slab_cache_t slab;
  void * objp;

  // initialize heap
  printf("simple_heap: heap @ %p\n", backing_store);
  memset(backing_store, 0, sizeof(backing_store));
  l4la_init(&heap);
  l4la_free(&heap, backing_store, sizeof(backing_store));

  printf("grow_only_heap: heap at %p, init size %u, max size %u\n",
         heap, sizeof(backing_store) / 2, sizeof(backing_store));

  // setup slab cache, no release callback
  ret = l4slab_cache_init(&slab,OBJ_SIZE,0,__grow,NULL);
  if (ret < 0)
    {
      LOG_Error("grow_only_heap: slab cache initialization failed: %s (%d)",
                l4env_errstr(ret),ret);
      return;
    }

  // check slab_size
  if (slab.slab_size != L4_PAGESIZE)
    {
      printf("Assumed %d-sized slabs - current size is %d. Aborting...\n",
             L4_PAGESIZE, slab.slab_size);
      return;
    }

  // add initial pages to slab
  while (l4la_avail(&heap) > sizeof(backing_store) / 2)
    {
      page = l4la_alloc(&heap, L4_PAGESIZE, L4_LOG2_PAGESIZE);
      l4slab_add_slab(&slab,page,NULL);

      printf("grow_only_heap: added initial page at %p\n",page);
    }

  do
    objp = l4slab_alloc(&slab);
  while (objp != NULL);

  // destroy slab
  l4slab_destroy(&slab);
}

/*****************************************************************************/
/**
 * \brief Main
 */
/*****************************************************************************/
int
main(void)
{
  // simple heap
  simple_heap();

  printf("\n");

  // grow-only heap
  grow_only_heap();

  printf("\nDone.\n");

  return 0;
}
