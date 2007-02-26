/* $Id$ */
/*****************************************************************************/
/**
 * \file   slab/examples/bench/main.c
 * \brief  Memory allocation benchmark
 *
 * \date   02/05/2003
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4/L4Env includes */
#include <l4/slab/slab.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/util/rdtsc.h>
#include <l4/env/errno.h>
#include <l4/oskit10_l4env/support.h>
#include <l4/util/macros.h>

/* standard includes */
#include <stdio.h>
#include <stdlib.h>

#define OBJ_SIZE  64
#define MEM_SIZE  (256*1024)
#define ROUNDS    1000

/* Log tag */
char LOG_tag[9] = "slab_be";

typedef struct
{
  void *        ptr;
  unsigned long cycles_alloc;
  unsigned long cycles_free;
} alloc_t;

/*****************************************************************************/
/**
 * \brief Slab benchmark
 */
/*****************************************************************************/ 
static void
bench_slab(void)
{
  l4slab_cache_t slab;
  void * mem, * page, * ptr;
  int ret;
  l4_uint64_t start,end,time;
  int num;
  static alloc_t alloc[ROUNDS];
  
  /* allocate slab memory */
  mem = l4dm_mem_allocate(MEM_SIZE,L4RM_MAP | L4RM_LOG2_ALIGNED);
  if (mem == NULL)
    {
      printf("memory allocation failed!\n");
      return;
    }
  printf("l4slab: using memory at %p\n",mem);
  memset(mem,0,MEM_SIZE);

  /* setup slab cache */
  ret = l4slab_cache_init(&slab,OBJ_SIZE,0,NULL,NULL);
  if (ret < 0)
    {
      printf("setup slab cache failed: %s (%d)\n",l4env_errstr(ret),ret);
      return;
    }

  /* add memory */
  page = mem + MEM_SIZE;
  while (page > mem)
    {
      page -= L4_PAGESIZE;
      l4slab_add_page(&slab,page,NULL);
    }
  //l4slab_dump_cache(&slab,0);

  /* do benchmark, alloc */
  num = 0;
  while (num < ROUNDS)
    {
      start = l4_rdtsc();
      alloc[num].ptr = l4slab_alloc(&slab);
      end = l4_rdtsc();

      alloc[num].cycles_alloc = (int)(end - start);
      num++;
    }

  time = 0;
  num = 0;
  while (num < ROUNDS)
    {
      // printf("%7d %7lu\n",num,alloc[num].cycles_alloc);
      time += alloc[num].cycles_alloc;
      num++;
    }
  printf("l4slab: alloc %u cycles average\n",(unsigned)(time / num));

  /* do benchamrk, free */
  num = 0;
  while (num < ROUNDS)
    {
      start = l4_rdtsc();
      l4slab_free(&slab,alloc[num].ptr);
      end = l4_rdtsc();

      alloc[num].cycles_free = (int)(end - start);
      num++;
    }

  time = 0;
  num = 0;
  while (num < ROUNDS)
    {
      //printf("%7d %7d\n",num,cycles[num]);
      time += alloc[num].cycles_free;
      num++;
    }
  printf("l4slab: free %u cycles average\n",(unsigned)(time / num));

  num = 0;
  do
    {
      ptr = l4slab_alloc(&slab);
      if (ptr != NULL)
	num++;
    }
  while (ptr != NULL);

  printf("l4slab: %d objects\n",num);

  /* destroy slab */
  l4slab_destroy(&slab);

  /* free slab memory */
  l4dm_mem_release(mem);

  printf("\n");
}

/*****************************************************************************/
/**
 * \brief OSKit malloc benchamrk
 */
/*****************************************************************************/ 
static void 
bench_malloc(void)
{
  int ret;
  l4_uint64_t start,end,time;
  int num;
  static alloc_t alloc[ROUNDS];
  void * ptr;

  /* init malloc */
  ret = OSKit_libc_support_init(MEM_SIZE);
  if (ret < 0)
    {
      printf("init OSKit heap failed: %s (%d)\n",l4env_errstr(ret),ret);
      return;;
    }

  /* do benchmark, alloc */
  num = 0;
  while (num < ROUNDS)
    {
      start = l4_rdtsc();
      alloc[num].ptr = malloc(OBJ_SIZE);
      end = l4_rdtsc();

      alloc[num].cycles_alloc = (int)(end - start);
      num++;
    }
	 
  time = 0;
  num = 0;
  while (num < ROUNDS)
    {
      //printf("%7d %7d\n",num,cycles[num]);
      time += alloc[num].cycles_alloc;
      num++;
    }
  printf("OSKit malloc: alloc %u cycles average\n",(unsigned)(time / num));

  /* do benchmark, free */
  num = 0;
  while (num < ROUNDS)
    {
      start = l4_rdtsc();
      free(alloc[num].ptr);
      end = l4_rdtsc();

      alloc[num].cycles_free = (int)(end - start);
      num++;
    }

  time = 0;
  num = 0;
  while (num < ROUNDS)
    {
      //printf("%7d %7d\n",num,cycles[num]);
      time += alloc[num].cycles_free;
      num++;
    }
  printf("OSKit malloc: free %u cycles average\n",(unsigned)(time / num));  

  num = 0;
  do
    {
      ptr = malloc(OBJ_SIZE);
      if (ptr != NULL)
	num++;
    }
  while (ptr != NULL);

  printf("OSKit malloc: %d objects\n",num);
}

/*****************************************************************************/
/**
 * \brief Main
 */
/*****************************************************************************/ 
int
main(void)
{
  /* do slab benchmark */
  bench_slab();

  /* do OSKit malloc benchmark */
  bench_malloc();

  /* done */
  KDEBUG("done.");

  exit(0);
}
