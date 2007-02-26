/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/examples/test/main.c
 * \brief  DMphys test application
 *
 * \date   11/23/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2000-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/

/* standard includes */
#include <string.h>

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/log/l4log.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/l4rm/l4rm.h>
#include <l4/oskit10_l4env/support.h>

/* DMphys includes */
#include <l4/dm_phys/dm_phys.h>

/**
 * DMphys service id
 */
l4_threadid_t dsm_id;

/*****************************************************************************
 *** Create some small free regions at DMphys
 *****************************************************************************/
static void 
__scatter_dm_phys(void)
{

#define SCATTER_NUM_DS    32
#define SCATTER_DS_SIZE   (4 * 1024)

  static l4dm_dataspace_t ds[SCATTER_NUM_DS];
  int i,ret;

  /* scatter DMphys region list */
  for (i = 0; i < SCATTER_NUM_DS; i++)
    {
      ret = l4dm_mem_open(dsm_id,SCATTER_DS_SIZE,0,0,"dummy",&ds[i]);
      if (ret < 0)
	Panic("open dummy dataspace failed (%d)!",ret);      
    }

  for (i = 0; i < SCATTER_NUM_DS; i += 2)
    {
      ret = l4dm_close(&ds[i]);
      if (ret < 0)
	Panic("close dummy dataspace failed (%d)!",ret);
    }
}

/*****************************************************************************
 *** Simple stress test 
 *****************************************************************************/
void 
stress_test(void);

void 
stress_test(void)
{
#define NUM_DS    4096
#define DS_SIZE   (32 * 1024)

  static l4dm_dataspace_t ds[NUM_DS];
  int i,ret;

  l4dm_memphys_show_pool_areas(0);
  KDEBUG("start...");

  for (i = 0; i < NUM_DS; i++)
    {
      ret = l4dm_mem_open(dsm_id,DS_SIZE,0,0,"dummy",&ds[i]);
      if (ret < 0)
	Panic("open dummy dataspace failed (%d)!",ret);      
    }

  //  l4dm_memphys_show_pool_areas(0);

  l4dm_memphys_show_slabs(0);
  KDEBUG("opened.");

  for (i = NUM_DS - 1; i >= 0; i--)
    //for (i = 0; i < NUM_DS; i++)
    {
      ret = l4dm_close(&ds[i]);
      if (ret < 0)
	Panic("close dummy dataspace failed (%d)!",ret);
    }

  KDEBUG("closed.");

  l4dm_memphys_show_slabs(0);

  KDEBUG("slabs.");

  l4dm_memphys_show_pool_areas(0);
  l4dm_ds_list_all(dsm_id);

  KDEBUG("continue...");
}

/*****************************************************************************
 *** Test l4dm_memphys_check_pagesize
 ***
 *** DMphys cfg: --pool=2,0x00800000,0x02000000,0x02800000
 *****************************************************************************/
void 
test_check_pagesize(void);

