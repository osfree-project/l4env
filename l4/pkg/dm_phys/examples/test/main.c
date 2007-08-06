/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/examples/test/main.c
 * \brief  DMphys test application
 *
 * \date   11/23/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* standard includes */
#include <stdio.h>
#include <string.h>

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/util/stack.h>
#include <l4/util/reboot.h>
#include <l4/log/l4log.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/l4rm/l4rm.h>
#include <l4/events/events.h>

/* DMphys includes */
#include <l4/dm_phys/dm_phys.h>

char LOG_tag[9] = "DMphysTE";
l4_ssize_t l4libc_heapsize = 4096;

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
      ret = l4dm_mem_open(dsm_id, SCATTER_DS_SIZE, 0, 0, "dummy-dum", &ds[i]);
      if (ret < 0)
	Panic("open dummy dataspace failed (%d)!", ret);      
    }

  for (i = 0; i < SCATTER_NUM_DS; i += 2)
    {
      ret = l4dm_close(&ds[i]);
      if (ret < 0)
	Panic("close dummy dataspace failed (%d)!", ret);
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
  int i, ret;

  l4dm_memphys_show_pool_areas(0);
  LOG("start...");

  for (i = 0; i < NUM_DS; i++)
    {
      ret = l4dm_mem_open(dsm_id, DS_SIZE, 0, 0, "dummy", &ds[i]);
      if (ret < 0)
	Panic("open dummy dataspace failed (%d)!", ret);      
    }

  //  l4dm_memphys_show_pool_areas(0);

  l4dm_memphys_show_slabs(0);
  LOG("opened.");

  for (i = NUM_DS - 1; i >= 0; i--)
    //for (i = 0; i < NUM_DS; i++)
    {
      ret = l4dm_close(&ds[i]);
      if (ret < 0)
	Panic("close dummy dataspace failed (%d)!", ret);
    }

  LOG("closed.");

  l4dm_memphys_show_slabs(0);

  LOG("slabs.");

  l4dm_memphys_show_pool_areas(0);
  l4dm_ds_list_all(dsm_id);

  LOG("continue...");
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
  int ret, ok;
  l4_addr_t addr;
  l4_offs_t offs;
  l4_size_t size;
  void * ptr;
  
  LOG("start...");

  ptr = l4dm_mem_ds_allocate(0x00400000,
			     L4DM_MEMPHYS_SUPERPAGES | L4RM_LOG2_ALIGNED | 
			     L4RM_LOG2_ALLOC | L4RM_MAP | L4DM_MAP_MORE,
			     &ds);
  if (ptr == NULL)
    Panic("allocate failed!");
  else
    LOGL("dataspace %u at "l4util_idfmt", mapped at 0x%08lx",
         ds.id, l4util_idstr(ds.manager), (l4_addr_t)ptr);

  l4dm_ds_show(&ds);
  
  ret = l4dm_memphys_check_pagesize(ptr, 0x00400000, L4_LOG2_SUPERPAGESIZE);
  LOGL("check_pagesize = %d", ret);

  l4dm_close(&ds);

  LOG("next...");

  ret = l4dm_memphys_open(L4DM_MEMPHYS_DEFAULT,     // pool
			  0x02000000,               // address
			  0x00400000,               // size
			  //L4_SUPERPAGESIZE,         // alignment
			  0,
                          //L4DM_MEMPHYS_4MPAGES,     // flags
			  L4DM_CONTIGUOUS,
                          "test", &ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)", ret);
  else
    LOGL("dataspace %u at "l4util_idfmt, ds.id, l4util_idstr(ds.manager));
  
  l4dm_ds_show(&ds);

  addr = 0x10000000;
  offs = 0x00000000;
  size = 0x00400000;
  ret = l4rm_attach_to_region(&ds, (void *)addr, size, offs, L4DM_RW);
  if (ret < 0)
    Panic("attach dataspace failed (%d)", ret);
  else
    LOGL("attached to addr 0x%08lx", addr);

  ret = l4dm_memphys_open(L4DM_MEMPHYS_DEFAULT,     // pool
			  0x02400000,               // address
			  0x00400000,               // size
			  //L4_SUPERPAGESIZE,         // alignment
			  0,
                          //L4DM_MEMPHYS_4MPAGES,     // flags
			  L4DM_CONTIGUOUS,
                          "test", &ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)", ret);
  else
    LOGL("dataspace %u at "l4util_idfmt, ds.id, l4util_idstr(ds.manager));

  l4dm_ds_show(&ds);

  addr = 0x10400000;
  offs = 0x00000000;
  size = 0x00400000;
  ret = l4rm_attach_to_region(&ds, (void *)addr, size, offs, L4DM_RW);
  if (ret < 0)
    Panic("attach dataspace failed (%d)", ret);
  else
    LOGL("attached to addr 0x%08lx", addr);

  LOG("attached.");

  addr = 0x10000000;
  size = 0x00800000;
  ret = l4dm_memphys_check_pagesize((void *)addr, size, L4_LOG2_SUPERPAGESIZE);
  LOGL("check_pagesize = %d", ret);

  LOG("next...");

  ret = l4dm_memphys_pagesize(&ds, 0, L4_SUPERPAGESIZE, L4_LOG2_SUPERPAGESIZE,
                              &ok);
  if (ret < 0)  
    Panic("check pagesize failed (%d)", ret);
  else
    LOGL("ok = %d", ok);

  LOG("done");
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
  void *addr;

  l4_size_t size = (24 * 1024);

  LOG("start...");

  ret = l4dm_mem_open(dsm_id, size, 0, 0, "test", &ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)", ret);
  else
    LOGL("dataspace %u at "l4util_idfmt, ds.id, l4util_idstr(ds.manager));

  ret = l4rm_attach(&ds, size, 0, L4DM_RW, &addr);
  if (ret < 0)
    Panic("attach failed (%d)", ret);
  else
    LOGL("attached to address %p", (void*)addr);
  memset(addr, 0x88, size);

  LOG("opened.");

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
    Panic("copy failed (%d)", ret);
  else
    LOGL("copy %u at "l4util_idfmt, ds.id, l4util_idstr(ds.manager));

  LOG("copied.");

  l4dm_memphys_show_pool_areas(L4DM_MEMPHYS_ISA_DMA);

  LOG("done.");
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
  l4_addr_t esp, ds_map_addr;
  l4_offs_t offs;
  l4_size_t ds_map_size;
  l4_threadid_t dummy;

  LOG("start...");

  esp = l4util_stack_get_sp();
  LOGL("stack at 0x%08lx", esp);

  ret = l4rm_lookup((void *)esp, &ds_map_addr, &ds_map_size, 
                    &ds, &offs, &dummy);
  if(ret < 0)
    Panic("l4rm_lookup failed (%d)!", ret);

  if (ret != L4RM_REGION_DATASPACE)
    Panic("invalid region type %d!", ret);

  printf("  ds %u at "l4util_idfmt", offset 0x%08lx, map area 0x%08lx-0x%08lx\n",
         ds.id, l4util_idstr(ds.manager), offs, ds_map_addr, 
         ds_map_addr + ds_map_size);

  l4dm_ds_show(&ds);

  LOG("next...");

  l4dm_memphys_show_pools();

  LOG("pools");

  l4dm_memphys_show_pool_areas(L4DM_MEMPHYS_ISA_DMA);

  LOG("areas");

  l4dm_memphys_show_pool_free(L4DM_MEMPHYS_ISA_DMA);

  LOG("free lists");
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

  LOG("start...");

  ptr = l4dm_mem_allocate(size, L4RM_MAP);
  if (ptr == NULL)
    Panic("memory allocation failed!");
  else
    LOGL("got mem at 0x%08lx", (l4_addr_t)ptr);

  LOG("allocated.");

  l4dm_mem_release(ptr);

  LOG("released.");
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

  LOG("TEST_TRANSFER");

  LOG("start...");

  owner = l4_myself();
  owner.id.lthread = 5;

  ret = l4dm_mem_open(dsm_id, size, 0, 0, "test", &ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)", ret);
  else
    LOGL("dataspace %u at "l4util_idfmt, ds.id, l4util_idstr(ds.manager));

  ret = l4dm_transfer(&ds, owner);
  if (ret < 0)
    Panic("transfer ownership failed (%d)", ret);

  ret = l4dm_mem_open(dsm_id, size, 0, 0, "test", &ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)", ret);
  else
    LOGL("dataspace %u at "l4util_idfmt, ds.id, l4util_idstr(ds.manager));

#if 0
  ret = l4dm_transfer(&ds, owner);
  if (ret < 0)
    Panic("transfer ownership failed (%d)", ret);
#endif

  ret = l4dm_mem_open(dsm_id, size, 0, 0, "test", &ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)", ret);
  else
    LOGL("dataspace %u at "l4util_idfmt, ds.id, l4util_idstr(ds.manager));

  ret = l4dm_transfer(&ds, owner);
  if (ret < 0)
    Panic("transfer ownership failed (%d)", ret);

  l4dm_ds_list_all(dsm_id);

  LOG("opened.");

  /* do NOT close all dataspaces, because that would also close the dataspace
   * containing out code segment, and whoops, its all gone.
  ret = l4dm_close_all(dsm_id, owner, 0);
  if (ret < 0)
    Panic("close all failed (%d)", ret);

  l4dm_ds_list_all(dsm_id);
  */

  LOG("closed.");
}

