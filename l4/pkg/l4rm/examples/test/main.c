/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/examples/test/main.c 
 * \brief  Region mapper test.
 *
 * \date   05/27/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <stdio.h>

/* L4 include */
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/util/rand.h>
#include <l4/util/rdtsc.h>
#include <l4/l4rm/l4rm.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/slab/slab.h>
#include <l4/env/env.h>
#include <l4/util/macros.h>

/* private lib includes */
#include "__avl_tree.h"

char LOG_tag[9]="l4rmtest";

#define NUM_NODES 8192
#define ROUNDS    100000

typedef struct node
{ 
  int start;
  int end;
  int data;
  int in_tree;
} node_t;

node_t nodes[NUM_NODES];

static double ns_per_cycle;

/*****************************************************************************
 * AVL tree test
 *****************************************************************************/

extern l4slab_cache_t l4rm_avl_node_cache;

static void 
avl_test(void)
{
#ifdef DEBUG
  int i,j,r,ret;
  int num_in_tree = 0;

  for (i = 0; i < NUM_NODES; i ++)
    {
      nodes[i].start = 2 * i;
      nodes[i].end = 2 * i + 1;
      nodes[i].data = i;
      nodes[i].in_tree = 0;
    }

  l4util_srand(l4_rdtsc_32());

  for (r = 0; r < ROUNDS; r++)
    {
      if ((r % 200) == 0)
	l4slab_dump_cache_free(&l4rm_avl_node_cache);

      /* insert */
      i = (int)((double)(NUM_NODES - num_in_tree - 1) * 
		((double)l4util_rand() / (double)L4_RAND_MAX));
      printf("%6d: insert %d\n",r,i);
      while (i > 0)
	{
	  j = (int)((double)(NUM_NODES - 1) * 
		    ((double)l4util_rand() / (double)L4_RAND_MAX));
	  if (!nodes[j].in_tree)
	    {
	      ret = AVLT_insert(nodes[j].start,nodes[j].end,nodes[j].data);
	      if (ret < 0)
		{
		  printf("insert failed, ret = %d\n",ret);
		  printf(" %d: %d-%d\n",j,nodes[j].start,nodes[j].end);
		  printf("\n");
		  enter_kdebug("???");
		}
	      nodes[j].in_tree = 1;
	      num_in_tree++;
	      i--;
	    }
	}

      /* remove */
      i = (int)((double)(num_in_tree - 1) * 
		((double)l4util_rand() / (double)L4_RAND_MAX));
      printf("%6d: remove %d of %d\n",r,i,num_in_tree);
      while (i > 0)
	{
	  j = (int)((double)(NUM_NODES - 1) * 
		    ((double)l4util_rand() / (double)L4_RAND_MAX));
	  if (nodes[j].in_tree)
	    {
	      ret = AVLT_remove(nodes[j].start,nodes[j].end);
	      if (ret < 0)
		{
		  printf("remove failed, ret = %d\n",ret);
		  printf(" %d: %d-%d\n",j,nodes[j].start,nodes[j].end);
		  printf("\n");
		  enter_kdebug("???");
		}
	      nodes[j].in_tree = 0;
	      num_in_tree--;
	      i--;
	    }
	}

      /* search nodes */
      printf("%6d: find ",r);
      for (j = 0; j < NUM_NODES; j++)
	{
	  if (nodes[j].in_tree)
	    {
	      ret = AVLT_find(nodes[j].start,nodes[j].end);
	      if (ret)
		{
		  printf("find failed, ret = %d\n",ret);
		  printf(" %d: %d-%d\n",j,nodes[j].start,nodes[j].end);
		  printf("\n");
		  enter_kdebug("???");
		}
#if 0
	      printf(".");
#endif
	    }
	}
      printf("\n");
    } 
#else
  LOG_Error("Debug output must be enabled to run this test...");
#endif
}

/*****************************************************************************
 * Region mapper test
 *****************************************************************************/