void 
test_check_pagesize(void)
{
  l4dm_dataspace_t ds;
  int ret,ok;
  l4_addr_t addr;
  l4_offs_t offs;
  l4_size_t size;
  void * ptr;
  
  KDEBUG("start...");

  ptr = l4dm_mem_ds_allocate(0x00400000,
			     L4DM_MEMPHYS_4MPAGES | L4RM_LOG2_ALIGNED | 
			     L4RM_LOG2_ALLOC | L4RM_MAP | L4DM_MAP_MORE,
			     &ds);
  if (ptr == NULL)
    Panic("allocate failed!");
  else
    {
      INFO("\n");
      DMSG("  dataspace %u at %x.%x, mapped at 0x%08x\n",ds.id,
	   ds.manager.id.task,ds.manager.id.lthread,(l4_addr_t)ptr);
    }

  l4dm_ds_show(&ds);
  
  ret = l4dm_memphys_check_pagesize(ptr,0x00400000,
				    L4_LOG2_SUPERPAGESIZE);
  INFO("check_pagesize = %d\n",ret);

  l4dm_close(&ds);

  KDEBUG("next...");

  ret = l4dm_memphys_open(2,                        // pool
			  0x02000000,               // address
			  0x00400000,               // size
			  //L4_SUPERPAGESIZE,         // alignment
			  0,
                          //L4DM_MEMPHYS_4MPAGES,     // flags
			  L4DM_CONTIGUOUS,
                          "test",&ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)",ret);
  else
    INFO("dataspace %u at %x.%x\n",ds.id,
	 ds.manager.id.task,ds.manager.id.lthread);

  l4dm_ds_show(&ds);

  addr = 0x10000000;
  offs = 0x00000000;
  size = 0x00400000;
  ret = l4rm_attach_to_region(&ds,(void *)addr,size,offs,L4DM_RW);
  if (ret < 0)
    Panic("attach dataspace failed (%d)",ret);
  else
    INFO("attached to addr 0x%08x\n",addr);

  ret = l4dm_memphys_open(2,                        // pool
			  0x02400000,               // address
			  0x00400000,               // size
			  //L4_SUPERPAGESIZE,         // alignment
			  0,
                          //L4DM_MEMPHYS_4MPAGES,     // flags
			  L4DM_CONTIGUOUS,
                          "test",&ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)",ret);
  else
    INFO("dataspace %u at %x.%x\n",ds.id,
	 ds.manager.id.task,ds.manager.id.lthread);

  l4dm_ds_show(&ds);

  addr = 0x10400000;
  offs = 0x00000000;
  size = 0x00400000;
  ret = l4rm_attach_to_region(&ds,(void *)addr,size,offs,L4DM_RW);
  if (ret < 0)
    Panic("attach dataspace failed (%d)",ret);
  else
    INFO("attached to addr 0x%08x\n",addr);

  KDEBUG("attached.");

  addr = 0x10000000;
  size = 0x00800000;
  ret = l4dm_memphys_check_pagesize((void *)addr,size,
				    L4_LOG2_SUPERPAGESIZE);
  INFO("check_pagesize = %d\n",ret);

  KDEBUG("next...");

  ret = l4dm_memphys_pagesize(&ds,0,L4_SUPERPAGESIZE,L4_LOG2_SUPERPAGESIZE,&ok);
  if (ret < 0)  
    Panic("check pagesize failed (%d)",ret);
  else
    INFO("ok = %d\n",ok);

  KDEBUG("done");
}

/*****************************************************************************
 *** l4dm_memphys_copy
 ***
 *** DMphys cfg: --isa=0x00200000
 *****************************************************************************/
void
test_phys_copy(void);

void
test_phys_copy(void)
{
  l4dm_dataspace_t ds,copy;
  int ret;
  l4_addr_t addr;

  l4_size_t size = (24 * 1024);

  KDEBUG("start...");

  ret = l4dm_mem_open(dsm_id,size,0,0,"test",&ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)",ret);
  else
    INFO("dataspace %u at %x.%x\n",ds.id,
	 ds.manager.id.task,ds.manager.id.lthread);

  ret = l4rm_attach(&ds,size,0,L4DM_RW,(void **)&addr);
  if (ret < 0)
    Panic("attach failed (%d)",ret);
  else
    INFO("attached to address 0x%08x\n",addr);
  memset((void *)addr,0x88,size);

  KDEBUG("opened.");

  ret = 
    l4dm_memphys_copy(&ds,                            // source dataspace
		      0x800,                          // source offset
		      1,                              // destination offset
		      0x3800,                         // bytes to copy
		      L4DM_MEMPHYS_ISA_DMA,           // dest. memory pool
		      0x00180000,                     // dest. addr
		      0x8000,                         // dest. size
		      0x00008000,                     // dest. alignment 
		      L4DM_CONTIGUOUS,
		      //0,
		      // L4DM_MEMPHYS_SAME_POOL,         // flags
		      "test copy",                    // name
		      &copy);

  if (ret < 0)
    Panic("copy failed (%d)",ret);
  else
    INFO("copy: %u at %x.%x\n",copy.id,
	 copy.manager.id.task,copy.manager.id.lthread);

  KDEBUG("copied.");

  l4dm_memphys_show_pool_areas(L4DM_MEMPHYS_ISA_DMA);

  KDEBUG("done.");
}

/*****************************************************************************
 *** l4dm_ds_show / pools / pool_areas / pool_free
 ***
 *** DMphys cfg: --isa=0x00200000
 *****************************************************************************/