/*****************************************************************************
 *** l4dm_ds_list / l4dm_mem_size / l4dm_ds_set/get_name
 *****************************************************************************/
void
test_debug(void);

void
test_debug(void)
{
  int ret;
  l4dm_dataspace_t ds;
  char name[128];
  l4_size_t ds_size;
  void *addr;

  l4_size_t size = (32 * 1024);

  LOG("start...");

  ret = l4dm_mem_open(dsm_id, size, 0, 0, "test", &ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)", ret);
  else
    LOGL("dataspace %u at "l4util_idfmt, ds.id, l4util_idstr(ds.manager));

  ret = l4dm_ds_set_name(&ds, "blub");
  if (ret < 0)
    Panic("set dataspace name failed (%d)", ret);
  
  ret = l4dm_ds_get_name(&ds, name);
  if (ret < 0)
    Panic("get dataspace name failed (%d)", ret);
  else
    LOGL("name \'%s\'", name);

  l4dm_ds_list(dsm_id, l4_myself(), L4DM_SAME_TASK);
  //l4dm_ds_list(dsm_id,L4_INVALID_ID);

  LOG("opened.");

  ret = l4dm_mem_size(&ds, &ds_size);
  if (ret < 0)
    Panic("get ds size failed (%d)", ret);

  ret = l4rm_attach(&ds, ds_size, 0, L4DM_RW, &addr);
  if (ret < 0)
    Panic("attach failed (%d)", ret);
  else
    LOGL("attached to address %p", (void*)addr);

  LOG("attached.");

  printf((char *)addr);

  LOG("done.");
}

