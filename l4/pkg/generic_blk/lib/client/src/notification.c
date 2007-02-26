/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_blk/lib/client/src/notification.c
 * \brief  Processed notification thread.
 *
 * \date   02/11/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Implementation of the processed notification thread.
 */
/*****************************************************************************/

/* L4/DROPS includes */
#include <l4/util/macros.h>
#include <l4/thread/thread.h>

/* library includes */
#include <l4/generic_blk/blk.h>
#include <l4/generic_blk/blk-client.h>
#include "__libblk.h"
#include "__driver.h"
#include "__request.h"
#include "__config.h"

/*****************************************************************************/
/**
 * \brief Processed notification thread
 * 
 * \param  data          Thread data, driver handle
 */
/*****************************************************************************/ 
static void
__notification_thread(void * data)
{
  blkclient_driver_t * drv = (blkclient_driver_t *)data;
  l4_uint32_t handle,status;
  int error;
  sm_exc_t exc;

  /* notification loop */
  while (1)
    {
      l4blk_notify_wait(drv->notify_id,drv->handle,&handle,&status,&error,&exc);
      if (exc._type != exc_l4_no_exception)
	Error("IPC error calling driver (%d)\n",exc._type);
      else
	/* set request status */
	blkclient_set_request_status(handle,status,error);
    }

  /* this should never happen */
  Panic("left notification thread...\n");
}

/*****************************************************************************/
/**
 * \brief Start notification thread
 *
 * \param  driver        Driver descriptor
 *	
 * \return 0 on success (started notification thread), -1 if thread creation
 *         failed.          
 */
/*****************************************************************************/ 
int
blkclient_start_notification_thread(blkclient_driver_t * driver)
{
  /* start thread */
  driver->notify_thread = 
    l4thread_create_long(L4THREAD_INVALID_ID,__notification_thread,
			 L4THREAD_INVALID_SP,BLKCLIENT_NOTIFY_STACK_SIZE,
			 L4THREAD_DEFAULT_PRIO,driver,L4THREAD_CREATE_ASYNC);
  if (driver->notify_thread < 0)
    {
      /* creation failed */
      driver->notify_thread = L4THREAD_INVALID_ID;
      return -1;
    }
  else
    return 0;				
}

/*****************************************************************************/
/**
 * \brief Shutdown notification thread
 * 
 * \param  driver        Driver descriptor 
 */
/*****************************************************************************/ 
void
blkclient_shutdown_notification_thread(blkclient_driver_t * driver)
{
  /* shutdown thread */
  l4thread_shutdown(driver->notify_thread);
}