static int
__pf_callback(l4_addr_t addr, l4_addr_t eip, l4_threadid_t src)
{
  printf("pf at 0x%08lx, eip 0x%08lx, src "l4util_idfmt"\n", 
         addr, eip, l4util_idstr(src));
  enter_kdebug("-");
  return L4RM_REPLY_EXCEPTION;
}

static void
rm_test(void)
{
  int ret;
  l4dm_dataspace_t ds1,ds2,ds3;
  volatile char *c;
  volatile l4_uint32_t addr;
  l4_addr_t addr1,addr2;
  void *addr3, *addr4, *addr5, *addr6, *addr7;
  l4_threadid_t dm_id, pager;
  l4_uint32_t area;
  l4_addr_t area_addr,a;
  l4_offs_t offset;
  l4_cpu_time_t t_start,t_end;
  l4_uint32_t cycles;
  l4_size_t map_size;

  l4_size_t size = (4 * 1024);
  //  l4_addr_t map_addr = 0x80001000;
  l4_addr_t map_addr = 0x7ffff000;

  /* get dm id */
  dm_id = l4env_get_default_dsm();
  if (l4_is_invalid_id(dm_id))
    {
      printf("dm not found\n");
      enter_kdebug("-");
    }

  printf("dm: "l4util_idfmt"\n",l4util_idstr(dm_id));

  /* open ds */
  ret = l4dm_mem_open(dm_id,size,0,0,"l4rm_test1",&ds1);
  if (ret < 0)
    {
      printf("dataspace allocation failed (ret %d)\n",ret);
      enter_kdebug("-");
    }

  /***************************************************************************
   *** pagefault handling
   ***************************************************************************/

  l4rm_set_unkown_pagefault_callback(__pf_callback);

  *(int *)0 = 1;

  /***************************************************************************
   *** l4rm_setup*
   ***************************************************************************/

  ret = l4rm_area_setup(size, L4RM_DEFAULT_REGION_AREA, L4RM_REGION_BLOCKED,
                        0, L4_INVALID_ID, &addr1);
  if (ret < 0)
    printf("l4rm_area_setup failed: %s\n", l4env_errstr(ret));
  else
    printf("got area at 0x%08lx\n", addr1);
  l4rm_show_region_list();
  enter_kdebug("-");

  ret = l4rm_area_clear_region(addr1);
  if (ret < 0)
    printf("l4rm_area_clear_region failed: %s\n", l4env_errstr(ret));

  l4rm_show_region_list();
  enter_kdebug("-");
  
  /***************************************************************************
   *** l4rm_area_release_addr
   ***************************************************************************/

  ret = l4rm_area_reserve(0x10000,0,&addr1,&area);
  if (ret < 0)
    printf("l4rm_area_reserve failed: %s (%d)\n",l4env_errstr(ret),ret);
  else
    printf("area %x at 0x%08lx\n",area,addr1);

  l4rm_show_region_list();
  enter_kdebug("-");

  ret = l4rm_area_release_addr((void *)addr1);
  if (ret < 0)
    printf("l4rm_area_release_addr 0x%08lx failed: %s (%d)\n",addr1,
	   l4env_errstr(ret),ret);
  else
    printf("released area at 0x%08lx\n",addr1);

  l4rm_show_region_list();
  enter_kdebug("-");

  /***************************************************************************
   *** l4rm_lookup
   ***************************************************************************/

  /* attach ds */
  ret = l4rm_attach_to_region(&ds1,(void *)map_addr,size,0,L4DM_RW);
  if (ret)
    {
      printf("attach dataspace failed (%d)\n",ret);
      enter_kdebug("-");
    }

  /* map pages */
  for (addr1 = map_addr; addr1 < map_addr + size; addr1 += L4_PAGESIZE)
    *((int *)addr1) = 1;

  printf("ds %d at "l4util_idfmt"\n", ds1.id,l4util_idstr(ds1.manager));
  l4rm_show_region_list();
  avlt_show_tree();
  enter_kdebug("attached");

  /* lookup address */
  t_start = l4_rdtsc();
  ret = l4rm_lookup((void *)map_addr + 0x123, &a, &map_size, 
                    &ds2, &offset, &pager);
  t_end = l4_rdtsc();
  if (ret != L4RM_REGION_DATASPACE)
    {
      printf("address lookup failed (%d)\n",ret);
      enter_kdebug("-");
    }
  cycles = (l4_uint32_t)(t_end - t_start);
  printf("lookup: ds %d at "l4util_idfmt
         ", offset %ld, maped to 0x%08lx, size %u\n",
	 ds2.id,l4util_idstr(ds2.manager),offset,a,map_size);
  printf("%u cyles\n",cycles);
  enter_kdebug("lookup");

  /* detach dataspace */
  ret = l4rm_detach((void *)(map_addr + 0x123));
  if (ret)
    {
      printf("detach dataspace failed (%d)\n",ret);
      enter_kdebug("-");
    }

  l4rm_show_region_list();
  enter_kdebug("detached");

  /***************************************************************************
   * old tests
   ***************************************************************************/

  /* open new ds 1 */
  ret = l4dm_mem_open(dm_id,10000,0,0,"l4rm_test1",&ds1);
  if (ret)
    {
      printf("error allocating dataspace: %d\n",ret);
      enter_kdebug("???");
    }
  printf("ds1 = %d at "l4util_idfmt"\n",ds1.id,l4util_idstr(ds1.manager));

  ret = l4rm_attach(&ds1,10000,0, L4RM_LOG2_ALIGNED | L4RM_LOG2_ALLOC, &addr7);
  if (ret)
    {
      printf("error attaching dataspace: %d\n",ret);
      enter_kdebug("???");
    }
  printf("attached ds1 to region at addr 0x%08x\n",(unsigned)addr7);

  ret = l4rm_area_reserve(0x01000001,L4RM_LOG2_ALIGNED | L4RM_LOG2_ALLOC,&addr2,&area);
  if (ret)
    {
      printf("reserve area failed: %d\n",ret);
      enter_kdebug("???");
    }
  printf("reserved area %x at 0x%08lx\n",area,addr2);

  ret = l4rm_area_attach(&ds1,area,4096,0,0,&addr3);
  if (ret)
    {
      printf("error attaching dataspace: %d\n",ret);
      enter_kdebug("???");
    }
  printf("attached ds1 to region at addr 0x%08x\n",(unsigned)addr3);
  
  ret = l4rm_area_attach(&ds1,area,10000,0,L4RM_LOG2_ALIGNED | L4RM_LOG2_ALLOC,
      &addr4);
  if (ret)
    {
      printf("error attaching dataspace: %d\n",ret);
      enter_kdebug("???");
    }
  printf("attached ds1 to region at addr 0x%08x\n",(unsigned)addr4);

  l4rm_show_region_list();
  enter_kdebug("-");

  /* open new ds 2 */
  ret = l4dm_mem_open(dm_id,1000,0,0,"l4rm_test2",&ds2);
  if (ret)
    {
      printf("error allocating dataspace: %d\n",ret);
      enter_kdebug("???");
    }
  printf("ds2 = %d at "l4util_idfmt"\n",ds2.id,l4util_idstr(ds2.manager));

  addr5 = (void*)0x10010000;
  ret = l4rm_attach_to_region(&ds2,addr5,1000,0,0);
  if (ret)
    {
      printf("error attaching dataspace: %d\n",ret);
      enter_kdebug("???");
    }
  printf("attached ds2 to region at address 0x%08x\n",(unsigned)addr5);

  l4rm_show_region_list();
  enter_kdebug("-");

  /* reserve vm area */
  ret = l4rm_area_reserve(0x10000000,0,&area_addr,&area);
  if (ret)
    {
      printf("reserve area failed: %d\n",ret);
      enter_kdebug("???");
    }
  printf("reserved area at 0x%08lx, id 0x%05x\n",area_addr,area);

  l4rm_show_region_list();
  enter_kdebug("-");

  /* open new ds 3 */
  ret = l4dm_mem_open(dm_id,1000,0,0,"l4rm_test3",&ds3);
  if (ret)
    {
      printf("error allocating dataspace: %d\n",ret);
      enter_kdebug("???");
    }
  printf("ds3 = %d at "l4util_idfmt"\n",ds3.id,l4util_idstr(ds3.manager));

  ret = l4rm_area_attach(&ds3,area,0x100000,0,0,&addr6);
  if (ret)
    {
      printf("error attaching dataspace: %d\n",ret);
      enter_kdebug("???");
    }
  printf("attached ds3 to region at address 0x%08x\n",(unsigned)addr6);

  l4rm_show_region_list();
  enter_kdebug("-");

  ret = l4rm_area_release(area);
  if (ret)
    {
      printf("release area failed: %d\n",ret);
      enter_kdebug("???");
    }

  l4rm_show_region_list();
  enter_kdebug("-");

  avlt_show_tree();

  ret = l4rm_detach(addr4);
  if (ret)
    {
      printf("error detaching dataspace: %d\n",ret);
      enter_kdebug("???");
    }

  ret = l4rm_detach(addr5);
  if (ret)
    {
      printf("error detaching dataspace: %d\n",ret);
      enter_kdebug("???");
    }

  l4rm_show_region_list();
  avlt_show_tree();

  printf("=========================\n"
         "Next error is EXPECTED!!!\n"
	 "=========================\n");

  addr = 0x10010000;
  c = (volatile char *)addr;
  *c = 'a';

  printf("done %c\n",*c);

}