/*****************************************************************************
 *** l4dm_mem_resize
 *****************************************************************************/
void 
test_resize(void);

void
test_resize(void)
{
  int ret, i;
  l4dm_dataspace_t ds;
  void *addr;
  l4_size_t psize;
  l4dm_mem_addr_t paddrs[16];

  l4_size_t size = (32 * 1024);

  __scatter_dm_phys();

  LOG("start...");

  ret = l4dm_mem_open(dsm_id, size, 0, 0, "test", &ds);
  //  ret = l4dm_memphys_open(L4DM_MEMPHYS_DEFAULT, 0x015fe000, 0x2000, 0x1000,
  //			  L4DM_CONTIGUOUS, "test", &ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)", ret);
  else
    LOGL("dataspace %u at "l4util_idfmt, ds.id, l4util_idstr(ds.manager));

  ret = l4rm_attach(&ds, size, 0, L4DM_RW, &addr);
  if (ret < 0)
    Panic("attach failed (%d)", ret);
  else
    LOGL("attached to address %p", (void*)addr);

  ret = l4dm_map(addr, size, L4DM_RW);
  if (ret < 0)
    Panic("map failed (%d)", ret);

  LOG("mapped.");

  ret = l4dm_mem_resize(&ds, 0x2000);
  if (ret < 0)
    Panic("resize dataspace failed (%d)!", ret);

  LOG("resized.");

  ret = l4rm_attach(&ds, 0x2000, 0, L4DM_RW, &addr);
  if (ret < 0)
    Panic("attach failed (%d)", ret);
  else
    LOGL("attached to address %p", (void*)addr);

  ret = l4dm_map(addr, 0x2000, L4DM_RW);
  if (ret < 0)
    Panic("map failed (%d)", ret);

  ret = l4dm_mem_phys_addr(addr, 0x2000, paddrs, 16, &psize);
  if (ret < 0)
    Panic("get phys. address failed (%d)", ret);
  else
    {
      LOGL("got %d region(s) (size 0x%08zx):", ret, psize);
      for (i = 0; i < ret; i++)
	{
	  printf("  0x%08lx-0x%08lx, size 0x%08zx\n", paddrs[i].addr, 
                 paddrs[i].addr + paddrs[i].size, paddrs[i].size);
	}
    }

  LOG("paddr");
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
  void *addr;

  l4_size_t size = (32 * 1024);

  LOG("start...");

  ret = l4dm_mem_open(dsm_id, size, 0, 0, "test", &ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)", ret);
  else
    LOGL("dataspace %u at "l4util_idfmt, ds.id, l4util_idstr(ds.manager));

  ret = l4rm_attach(&ds, size, 0, L4DM_RW, &addr);
  if (ret < 0)
    Panic("attach failed (%d)", ret);
  else
    LOGL("attached to address %p", (void*)addr);
  
  ret = l4rm_attach_to_region(&ds, (void *)((l4_addr_t)addr + size),
			      size, 0, L4DM_RW);
  if (ret < 0)
    {
      LOGL("addr %p", (char *)addr + size);
      l4rm_show_region_list();
      Panic("attach failed (%d)", ret);
    }
  else
    LOGL("attached to address %p", (char *)addr + size);

  ret = l4dm_mem_lock((void *)(addr + 0x3456), 0x9876);
  if (ret < 0)
    Panic("lock failed (%d)", ret);

  LOG("locked.");

  ret = l4dm_mem_unlock((void *)addr, 2 * size);
  if (ret < 0)
    Panic("unlock failed (%d)", ret);
  
  LOG("unlocked.");
  
  ret = l4dm_mem_ds_lock(&ds, 0x1234, 0x5678);
  if (ret < 0)
    Panic("lock failed (%d)", ret);

  LOG("locked.");

  ret = l4dm_mem_ds_unlock(&ds, 0x1234, 0x5678);
  if (ret < 0)
    Panic("unlock failed (%d)", ret);
  
  LOG("unlocked.");
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
  int ret, i;
  l4_addr_t paddr;
  void *addr;
  l4_size_t psize;
  l4dm_mem_addr_t paddrs[16];

  l4_size_t size = (24 * 1024);

  __scatter_dm_phys();

  LOG("start...");

  ret = l4dm_mem_open(dsm_id, size, 0, 0, "test", &ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)", ret);
  else
    LOGL("dataspace %u at "l4util_idfmt, ds.id, l4util_idstr(ds.manager));

  ret = l4dm_mem_ds_phys_addr(&ds, 0x800, 0x1000, &paddr, &psize);
  if (ret < 0)
    Panic("get phys. address failed (%d)", ret);
  else
    LOGL("phys. addr 0x%08lx,  size 0x%08zx", paddr, psize);

  ret = l4rm_attach(&ds, size, 0, L4DM_RW, &addr);
  if (ret < 0)
    Panic("attach failed (%d)", ret);
  else
    LOGL("attached to address 0x%08lx", (unsigned long)addr);

  ret = l4rm_attach_to_region(&ds, (void *)((l4_addr_t)addr + size), 
			      size, 0, L4DM_RW);
  if (ret < 0)
    {
      LOGL("addr 0x%08lx", (unsigned long)addr + size);
      l4rm_show_region_list();
      Panic("attach failed (%d)", ret);
    }
  else
    LOGL("attached to address 0x%08lx", (unsigned long)addr + size);

  LOG("attached.");

  ret = l4dm_mem_phys_addr((void *)((l4_addr_t)addr + 0x6abc),
			   0x2345, paddrs, 16, &psize);
  if (ret < 0)
    Panic("get phys. address for VM region 0x%08lx-0x%08lx failed (%d)",
	  (l4_addr_t)addr, (l4_addr_t)addr + size, ret);
  else
    {
      LOGL("got %d region(s) (size 0x%08zx):", ret, psize);
      for (i = 0; i < ret; i++)
	{
	  printf("  0x%08lx-0x%08lx,  size 0x%08zx\n", paddrs[i].addr,
                 paddrs[i].addr + paddrs[i].size, paddrs[i].size);
	}
    }

  LOG("done.");
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
  l4dm_dataspace_t ds, copy;
  void *addr;
  l4_size_t ds_size;

  l4_size_t size = (64 * 1024);

  __scatter_dm_phys();

  LOG("start...");

  ret = l4dm_mem_open(dsm_id, size, 0, 0, "test", &ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)", ret);
  else
    LOGL("dataspace %u at "l4util_idfmt, ds.id, l4util_idstr(ds.manager));

  ret = l4rm_attach(&ds, size, 0, L4DM_RW, &addr);
  if (ret < 0)
    Panic("attach failed (%d)", ret);
  else
    LOGL("attached to address %p", (void*)addr);
  memset(addr, 0x88, size);

  LOG("opened.");

  ret = l4dm_copy_long(&ds, 0x800, 1, 0x7800, 0, "test copy", &copy);
  if (ret < 0)
    Panic("copy failed (%d)", ret);
  else
    LOGL("copy: %u at "l4util_idfmt, copy.id, l4util_idstr(copy.manager));

  ret = l4dm_mem_size(&copy, &ds_size);
  if (ret < 0)
    Panic("get size failed (%d)", ret);
  else
    LOGL("copy size: 0x%08zx", ds_size);

  LOG("copied.");
}

