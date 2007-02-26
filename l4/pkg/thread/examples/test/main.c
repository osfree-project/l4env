/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/examples/test/main.c
 * \brief  Thread lib test app
 *
 * \date   09/11/2000
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

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/log/l4log.h>
#include <l4/thread/thread.h>
#include <l4/l4rm/l4rm.h>

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
  l4_addr_t esp,addr,ds_map_addr;
  l4dm_dataspace_t ds;
  l4_offs_t offs;
  l4_size_t ds_map_size;
  int ret;

  asm("movl   %%esp, %0\n\t" : "=r" (esp) : );
  LOGI("stack at 0x%08x\n",esp);

  ret = l4rm_lookup((void *)esp,&ds,&offs,&ds_map_addr,&ds_map_size);
  if(ret < 0)
    {
      printf("l4rm_lookup failed (%d)!\n",ret);
      enter_kdebug("PANIC");
    }
  printf("  ds %u at %x.%x, offset 0x%08x, mapped to 0x%08x-0x%08x\n",
	 ds.id,ds.manager.id.task,ds.manager.id.lthread,offs,
	 ds_map_addr,ds_map_addr + ds_map_size);

  ret = l4rm_attach(&ds,L4_PAGESIZE,0,L4DM_RO,(void **)&addr);
  if (ret < 0)
    {
      printf("l4rm_attach failed (%d)!\n",ret);
      enter_kdebug("PANIC");
    }
  printf("  attached to addr 0x%08x\n",addr);
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

  LOGI("id = %d, id1 = %d\n",id,*id1);

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

  LOG_init("th_test");

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
