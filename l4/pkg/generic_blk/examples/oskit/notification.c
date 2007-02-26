/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_blk/examples/oskit/notisfication.c
 * \brief  OSKit block driver, notification interface
 *
 * \date   09/13/2003
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* standard includes */
#include <stdlib.h>

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/thread/thread.h>
#include <l4/lock/lock.h>
#include <l4/semaphore/semaphore.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>

/* generic_blk includes */
#include <l4/generic_blk/blk-server.h>

/* private includes */
#include "blksrv.h"
#include "debug.h"

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Notification server thread
 * 
 * \param  data          Thread data, pointer to driver descriptor
 */
/*****************************************************************************/ 
static void 
__notification_thread(void * data)
{
  CORBA_Environment env = dice_default_server_environment;

  /* the driver descriptor is passed to the component function using the
   * user_data pointer in the sever environment */
  env.user_data = data;

  /* start server loop */
  l4blk_notify_server_loop(&env);
}

/*****************************************************************************
 *** server interface functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Wait for client notification
 * 
 * \param  _dice_corba_obj    Request source
 * \param  drv                Driver handle
 * \retval req_handle         User request handle
 * \retval status             Request status
 * \retval error              Error code
 * \param  _dice_corba_env    Server environment
 *	
 * \return 0 on success, error code otherwise
 */
/*****************************************************************************/ 
l4_int32_t 
l4blk_notify_wait_component(CORBA_Object _dice_corba_obj,
                            l4blk_driver_id_t drv,
                            l4_uint32_t * req_handle,
                            l4_uint32_t * status,
                            l4_int32_t * error,
                            CORBA_Environment * _dice_corba_env)
{
  blksrv_driver_t * driver = (blksrv_driver_t *)_dice_corba_env->user_data;
  blksrv_notification_t * notification = NULL;

  LOGdL(DEBUG_NOTIFY, "wait from "IdFmt", driver %d", 
        IdStr(*_dice_corba_obj), drv);

  if ((driver == NULL) || (driver->driver != drv))
    return -L4_EINVAL;

  /* wait for notification */
  do
    {
      l4semaphore_down(&driver->wait);

      /* check notification */
      l4lock_lock(&driver->lock);

      notification = driver->notifications;
      if (notification != NULL)
        driver->notifications = notification->next;

      l4lock_unlock(&driver->lock);

      if (notification == NULL)
        LOGL("what's that: woke up but no notification pending!?");
    }
  while (notification == NULL);

  /* setup reply */
  *req_handle = notification->handle;
  *status = notification->status;
  *error = notification->error;

  LOGdL(DEBUG_NOTIFY, "request %d, status %x, error %d", notification->handle,
        notification->status, notification->error);

  free(notification);

  /* done */
  return 0;
}

/*****************************************************************************
 *** internal functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Enqueue notification
 * 
 * \param  driver        Driver descriptor
 * \param  req_handle    User request handle
 * \param  status        Request status
 * \param  error         Error code
 *	
 * \return 0 on success, error code otherwise
 */
/*****************************************************************************/ 
int
blksrv_do_notification(blksrv_driver_t * driver, l4_uint32_t req_handle,
                       l4_uint32_t status, l4_int32_t error)
{
  blksrv_notification_t * notification;
  blksrv_notification_t * np;

  /* enqueue new notification */
  notification = malloc(sizeof(blksrv_notification_t));
  if (notification == NULL)
    return -L4_ENOMEM;

  notification->handle = req_handle;
  notification->status = status;
  notification->error = error;
  notification->next = NULL;

  l4lock_lock(&driver->lock);

  if (driver->notifications == NULL)
    driver->notifications = notification;
  else
    {
      np = driver->notifications;
      while (np->next != NULL)
        np = np->next;
      np->next = notification;
    }
  
  l4lock_unlock(&driver->lock);

  /* wakeup notification thread */
  l4semaphore_up(&driver->wait);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Create notifiaction thread 
 * 
 * \param  driver        Driver descriptor
 *	
 * \return 0 on success, error code otherwise
 */
/*****************************************************************************/ 
int 
blksrv_start_notification_thread(blksrv_driver_t * driver)
{
  /* start thread */
  driver->notify_th = l4thread_create(__notification_thread, driver, 
                                      L4THREAD_CREATE_ASYNC);
  if (driver->notify_th < 0)
    return driver->notify_th;
  else
    return 0;
}

/*****************************************************************************/
/**
 * \brief  Shutdown notification thread
 * 
 * \param  driver        Driver descriptor
 */
/*****************************************************************************/ 
void
blksrv_shutdown_notification_thread(blksrv_driver_t * driver)
{
  /* shutdown thread */
  l4thread_shutdown(driver->notify_th);  
}

