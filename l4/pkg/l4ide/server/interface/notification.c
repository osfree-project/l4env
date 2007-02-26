/*****************************************************************************/
/**
 * \file   l4ide/src/interface/notisfication.c
 * \brief  L4IDE driver, notification interface
 *
 * \date   01/27/2004
 * \author Marek Menzer <mm19@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4/DROPS includes */
#include <l4/env/errno.h>
#include <l4/sys/types.h>
#include <l4/util/atomic.h>
#include <l4/util/macros.h>
#include <l4/log/l4log.h>
#include <l4/lock/lock.h>
#include <l4/thread/thread.h>
#include <l4/generic_blk/blk-server.h>
#include <l4/dde_linux/dde.h>

/* private includes */
#include <driver/config.h>
#include <driver/notification.h>
#include <driver/driver.h>
#include <driver/types.h>

/*****************************************************************************
 *** Global data
 *****************************************************************************/

static l4ide_notification_t notification_descs[CONFIG_L4IDE_NUM_REQUESTS];
static int alloc_next_desc = 0;

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
  /* we need a task_struct for Linux sleeping */
  l4dde_process_add_worker();

  /* start server loop */
  l4blk_notify_server_loop(NULL);
}

/*****************************************************************************/
/**
 * \brief  Allocate notification descriptor
 * 
 * \return notification descriptor, NULL if allocation failed
 */
/*****************************************************************************/ 
static inline l4ide_notification_t *
__allocate_notification_desc(void)
{
  int i = alloc_next_desc;
  int found = 0;
  
  do {
    if (l4util_cmpxchg32(&notification_descs[i].status, -1, -2)) {
      found = 1;
      break;
    }
    i = (i + 1) % CONFIG_L4IDE_NUM_REQUESTS;
  } while (i != alloc_next_desc);
  
  if (!found) {
    LOG_Error("no notification descriptor available");
    return NULL;
  }
  
  alloc_next_desc = (i + 1) % CONFIG_L4IDE_NUM_REQUESTS;
  
  return &notification_descs[i];
}

/*****************************************************************************/
/**
 * \brief  Release notification descriptor
 * 
 * \param  notification		notification descriptor
 */
/*****************************************************************************/ 
static inline void
__release_notification_desc(l4ide_notification_t * notification)
{
  notification->status = -1;
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
                            CORBA_Server_Environment * _dice_corba_env)
{
  l4ide_driver_t * driver;
  l4ide_notification_t * notification;

  driver = l4ide_get_driver(drv);
  if (driver == NULL)
    return -L4_EINVAL;

  if (!l4_task_equal(*_dice_corba_obj, driver->client))
    return -L4_EPERM;

  /* wait for notification */
  do
    {
      l4semaphore_down(&driver->wait);

      /* check notification */
      l4lock_lock(&driver->p_lock);

      notification = driver->pending;
      if (notification != NULL)
        driver->pending = notification->next;

      l4lock_unlock(&driver->p_lock);

      if (notification == NULL)
        LOGL("what's that: woke up but no notification pending!?");
    }
  while (notification == NULL);

//  LOGdL(CONFIG_L4IDE_DEBUG_NOTIFY, "\n client "l4util_idfmt", request %u, status %d, error %d",
//       l4util_idstr(*_dice_corba_obj), notification->handle, notification->status, notification->error)

  /* setup reply */
  *req_handle = notification->handle;
  *status = notification->status;
  *error = notification->error;

  __release_notification_desc(notification);

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
l4ide_do_notification(l4ide_driver_t * driver, l4_uint32_t req_handle,
                      l4_uint32_t status, l4_uint32_t error)
{
  l4ide_notification_t * notification = __allocate_notification_desc();
  l4ide_notification_t * np;

  /* enqueue new notification */
  if (notification == NULL)
    return -L4_ENOMEM;

  notification->handle = req_handle;
  notification->status = status;
  notification->error = error;
  notification->next = NULL;

  l4lock_lock(&driver->p_lock);

  if (driver->pending == NULL)
    driver->pending = notification;
  else
    {
      np = driver->pending;
      while (np->next != NULL)
        np = np->next;
      np->next = notification;
    }
  
  l4lock_unlock(&driver->p_lock);

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
l4ide_start_notification_thread(l4ide_driver_t * driver)
{
  l4thread_t t;

  /* start thread */
  t = l4thread_create_long(CONFIG_L4IDE_THREAD_BASE + 2*(driver->handle) + 1,
                           __notification_thread, "L4IDE_NFY", L4THREAD_INVALID_SP,
			   CONFIG_L4IDE_NOTIFY_STACK_SIZE,
			   CONFIG_L4IDE_NOTIFY_PRIO,
			   driver, L4THREAD_CREATE_ASYNC);

  if (t < 0)
    {
      driver->notify_th = L4THREAD_INVALID_ID;
      return t;
    }
  else
    {
      driver->notify_th = t;
      return 0;
    }
}

/*****************************************************************************/
/**
 * \brief  Shutdown notification thread
 * 
 * \param  driver        Driver descriptor
 */
/*****************************************************************************/ 
void
l4ide_shutdown_notification_thread(l4ide_driver_t * driver)
{
  /* shutdown thread */
  l4thread_shutdown(driver->notify_th);  
}

/*****************************************************************************/
/**
 * \brief  Setup notification handling
 */
/*****************************************************************************/ 
void
l4ide_init_notifications(void)
{
  int i;
  
  for (i = 0; i < CONFIG_L4IDE_NUM_REQUESTS; i++)
    notification_descs[i].status = -1;
}