/*****************************************************************************
 * Slab tests
 *****************************************************************************/
unsigned char buf[L4_PAGESIZE] __attribute__ ((aligned (L4_PAGESIZE)));
unsigned char buf1[L4_PAGESIZE] __attribute__ ((aligned (L4_PAGESIZE)));

unsigned char * buffers[2];
int idx = 0;

static void *
grow(l4slab_cache_t * cache, void ** data)
{
  void * ptr;

  printf("grow: called -> buf[%d]\n",idx);

  ptr = buffers[idx];
  idx++;

  return ptr;
}

static void 
release(l4slab_cache_t * cache, void * ptr, void * data)
{
  printf("release: called, ptr = 0x%08x\n",(unsigned)ptr);
}

static void
slab_test(void)
{
  void * ptr, * ptr1, * ptr2;
  l4slab_cache_t cache;

  buffers[0] = buf;
  buffers[1] = buf1;

  l4slab_cache_init(&cache,2000,0,grow,release);

  l4slab_dump_cache(&cache,1);

  ptr = l4slab_alloc(&cache);
  printf("got obj at 0x%08x\n",(unsigned)ptr);
  ptr1 = l4slab_alloc(&cache);
  printf("got obj at 0x%08x\n",(unsigned)ptr1);
  ptr2 = l4slab_alloc(&cache);
  printf("got obj at 0x%08x\n",(unsigned)ptr2);

  l4slab_dump_cache(&cache,1);

  l4slab_free(&cache,ptr);
  l4slab_free(&cache,ptr1);

  l4slab_dump_cache(&cache,1);
  
}

/*****************************************************************************
 * Main
 *****************************************************************************/
int 
main(void)
{
  double mhz;

  /* init time measuring */
  l4_calibrate_tsc();
  ns_per_cycle = l4_tsc_to_ns(10000000ULL) / 10000000.0;
  mhz = 1000.0 / ns_per_cycle;
  printf("running on a %u.%02u MHz machine (%u.%03u ns/cycle)\n",
	 (unsigned)mhz,(unsigned)((mhz - (unsigned)mhz) * 100),
	 (unsigned)ns_per_cycle,
	 (unsigned)((ns_per_cycle - (unsigned)ns_per_cycle) * 1000));


  enter_kdebug("start...");

  if (0) 
    avl_test();

  if (1)
    rm_test();

  if (0)
    slab_test();

  printf("main: done\n");

  return 0;
}