void
test_debug_DMphys(void);

void
test_debug_DMphys(void)
{
  int ret;
  l4dm_dataspace_t ds;
  l4_addr_t esp,ds_map_addr;
  l4_offs_t offs;
  l4_size_t ds_map_size;

  KDEBUG("start...");

  asm("movl   %%esp, %0\n\t" : "=r" (esp) : );
  INFO("stack at 0x%08x\n",esp);

  ret = l4rm_lookup((void *)esp,&ds,&offs,&ds_map_addr,&ds_map_size);
  if(ret < 0)
    Panic("l4rm_lookup failed (%d)!",ret);

  Msg("  ds %u at %x.%x, offset 0x%08x, map area 0x%08x-0x%08x\n",
      ds.id,ds.manager.id.task,ds.manager.id.lthread,offs,
      ds_map_addr,ds_map_addr + ds_map_size);

  l4dm_ds_show(&ds);

  KDEBUG("next...");

  l4dm_memphys_show_pools();

  KDEBUG("pools");

  l4dm_memphys_show_pool_areas(L4DM_MEMPHYS_ISA_DMA);

  KDEBUG("areas");

  l4dm_memphys_show_pool_free(L4DM_MEMPHYS_ISA_DMA);

  KDEBUG("free lists");
}

/*****************************************************************************
 *** l4dm_mem_allocate / l4dm_mem_release
 *****************************************************************************/
void 
test_allocate(void);

void
test_allocate(void)
{
  void * ptr;
  l4_size_t size = (32 * 1024);

  KDEBUG("start...");

  ptr = l4dm_mem_allocate(size,L4RM_MAP);
  if (ptr == NULL)
    Panic("memory allocation failed!");
  else
    INFO("got mem at 0x%08x\n",(l4_addr_t)ptr);

  KDEBUG("allocated.");

  l4dm_mem_release(ptr);

  KDEBUG("released.");
}

/*****************************************************************************
 *** l4dm_transfer / l4dm_close_all
 *****************************************************************************/
void
test_transfer(void);

void
test_transfer(void)
{
  l4_threadid_t owner;
  l4dm_dataspace_t ds;
  int ret;

  l4_size_t size = (32 * 1024);

  KDEBUG("start...");

  owner = l4_myself();
  owner.id.lthread = 5;

  ret = l4dm_mem_open(dsm_id,size,0,0,"test",&ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)",ret);
  else
    INFO("dataspace %u at %x.%x\n",ds.id,
	 ds.manager.id.task,ds.manager.id.lthread);

  ret = l4dm_transfer(&ds,owner);
  if (ret < 0)
    Panic("transfer ownership failed (%d)",ret);

  ret = l4dm_mem_open(dsm_id,size,0,0,"test",&ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)",ret);
  else
    INFO("dataspace %u at %x.%x\n",ds.id,
	 ds.manager.id.task,ds.manager.id.lthread);

#if 0
  ret = l4dm_transfer(&ds,owner);
  if (ret < 0)
    Panic("transfer ownership failed (%d)",ret);
#endif

  ret = l4dm_mem_open(dsm_id,size,0,0,"test",&ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)",ret);
  else
    INFO("dataspace %u at %x.%x\n",ds.id,
	 ds.manager.id.task,ds.manager.id.lthread);

  ret = l4dm_transfer(&ds,owner);
  if (ret < 0)
    Panic("transfer ownership failed (%d)",ret);

  l4dm_ds_list_all(dsm_id);

  KDEBUG("opened.");

  ret = l4dm_close_all(dsm_id,owner,0);
  if (ret < 0)
    Panic("close all failed (%d)",ret);

  l4dm_ds_list_all(dsm_id);

  KDEBUG("closed.");
}

/*****************************************************************************
 *** l4dm_ds_dump / l4dm_ds_list / l4dm_mem_size / l4dm_ds_set/get_name
 *****************************************************************************/
void
test_debug(void);

