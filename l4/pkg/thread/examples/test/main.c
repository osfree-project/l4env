/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/examples/test/main.c
 * \brief  Thread lib test app
 *
 * \date   09/11/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/util/macros.h>
#include <l4/util/stack.h>
#include <l4/log/l4log.h>
#include <l4/thread/thread.h>
#include <l4/l4rm/l4rm.h>
#include <l4/env/errno.h>

#include <stdio.h>

char * dat = "Init string\0";
int data_key;
unsigned char private_stack[16384];

char LOG_tag[9] = "th_test";

/*****************************************************************************/
/**
 * \brief L4RM / DMphys test thread 
 */
/*****************************************************************************/ 
void test_fn1(void * data);
void test_fn1(void * data)
{
  l4_addr_t esp,ds_map_addr,low,high;
  l4dm_dataspace_t ds;
  l4_offs_t offs;
  l4_size_t ds_map_size;
  int ret;
  void *addr;
  l4_threadid_t dummy;

  esp = l4util_stack_get_sp();
  LOG("stack pointer at 0x%08lx",esp);

  ret = l4thread_get_stack(l4thread_myself(), &low, &high);
  if (ret < 0)
    Panic("l4thread_get_stack_current failed: %s (%d)", 
          l4env_errstr(ret), ret);

  LOG("stack at 0x%08lx-0x%08lx", low, high);

  ret = l4rm_lookup((void *)esp, &ds_map_addr, &ds_map_size,
                    &ds, &offs, &dummy);
  if(ret < 0)
    Panic("l4rm_lookup failed (%d)!",ret);
  
  if (ret != L4RM_REGION_DATASPACE)
    Panic("invalid region type %d!", ret);

  printf("  ds %u at "l4util_idfmt", offset 0x%08lx, mapped to 0x%08lx-0x%08lx\n",
	 ds.id, l4util_idstr(ds.manager), offs,
	 ds_map_addr,ds_map_addr + ds_map_size);

  ret = l4rm_attach(&ds,L4_PAGESIZE,0,L4DM_RO,&addr);
  if (ret < 0)
    {
      printf("l4rm_attach failed (%d)!\n",ret);
      enter_kdebug("PANIC");
    }
  printf("  attached to addr 0x%08x\n",(unsigned)addr);
}

/*****************************************************************************/
/**
 * \brief Thread test function
 */
/*****************************************************************************/ 
void test_fn(void * data);
void test_fn(void * data)
{
  l4thread_t id = l4thread_myself();
  l4thread_t * id1;

  l4thread_started(NULL);
  l4thread_data_set(id,data_key,&id);
  l4thread_sleep(100);
  id1 = l4thread_data_get(id,data_key);

  LOGL("id = %d, id1 = %d",id,*id1);

  l4thread_exit();
  l4thread_sleep(10000);
}
 
/*****************************************************************************/
/**
 * \brief Main
 */
/*****************************************************************************/ 
int main(void)
{
  int ret;

  /***************************************************************************
   *** L4RM/DMphys test
   ***************************************************************************/
  ret = l4thread_create(test_fn1,(void *)dat,L4THREAD_CREATE_ASYNC);

  printf("started %d\n",ret);

  l4thread_sleep(1000);
  LOG_flush();

  enter_kdebug("next test");

  /***************************************************************************
   *** Thread test
   ***************************************************************************/

  data_key = l4thread_data_allocate_key();
  printf("got key %d\n",data_key);

  ret = l4thread_create(test_fn,(void *)dat,L4THREAD_CREATE_SYNC);

  printf("started %d\n",ret);

  ret = l4thread_create_long(L4THREAD_INVALID_ID,test_fn,
  			     0,
			     L4THREAD_INVALID_SP,4 * 1024 * 1024,
			     255,(void *)dat,
			     L4THREAD_CREATE_SYNC);

  printf("started %d\n",ret);

#if 1
  l4thread_sleep(1000);
  LOG_flush();
  enter_kdebug("-");
#endif

  l4thread_shutdown(ret);

  l4thread_sleep(2000);

  enter_kdebug("up.");

  return 0;
}