/*****************************************************************************
 *** l4dm_map
 *****************************************************************************/
void
test_map(void);

void
test_map(void)
{
  int ret, i;
  l4dm_dataspace_t ds;
  l4_addr_t esp;
  void *addr;
  l4_offs_t offs;
  l4_addr_t ds_map_addr, fpage_addr;
  l4_size_t ds_map_size, fpage_size;
  l4_threadid_t dummy;

  l4_size_t size = (64 * 1024);
 
  LOG("TEST_MAP");
  
  __scatter_dm_phys();

  LOG("start...");

  ret = l4dm_mem_open(dsm_id, size, size, L4DM_CONTIGUOUS, "test", &ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)", ret);
  else
    LOGL("dataspace %u at "l4util_idfmt, ds.id, l4util_idstr(ds.manager));

  LOG("opened.");

  ret = l4rm_attach(&ds, size, 0, L4RM_LOG2_ALIGNED, &addr);
  //ret = l4rm_attach(&ds, size, 0, 0, (void **)&addr);
  if (ret < 0)
    Panic("attach failed (%d)", ret);
  else
    LOGL("attached to address %p", (void*)addr);

  ret = l4dm_map(addr, L4_PAGESIZE, L4DM_RW | L4DM_MAP_MORE);
  if (ret < 0)
    Panic("map failed (%d)", ret);

  LOG("mapped.");

  for (i=0; i<16; i++)
    memset(addr + i*L4_PAGESIZE, '0'+i, L4_PAGESIZE);

  addr = (void*)0x20003000;
  ret = l4dm_map_ds(&ds, 0x1000, (l4_addr_t)addr, 0xA000, L4DM_RO);
  if (ret < 0)
    LOG_Error("map failed (%d)", ret);
  else
    LOGL("mapped to %p", (void*)addr);

  ret = l4dm_close(&ds);
  if (ret < 0)
    Panic("close failed (%d)", ret);

  LOG("next...");

  esp = l4util_stack_get_sp();
  LOGL("stack at 0x%08lx", esp);

  ret = l4rm_lookup((void *)esp, &ds_map_addr, &ds_map_size, 
                    &ds, &offs, &dummy);
  if(ret < 0)
    Panic("l4rm_lookup failed (%d)!", ret);

  if (ret != L4RM_REGION_DATASPACE)
    Panic("invalid region type %d!", ret);

  printf("  ds %u at "l4util_idfmt", offset 0x%08lx, map area 0x%08lx-0x%08lx\n",
         ds.id, l4util_idstr(ds.manager), offs, ds_map_addr,
         ds_map_addr + ds_map_size);

  addr = (void*)0x10000000;
  ret = l4dm_map_pages(&ds, 0, 0x1000, (l4_addr_t)addr, 12, 0, L4DM_RO, 
		       &fpage_addr, &fpage_size);
  if (ret < 0)
    LOG_Error("map failed (%d)", ret);
  else
    LOGL("mapped to 0x%08lx", (unsigned long)addr);

  LOG("done");
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

  // ret = l4dm_mem_open(dsm_id, 0x400000, 0x400000, L4DM_CONTIGUOUS, "test", &ds);
  ret = l4dm_mem_open(dsm_id, 0x400000, 0, 0, "test", &ds);
  if (ret < 0)
    LOG_Error("open dataspace failed (%d)", ret);
  else
    LOGL("dataspace %u at "l4util_idfmt, ds.id, l4util_idstr(ds.manager));

  LOG("opened.");

  ret = l4dm_map_pages(&ds,
		       0x0000,                /* offset */
		       0x3000,                /* size */
		       0x10000000,            /* receive window address */
		       28,                    /* receive window size (log2)*/
		       0x00001000,            /* offset in receive window */
		       L4DM_RO | L4DM_MAP_PARTIAL,
		       &fpage_addr, &fpage_size);
  if (ret < 0)
    LOG_Error("map page failed (%d)", ret);

  LOG("mapped.");

  ret = l4dm_close(&ds);
  if (ret < 0)
    LOG_Error("close dataspace failed (%d)", ret);

  LOG("done");
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
  l4dm_dataspace_t ds1, ds2;
  
  l4_size_t size = 64 * 1024;

  LOG("TEST_IS_CONTIGOUS");
  
  __scatter_dm_phys();

  LOG("start...");

  ret = l4dm_mem_open(dsm_id, size, 0, 0, "not contiguous", &ds1);
  if (ret < 0)
    Panic("open dataspace failed (%d)", ret);
  else
  {
    LOG("show not contiguous:");
    l4dm_ds_show(&ds1);
  }

  if (l4dm_mem_ds_is_contiguous(&ds1))
    LOGL("ds is contiguous.");
  else
    LOGL("ds is not contiguous.");

  ret = l4dm_mem_open(dsm_id, size*3, 0, L4DM_CONTIGUOUS, "contiguous", &ds2);
  if (ret < 0)
    Panic("open dataspace failed (%d)", ret);
  else
  {
    LOG("show contiguous:");
    l4dm_ds_show(&ds2);
  }

  if (l4dm_mem_ds_is_contiguous(&ds2))
    LOGL("ds is contiguous.");
  else
    LOGL("ds is not contiguous.");

  l4rm_show_region_list();
  l4dm_ds_list_all(dsm_id);
  
  l4dm_close(&ds1);
  l4dm_close(&ds2);
  
  l4dm_ds_list_all(dsm_id);

  LOG("done");
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

  LOG("start...");

  __scatter_dm_phys();

  l4dm_memphys_show_pool_areas(0);

  ret = l4dm_mem_open(L4DM_DEFAULT_DSM, 0x20000000, 0, 0, "blub", &ds);
  if (ret < 0)
    Panic("open dataspace failed: %s (%d)", l4env_errstr(ret), ret);
  else
    l4dm_ds_show(&ds);

  LOG("next...");

  l4dm_memphys_show_memmap();

  LOG("next...");

  ret = l4dm_memphys_open(0,                          // pool
			  0x08000000,                 // addr
                          0x00001000,                 // size
                          0x00001000,                 // align 
                          0,                          // flags
                          "test",                     // name
			  &ds);
  if (ret < 0)
    Panic("open dataspace failed (%d)", ret);
  else
    LOGL("dataspace %u at "l4util_idfmt,  ds.id, l4util_idstr(ds.manager));

  l4dm_memphys_show_pool_areas(0);

  LOG("opened");

  l4dm_close(&ds);

  l4dm_memphys_show_pool_areas(0);

  LOG("closed");
}