void
test_debug(void)
{
  int ret;
  l4dm_dataspace_t ds,dump;
  char name[128];
  l4_size_t dump_size;
  l4_addr_t addr;

  l4_size_t size = (32 * 1024);

  KDEBUG("start...");

  ret = l4dm_mem_open(dsm_id,size,0,0,"test",&ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)",ret);
  else
    INFO("dataspace %u at %x.%x\n",ds.id,
	 ds.manager.id.task,ds.manager.id.lthread);

  ret = l4dm_ds_dump(dsm_id,L4_INVALID_ID,0,&dump);
  if (ret < 0)
    Panic("dump dataspaces failed (%d)",ret);
  else
    INFO("dumped to ds %u at at %x.%x\n",dump.id,
	 dump.manager.id.task,dump.manager.id.lthread);

  ret = l4dm_ds_set_name(&dump,"blub");
  if (ret < 0)
    Panic("set dataspace name failed (%d)",ret);
  
  ret = l4dm_ds_get_name(&dump,name);
  if (ret < 0)
    Panic("get dataspace name failed (%d)",ret);
  else
    INFO("name \'%s\'\n",name);

  l4dm_ds_list(dsm_id,l4_myself(),L4DM_SAME_TASK);
  //l4dm_ds_list(dsm_id,L4_INVALID_ID);

  KDEBUG("opened.");

  ret = l4dm_mem_size(&dump,&dump_size);
  if (ret < 0)
    Panic("get dump ds size failed (%d)",ret);

  ret = l4rm_attach(&dump,dump_size,0,L4DM_RW,(void **)&addr);
  if (ret < 0)
    Panic("attach failed (%d)",ret);
  else
    INFO("attached to address 0x%08x\n",addr);

  KDEBUG("attached.");

  Msg((char *)addr);

  KDEBUG("done.");
}

/*****************************************************************************
 *** l4dm_mem_resize
 *****************************************************************************/
void 
test_resize(void);

void
test_resize(void)
{
  int ret,i;
  l4dm_dataspace_t ds;
  l4_addr_t addr;
  l4_size_t psize;
  l4dm_mem_addr_t paddrs[16];

  l4_size_t size = (32 * 1024);

  __scatter_dm_phys();

  KDEBUG("start...");

  ret = l4dm_mem_open(dsm_id,size,0,0,"test",&ds);
  //  ret = l4dm_memphys_open(L4DM_MEMPHYS_DEFAULT,0x015fe000,0x2000,0x1000,
  //			  L4DM_CONTIGUOUS,"test",&ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)",ret);
  else
    INFO("dataspace %u at %x.%x\n",ds.id,
	 ds.manager.id.task,ds.manager.id.lthread);

  ret = l4rm_attach(&ds,size,0,L4DM_RW,(void **)&addr);
  if (ret < 0)
    Panic("attach failed (%d)",ret);
  else
    INFO("attached to address 0x%08x\n",addr);

  ret = l4dm_map((void *)addr,size,L4DM_RW);
  if (ret < 0)
    Panic("map failed (%d)",ret);

  KDEBUG("mapped.");

  ret = l4dm_mem_resize(&ds,0x2000);
  if (ret < 0)
    Panic("resize dataspace failed (%d)!",ret);

  KDEBUG("resized.");

  ret = l4rm_attach(&ds,0x2000,0,L4DM_RW,(void **)&addr);
  if (ret < 0)
    Panic("attach failed (%d)",ret);
  else
    INFO("attached to address 0x%08x\n",addr);

  ret = l4dm_map((void *)addr,0x2000,L4DM_RW);
  if (ret < 0)
    Panic("map failed (%d)",ret);

  ret = l4dm_mem_phys_addr((void *)addr,0x2000,paddrs,16,&psize);
  if (ret < 0)
    Panic("get phys. address failed (%d)",ret);
  else
    {
      INFO("got %d region(s) (size 0x%08x):\n",ret,psize);
      for (i = 0; i < ret; i++)
	{
	  DMSG("  0x%08x-0x%08x, size 0x%08x\n",paddrs[i].addr,
	       paddrs[i].addr + paddrs[i].size,paddrs[i].size);
	}
    }

  KDEBUG("paddr");
}

/*****************************************************************************
 *** l4dm_mem_ds_lock/unlock, l4dm_mem_lock/unlock
 *****************************************************************************/
void
test_lock(void);

