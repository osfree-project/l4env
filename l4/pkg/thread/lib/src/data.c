/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/src/data.c
 * \brief  Thread data management
 *
 * \date   07/02/2001
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
#include <l4/util/bitops.h>
#include <l4/util/macros.h>
#include <l4/env/errno.h>

/* private includes */
#include <l4/thread/thread.h>
#include "__tcb.h"
#include "__config.h"

/*****************************************************************************
 *** global data
 *****************************************************************************/

/**
 * data key allocation bit field
 */
static l4_uint32_t l4th_data_keys = 0;

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Set thread data pointer.
 * 
 * \param  tcb           Thread control block
 * \param  key           Data key
 * \param  data          Data pointer
 *	
 * \return 0 on success, -#L4_EINVAL if invalid data key
 */
/*****************************************************************************/ 
static inline int
__set_data(l4th_tcb_t * tcb, int key, void * data)
{
  /* sanity checks */
  if ((key < 0) || (key >= L4THREAD_MAX_DATA_KEYS) || 
      (test_bit(&l4th_data_keys,key) == 0))
    return -L4_EINVAL;

  /* set data pointer */
  tcb->data[key] = data;

  return 0;
}

/*****************************************************************************/
/**
 * \brief  Get thread data pointer 
 * 
 * \param  tcb           Thread control block
 * \param  key           Data key
 *	
 * \return Data pointer, NULL if invalid data key
 */
/*****************************************************************************/ 
static inline void * 
__get_data(l4th_tcb_t * tcb, int key)
{
  /* sanity checks */
  if ((key < 0) || (key >= L4THREAD_MAX_DATA_KEYS) || 
      (test_bit(&l4th_data_keys,key) == 0))
    return NULL;
  
  /* return data pointer */
  return tcb->data[key];
}

/*****************************************************************************
 *** l4thread user API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Allocate new data key.
 * 
 * \return New data key, -#L4_ENOKEY if no key available.
 */
/*****************************************************************************/ 
int 
l4thread_data_allocate_key(void)
{
  int i;

  for (i = 0; i < L4THREAD_MAX_DATA_KEYS; i++)
    {
      if (bts(&l4th_data_keys,i) == 0)
	/* found unused key */
	break;
    }

  if (i == L4THREAD_MAX_DATA_KEYS)
    /* no unused key found */
    return -L4_ENOKEY;
  else
    return i;
}

/*****************************************************************************/
/**
 * \brief  Release data key.
 * 
 * \param  key           data key 
 */
/*****************************************************************************/ 
void 
l4thread_data_release_key(int key)
{
  if ((key < 0) || (key >= L4THREAD_MAX_DATA_KEYS))
    return;

  /* release key */
  clear_bit(&l4th_data_keys,key);
}

/*****************************************************************************/
/**
 * \brief  Set data pointer for current thread
 * 
 * \param  key           data key
 * \param  data          data pointer
 *	
 * \return 0 on success, -#L4_EINVAL if invalid or unused data key.
 */
/*****************************************************************************/ 
int 
l4thread_data_set_current(int key, void * data)
{
  l4th_tcb_t * tcb;

  /* get my tcb */
  tcb = l4th_tcb_get_current();
  if ((tcb == NULL) || (tcb->state != TCB_ACTIVE))
    {
      Error("l4thread: myself not found!");
      return -L4_EINVAL;
    }

  /* set data pointer */
  return __set_data(tcb,key,data);
}

/*****************************************************************************/
/**
 * \brief  Get data pointer for current thread
 * 
 * \param  key           data key
 *	
 * \return Data pointer, NULL if invalid or unused data key.
 */
/*****************************************************************************/ 
void *
l4thread_data_get_current(int key)
{
  l4th_tcb_t * tcb;

  /* get my tcb */
  tcb = l4th_tcb_get_current();
  if ((tcb == NULL) || (tcb->state != TCB_ACTIVE))
    {
      Error("l4thread: myself not found!");
      return NULL;
    }

  /* get data pointer */
  return __get_data(tcb,key);
}

/*****************************************************************************/
/**
 * \brief  Set data pointer
 * 
 * \paran  thread        thread id
 * \param  key           data key
 * \param  data          data pointer
 *	
 * \return 0 on success, -#L4_EINVAL if invalid or unused data key / thread.
 */
/*****************************************************************************/ 
int
l4thread_data_set(l4thread_t thread, int key, void * data)
{
  l4th_tcb_t * tcb;

  /* get tcb */
  tcb = l4th_tcb_get_active(thread);
  if (tcb == NULL)
    return -L4_EINVAL;

  /* set data pointer */
  return __set_data(tcb,key,data);  
}

/*****************************************************************************/
/**
 * \brief  Get data pointer
 * 
 * \param  thread        thread id
 * \param  key           data key
 *	
 * \return Data pointer, NULL if invalid or unused data key / thread.
 */
/*****************************************************************************/ 
void *
l4thread_data_get(l4thread_t thread, int key)
{
  l4th_tcb_t * tcb;

  /* get tcb */
  tcb = l4th_tcb_get_active(thread);
  if (tcb == NULL)
    return NULL;
  
  /* get data pointer */
  return __get_data(tcb,key);  
}