void
exit_test(void);

void
exit_test(void)
{
  l4events_nr_t eventnr;
  l4events_event_t event;

  __scatter_dm_phys();
  l4dm_memphys_show_pool_areas(0);

  event.len = sizeof(l4_threadid_t);
  *(l4_threadid_t*)event.str = l4_myself();
  l4events_send(1, &event, &eventnr, L4EVENTS_RECV_ACK);
      
  
  l4dm_memphys_show_pool_areas(0);
}

/*****************************************************************************
 *** Main
 *****************************************************************************/
int
main(int argc, char * argv[])
{
  l4_threadid_t my_id;
  
  /* get DMphys thread id */
  dsm_id = l4dm_memphys_find_dmphys();
  if (l4_is_invalid_id(dsm_id))
    Panic("DMphys not found!");

  LOG("DMphys at: "l4util_idfmt, l4util_idstr(dsm_id));

  
  my_id = l4_myself();
 
  l4rm_show_region_list();
  l4dm_ds_list_all(dsm_id);
 
  /* do tests */
#if 0
  test();
#endif

#if 1
  test_is_contiguous();
#endif

#if 1
  test_open();
#endif

#if 1
  test_map();
#endif

#if 1
  test_copy();
#endif

#if 1
  test_paddr();
#endif

#if 0
  test_lock();
#endif

#if 1
  test_resize();
#endif

#if 1
  test_debug();
#endif

#if 1
  test_transfer();
#endif

#if 1
  test_allocate();
#endif

#if 1
  test_debug_DMphys();
#endif

#if 0
  test_phys_copy();
#endif

#if 1
  test_check_pagesize();
#endif

#if 1
  stress_test();
#endif

#if 1
  exit_test();
#endif
  
  LOG("really done.");
  LOG_flush();
  l4util_reboot();

  return 0;
}