void
test_lock(void)
{
  int ret;
  l4dm_dataspace_t ds;
  l4_addr_t addr;

  l4_size_t size = (32 * 1024);

  KDEBUG("start...");

  ret = l4dm_mem_open(dsm_id,size,0,0,"test",&ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)",ret);
  else
    INFO("dataspace %u at %x.%x\n",ds.id,
	 ds.manager.id.task,ds.manager.id.lthread);

  ret = l4rm_attach(&ds,size,0,L4DM_RW,(void **)&addr);
  if (ret < 0)
    Panic("attach failed (%d)",ret);
  else
    INFO("attached to address 0x%08x\n",addr);
  
  ret = l4rm_attach_to_region(&ds,(void *)(addr + size),size,0,L4DM_RW);
  if (ret < 0)
    {
      INFO("addr 0x%08x\n",addr + size);
      l4rm_show_region_list();
      Panic("attach failed (%d)",ret);
    }
  else
    INFO("attached to address 0x%08x\n",addr + size);

  ret = l4dm_mem_lock((void *)(addr + 0x3456),0x9876);
  if (ret < 0)
    Panic("lock failed (%d)",ret);

  KDEBUG("locked.");

  ret = l4dm_mem_unlock((void *)addr,2 * size);
  if (ret < 0)
    Panic("unlock failed (%d)",ret);
  
  KDEBUG("unlocked.");
  
  ret = l4dm_mem_ds_lock(&ds,0x1234,0x5678);
  if (ret < 0)
    Panic("lock failed (%d)",ret);

  KDEBUG("locked.");

  ret = l4dm_mem_ds_unlock(&ds,0x1234,0x5678);
  if (ret < 0)
    Panic("unlock failed (%d)",ret);
  
  KDEBUG("unlocked.");
}

/*****************************************************************************
 *** l4dm_mem_ds_phys_addr / l4dm_mem_phys_addr
 *****************************************************************************/
void
test_paddr(void);

void
test_paddr(void)
{
  l4dm_dataspace_t ds;
  int ret,i;
  l4_addr_t addr,paddr;
  l4_size_t psize;
  l4dm_mem_addr_t paddrs[16];

  l4_size_t size = (24 * 1024);
  
  __scatter_dm_phys();

  KDEBUG("start...");

  ret = l4dm_mem_open(dsm_id,size,0,0,"test",&ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)",ret);
  else
    INFO("dataspace %u at %x.%x\n",ds.id,
	 ds.manager.id.task,ds.manager.id.lthread);
  
  ret = l4dm_mem_ds_phys_addr(&ds,0x800,0x1000,&paddr,&psize);
  if (ret < 0)
    Panic("get phys. address failed (%d)",ret);
  else
    INFO("phys. addr 0x%08x, size 0x%08x\n",paddr,psize);

  ret = l4rm_attach(&ds,size,0,L4DM_RW,(void **)&addr);
  if (ret < 0)
    Panic("attach failed (%d)",ret);
  else
    INFO("attached to address 0x%08x\n",addr);

  ret = l4rm_attach_to_region(&ds,(void *)(addr + size),size,0,L4DM_RW);
  if (ret < 0)
    {
      INFO("addr 0x%08x\n",addr + size);
      l4rm_show_region_list();
      Panic("attach failed (%d)",ret);
    }
  else
    INFO("attached to address 0x%08x\n",addr + size);
			      
  KDEBUG("attached.");

  ret = l4dm_mem_phys_addr((void *)(addr + 0x6abc),0x2345,paddrs,16,&psize);
  if (ret < 0)
    Panic("get phys. address for VM region 0x%08x-0x%08x failed (%d)",
	  addr,addr + size,ret);
  else
    {
      INFO("got %d region(s) (size 0x%08x):\n",ret,psize);
      for (i = 0; i < ret; i++)
	{
	  DMSG("  0x%08x-0x%08x, size 0x%08x\n",paddrs[i].addr,
	       paddrs[i].addr + paddrs[i].size,paddrs[i].size);
	}
    }

  KDEBUG("done.");
}

/*****************************************************************************
 *** l4dm_copy / l4dm_mem_size
 *****************************************************************************/
void
test_copy(void);

