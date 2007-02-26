/* $Id$ */
/*****************************************************************************/
/**
 * \file   slab/examples/test/main.c
 * \brief  Slab allocator tests
 *
 * \date   02/05/2003
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4/L4Env includes */
#include <l4/slab/slab.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/l4rm/l4rm.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* Log tag */
char LOG_tag[9] = "slab_te";

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

  // allocate and map page
  page = l4dm_mem_allocate(L4_PAGESIZE,L4RM_MAP | L4RM_LOG2_ALIGNED);
  if (page == NULL)
    Error("__grow_simple: page allocation failed!\n");

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
  l4dm_mem_release(page);
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

  // initialize slab
  ret = l4slab_cache_init(&slab,OBJ_SIZE,0,__grow_simple,__release_simple);
  if (ret < 0)
    {
      Error("simple_heap: slab cache initializstion failed: %s (%d)\n",
	    l4env_errstr(ret),ret);
      return;
    }

  // allocate objects
  objp = l4slab_alloc(&slab);

  printf("simple_heap: got obj at %p\n",objp);

  // release object
  l4slab_free(&slab,objp);
  
  // destroy slab
  l4slab_destroy(&slab);
}

/*****************************************************************************
 *** grow-only heap
 *****************************************************************************/

#define MAX_HEAP_SIZE  (64 * 1024)
#define INIT_HEAP_SIZE (16 * 1024)
 
l4dm_dataspace_t heap_ds;
l4_addr_t        heap_addr;
l4_size_t        heap_cur_size;

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
  l4_addr_t addr;
  l4_size_t new_size; 
  int ret;

  if ((heap_cur_size + L4_PAGESIZE) > MAX_HEAP_SIZE)
    {
      printf("__grow: heap overflow!\n");
      return NULL;
    }

  // resize dataspace
  addr = heap_addr + heap_cur_size;
  new_size = heap_cur_size + L4_PAGESIZE;

  ret = l4dm_mem_resize(&heap_ds,new_size);
  if (ret < 0)
    {
      printf("__grow: resize heap dataspace failed: %s (%d)\n",
	     l4env_errstr(ret),ret);
      return NULL;
    }
  heap_cur_size = new_size;

  printf("__grow: resized dataspace to %u, added page at %p\n",
	 new_size,(void *)addr);

  // done
  return (void *)addr;
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
  void * heap, * page;
  l4slab_cache_t slab;
  void * objp;

  // allocate heap 
  ret = l4dm_mem_open(L4DM_DEFAULT_DSM, INIT_HEAP_SIZE, L4_PAGESIZE, 0,
		      "Heap", &heap_ds);
  if (ret < 0)
    {
      Error("grow_only_heap: heap allocation failed: %s (%d)\n",
	    l4env_errstr(ret),ret);
      return;
    }

  // attach heap dataspace, already reserve the whole vm area 
  ret = l4rm_attach(&heap_ds, MAX_HEAP_SIZE, 0, L4DM_RW, &heap);
  if (ret < 0)
    {
      Error("grow_only_heap: attach heap dataspace failed: %s (%d)\n",
	    l4env_errstr(ret),ret);
      l4dm_close(&heap_ds);
      return;
    }
  heap_addr = (l4_addr_t)heap;
  heap_cur_size = INIT_HEAP_SIZE;

  printf("grow_only_heap: heap at %p, init size %u, max size %u\n",
	 heap,INIT_HEAP_SIZE,MAX_HEAP_SIZE);

  // setup slab cache, no release callback
  ret = l4slab_cache_init(&slab,OBJ_SIZE,0,__grow,NULL);
  if (ret < 0)
    {
      Error("grow_only_heap: slab cache initialization failed: %s (%d)\n",
	    l4env_errstr(ret),ret);
      return;
    }

  // add initial pages to slab 
  page = heap;
  while (page < (heap + INIT_HEAP_SIZE))
    {
      l4slab_add_page(&slab,page,NULL);

      printf("grow_only_heap: added initial page at %p\n",page);
      
      page += L4_PAGESIZE;
    }

  do
    objp = l4slab_alloc(&slab);
  while (objp != NULL);

  // destroy slab
  l4slab_destroy(&slab);

  // release heap memory 
  l4rm_detach((void *)heap_addr);
  l4dm_close(&heap_ds);
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

  printf("\n");

  KDEBUG("done");

  return 0;
}
