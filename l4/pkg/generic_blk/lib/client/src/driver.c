/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_blk/lib/client/src/driver.c
 * \brief  Driver handling
 *
 * \date   02/11/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4 includes */
#include <l4/util/atomic.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>
#include <l4/names/libnames.h>

/* library includes */
#include <l4/generic_blk/blk.h>
#include <l4/generic_blk/blk-client.h>
#include "__driver.h"
#include "__libblk.h"
#include "__config.h"
#include "__debug.h"

/*****************************************************************************
 *** driver descriptors
 *****************************************************************************/

static blkclient_driver_t drivers[BLKCLIENT_MAX_DRIVERS];

/*****************************************************************************
 *** internal stuff
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Init driver handling
 */
/*****************************************************************************/ 
void
blkclient_init_drivers(void)
{
  int i;

  for (i = 0; i < BLKCLIENT_MAX_DRIVERS; i++)
    drivers[i].notify_thread = -1;
}

/*****************************************************************************/
/**
 * \brief Return driver descriptor
 * 
 * \param  driver        Driver handle
 *	
 * \return driver descriptor, NULL if invalid driver handle.
 */
/*****************************************************************************/ 
blkclient_driver_t * 
blkclient_get_driver(l4blk_driver_t driver)
{
  /* check driver handle */
  if ((driver < 0) || (driver >= BLKCLIENT_MAX_DRIVERS) ||
      (drivers[driver].notify_thread == -1))
    return NULL;

  /* return command thread id */
  return &drivers[driver];
}

/*****************************************************************************
 *** API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Open block device driver.
 * 
 * \param  name          Driver name
 * \retval driver        Driver handle
 * \param  cb            Notification thread setup callback function, 
 *                       if set it will be called by created notification 
 *                       thread before it enters its work loop
 *	
 * \return 0 on success (\a driver contains a valid handle), error code 
 *         otherwise:
 *         - -#L4_EINVAL     invalid name
 *         - -#L4_ENOTFOUND  driver not found
 *         - -#L4_ENOMEM     out of memory allocating driver descriptor
 *         - -#L4_EIPC       error calling driver
 *         - -#L4_ENOTHREAD  creation of notification thread failed
 *
 * \a l4blk_open_driver does two things:
 * - request the id of the driver at the DROPS nameserver using \a name
 * - call the driver to open new instances of the command and wait threads
 */
/*****************************************************************************/ 
int
l4blk_open_driver(const char * name, l4blk_driver_t * driver, 
                  l4blk_setup_notify_callback_fn_t cb)
{
  int d,ret;
  blkclient_driver_t * drv;
  CORBA_Environment _env = dice_default_environment;

  /* return invalid driver handle on error */
  *driver = -1;

  /* find driver descriptor */
  for (d = 0; d < BLKCLIENT_MAX_DRIVERS; d++)
    {
      if (l4util_cmpxchg32(&drivers[d].notify_thread, -1, 0))
	/* found */
	break;
    }

  LOGdL(DEBUG_DRIVER_OPEN, "using driver descriptor %d", d);

  if (d == BLKCLIENT_MAX_DRIVERS)
    return -L4_ENOMEM;

  drv = &drivers[d];

  /* request driver id at nameserver */
  if (!names_waitfor_name(name, &drv->driver_id, BLKCLIENT_NAMES_WAIT))
    {
      LOG_Error("block driver \"%s\" not found!",name);
      return -L4_ENOTFOUND;
    }

  /* open driver */
  ret = l4blk_driver_open_call(&drv->driver_id, &drv->handle,
                               &drv->cmd_id, &drv->notify_id, &_env);
  if (ret || DICE_HAS_EXCEPTION(&_env))
    {
      LOG_Error("open driver failed \"%s\" (ret %d, exc %d)",
                name, ret, DICE_EXCEPTION_MAJOR(&_env));
      if (ret)
	return ret;
      else
	return -L4_EIPC;
    }

  LOGdL(DEBUG_DRIVER_OPEN, "opened driver %d, cmd "l4util_idfmt \
        ", notify "l4util_idfmt, drv->handle, l4util_idstr(drv->cmd_id), 
        l4util_idstr(drv->notify_id));

  /* create notification thread */
  drv->cb = cb;
  ret = blkclient_start_notification_thread(drv);
  if (ret)
    {
      LOG_Error("create notification thread failed!");
      l4blk_driver_close_call(&drv->driver_id, drv->handle, &_env);
      return -L4_ENOTHREAD;
    }

  LOGdL(DEBUG_DRIVER_OPEN, "notification wait thread "l4util_idfmt,
        l4util_idstr(l4thread_l4_id(drv->notify_thread)));

  *driver = d;

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Close block device driver.
 * 
 * \param  driver        Driver handle.
 *	
 * \return 0 on success (close driver instance), error code otherwise:
 *         - -#L4_EINVAL  invalid driver handle
 *         - -#L4_EIPC    error calling driver
 */
/*****************************************************************************/ 
int 
l4blk_close_driver(l4blk_driver_t driver)
{
  blkclient_driver_t * drv;
  int ret;
  CORBA_Environment _env = dice_default_environment;  
  
  /* check driver handle */
  if ((driver < 0) || (driver >= BLKCLIENT_MAX_DRIVERS))
    {
      LOG_Error("invalid driver handle %d", driver);
      return -L4_EINVAL;

    }

  if (drivers[driver].notify_thread == -1)
    {
      LOG_Error("unused driver handle %d", driver);
      return -L4_EINVAL;
    }

  drv = &drivers[driver];

  LOGdL(DEBUG_DRIVER_CLOSE, 
        "closing driver %d, notification thread %d ("l4util_idfmt")", 
        driver, drv->notify_thread, 
        l4util_idstr(l4thread_l4_id(drv->notify_thread)));

  /* shutdown notification thread */
  blkclient_shutdown_notification_thread(drv);
  drv->notify_thread = -1;

  /* close driver */
  ret = l4blk_driver_close_call(&drv->driver_id, drv->handle, &_env);
  if (ret || DICE_HAS_EXCEPTION(&_env))
    {
      LOG_Error("close driver failed (ret %d, exc %d)", ret,
	  DICE_EXCEPTION_MAJOR(&_env));
      if (ret)
	return ret;
      else
	return -L4_EIPC;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Return thread id of command thread
 * 
 * \param  driver        Driver handle
 *	
 * \return thread id of command thread, L4_INVALID_ID if invalid driver handle
 */
/*****************************************************************************/ 
l4_threadid_t 
l4blk_get_driver_thread(l4blk_driver_t driver)
{
  /* check driver handle */
  if ((driver < 0) || (driver >= BLKCLIENT_MAX_DRIVERS) ||
      (drivers[driver].notify_thread == -1))
    return L4_INVALID_ID;

  /* return command thread id */
  return drivers[driver].cmd_id;
}