void
test_copy(void)
{
  int ret;
  l4dm_dataspace_t ds,copy;
  l4_addr_t addr;
  l4_size_t ds_size;

  l4_size_t size = (64 * 1024);

  __scatter_dm_phys();

  KDEBUG("start...");

  ret = l4dm_mem_open(dsm_id,size,0,0,"test",&ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)",ret);
  else
    INFO("dataspace %u at %x.%x\n",ds.id,
	 ds.manager.id.task,ds.manager.id.lthread);

  ret = l4rm_attach(&ds,size,0,L4DM_RW,(void **)&addr);
  if (ret < 0)
    Panic("attach failed (%d)",ret);
  else
    INFO("attached to address 0x%08x\n",addr);
  memset((void *)addr,0x88,size);

  KDEBUG("opened.");

  ret = l4dm_copy_long(&ds,0x800,1,0x7800,0,"test copy",&copy);
  if (ret < 0)
    Panic("copy failed (%d)",ret);
  else
    INFO("copy: %u at %x.%x\n",copy.id,
	 copy.manager.id.task,copy.manager.id.lthread);

  ret = l4dm_mem_size(&copy,&ds_size);
  if (ret < 0)
    Panic("get size failed (%d)",ret);
  else
    INFO("copy size: 0x%08x\n",ds_size);

  KDEBUG("copied.");
}

/*****************************************************************************
 *** l4dm_map
 *****************************************************************************/
void
test_map(void);

void
test_map(void)
{
  int ret;
  l4dm_dataspace_t ds;
  l4_addr_t esp,addr;
  l4_offs_t offs;
  l4_addr_t ds_map_addr,fpage_addr;
  l4_size_t ds_map_size,fpage_size;

  l4_size_t size = (64 * 1024);

  __scatter_dm_phys();

  KDEBUG("start...");

  ret = l4dm_mem_open(dsm_id,size,size,L4DM_CONTIGUOUS,"test",&ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)",ret);
  else
    INFO("dataspace %u at %x.%x\n",ds.id,
	ds.manager.id.task,ds.manager.id.lthread);

  KDEBUG("opened.");

  ret = l4rm_attach(&ds,size,0,L4RM_LOG2_ALIGNED,(void **)&addr);
  //ret = l4rm_attach(&ds,size,0,0,(void **)&addr);
  if (ret < 0)
    Panic("attach failed (%d)",ret);
  else
    INFO("attached to address 0x%08x\n",addr);

  ret = l4dm_map((void *)addr,L4_PAGESIZE,L4DM_RW | L4DM_MAP_MORE);
  if (ret < 0)
    Panic("map failed (%d)",ret);

  KDEBUG("mapped.");

  ret = l4dm_close(&ds);
  if (ret < 0)
    Panic("close failed (%d)",ret);

  KDEBUG("next...");

  asm("movl   %%esp, %0\n\t" : "=r" (esp) : );
  INFO("stack at 0x%08x\n",esp);

  ret = l4rm_lookup((void *)esp,&ds,&offs,&ds_map_addr,&ds_map_size);
  if(ret < 0)
    Panic("l4rm_lookup failed (%d)!",ret);

  Msg("  ds %u at %x.%x, offset 0x%08x, map area 0x%08x-0x%08x\n",
      ds.id,ds.manager.id.task,ds.manager.id.lthread,offs,
      ds_map_addr,ds_map_addr + ds_map_size);

  addr = 0x10000000;
  ret = l4dm_map_pages(&ds,0,0x1000,addr,12,0,L4DM_RO,
		       &fpage_addr,&fpage_size);
  if (ret < 0)
    Error("map failed (%d)\n",ret);
  else
    INFO("mapped to 0x%08x\n",addr);

  KDEBUG("done");
}

/*****************************************************************************
 *** open / map / close
 *****************************************************************************/
void
test_open(void);

void
test_open(void)
{
  int ret;
  l4dm_dataspace_t ds;
  l4_addr_t fpage_addr;
  l4_size_t fpage_size;

  // ret = l4dm_mem_open(dsm_id,0x400000,0x400000,L4DM_CONTIGUOUS,"test",&ds);
  ret = l4dm_mem_open(dsm_id,0x400000,0,0,"test",&ds);
  if (ret < 0)
    Error("open dataspace failed (%d)",ret);
  else
    Msg("dataspace %u at %x.%x\n",ds.id,
	ds.manager.id.task,ds.manager.id.lthread);

  KDEBUG("opened.");

  ret = l4dm_map_pages(&ds,
		       0x0000,                /* offset */
		       0x3000,                /* size */
		       0x10000000,            /* receive window address */
		       28,                    /* receive window size (log2)*/
		       0x00001000,            /* offset in receive window */
		       L4DM_RO | L4DM_MAP_PARTIAL,
		       &fpage_addr,&fpage_size);
  if (ret < 0)
    Error("map page failed (%d)",ret);

  KDEBUG("mapped.");

  ret = l4dm_close(&ds);
  if (ret < 0)
    Error("close dataspace failed (%d)",ret);

  KDEBUG("done");
}

/*****************************************************************************
 *** l4dm_mem_ds_is_contiguous
 *****************************************************************************/
void
test_is_contiguous(void);

void
test_is_contiguous(void)
{
  int ret;
  l4dm_dataspace_t ds;
  
  l4_size_t size = 64 * 1024;

  __scatter_dm_phys();

  KDEBUG("start...");

  ret = l4dm_mem_open(dsm_id,size,0,0,"not contiguous",&ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)",ret);
  else
    l4dm_ds_show(&ds);

  if (l4dm_mem_ds_is_contiguous(&ds))
    INFO("ds is contiguous.\n");
  else
    INFO("ds is not contiguous.\n");

  ret = l4dm_mem_open(dsm_id,size,0,L4DM_CONTIGUOUS,"contiguous",&ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)",ret);
  else
    l4dm_ds_show(&ds);

  if (l4dm_mem_ds_is_contiguous(&ds))
    INFO("ds is contiguous.\n");
  else
    INFO("ds is not contiguous.\n");

  KDEBUG("done");
}

/*****************************************************************************
 *** Test
 *****************************************************************************/
void
test(void);

void
test(void)
{
  l4dm_dataspace_t ds;
  int ret;

  KDEBUG("start...");

  __scatter_dm_phys();

  l4dm_memphys_show_pool_areas(0);

  ret = l4dm_mem_open(L4DM_DEFAULT_DSM,0x20000000,0,0,"blub",&ds);
  if (ret < 0)
    Panic("open dataspace failed: %s (%d)",l4env_errstr(ret),ret);
  else
    l4dm_ds_show(&ds);

  KDEBUG("next...");

  l4dm_memphys_show_memmap();

  KDEBUG("next...");

  ret = l4dm_memphys_open(0,                          // pool
			  0x08000000,                 // addr
                          0x00001000,                 // size
                          0x00001000,                 // align 
                          0,                          // flags
                          "test",                     // name
			  &ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)",ret);
  else
    INFO("dataspace %u at %x.%x\n",ds.id,
	 ds.manager.id.task,ds.manager.id.lthread);

  l4dm_memphys_show_pool_areas(0);

  KDEBUG("opened");

  l4dm_close(&ds);

  l4dm_memphys_show_pool_areas(0);

  KDEBUG("closed");
}

/*****************************************************************************
 *** Main
 *****************************************************************************/
int
main(int argc, char * argv[])
{
  /* Set log tag */
  LOG_init("DMphysTE");

  /* get DMphys thread id */
  dsm_id = l4dm_memphys_find_dmphys();
  if (l4_is_invalid_id(dsm_id))
    Panic("DMphys not found!");

  INFO("DMphys at %x.%x\n",dsm_id.id.task,dsm_id.id.lthread);

  /* init OSKit malloc */
  OSKit_libc_support_init(0x10000);

  l4rm_show_region_list();
  l4dm_ds_list_all(dsm_id);

  /* do tests */
#if 0
  test();
#endif

#if 1
  test_is_contiguous();
#endif

#if 0
  test_open();
#endif

#if 0
  test_map();
#endif

#if 0
  test_copy();
#endif

#if 0
  test_paddr();
#endif

#if 0
  test_lock();
#endif

#if 0
  test_resize();
#endif

#if 0
  test_debug();
#endif

#if 0
  test_transfer();
#endif

#if 0
  test_allocate();
#endif

#if 0
  test_debug_DMphys();
#endif

#if 0
  test_phys_copy();
#endif

#if 0
  test_check_pagesize();
#endif

#if 0
  stress_test();
#endif

  KDEBUG("really done.");

  return 0;
}
