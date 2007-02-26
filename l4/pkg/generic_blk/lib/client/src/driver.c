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
l4blk_open_driver(const char * name, 
		  l4blk_driver_t * driver)
{
  int d,ret;
  blkclient_driver_t * drv;
  sm_exc_t exc;

  /* return invalid driver handle on error */
  *driver = -1;

  /* find driver descriptor */
  for (d = 0; d < BLKCLIENT_MAX_DRIVERS; d++)
    {
      if (cmpxchg32(&drivers[d].notify_thread,-1,0))
	/* found */
	break;
    }

#if DEBUG_DRIVER_OPEN
  INFO("using driver descriptor %d\n",d);
#endif

  if (d == BLKCLIENT_MAX_DRIVERS)
    return -L4_ENOMEM;

  drv = &drivers[d];

  /* request driver id at nameserver */
  if (!names_waitfor_name(name,&drv->driver_id,BLKCLIENT_NAMES_WAIT))
    {
      Error("Block driver \"%s\" not found!\n",name);
      return -L4_ENOTFOUND;
    }

  /* open driver */
  ret = l4blk_driver_open(drv->driver_id,&drv->handle,
			  (l4blk_threadid_t *)&drv->cmd_id,
			  (l4blk_threadid_t *)&drv->notify_id,&exc);
  if (ret || (exc._type != exc_l4_no_exception))
    {
      Error("Open driver failed \"%s\" (ret %d, exc %d)\n",name,ret,exc._type);
      if (ret)
	return ret;
      else
	return -L4_EIPC;
    }

#if DEBUG_DRIVER_OPEN
  INFO("opened driver %d, cmd %x.%x, notify %x.%x\n",drv->handle,
       drv->cmd_id.id.task,drv->cmd_id.id.lthread,
       drv->notify_id.id.task,drv->notify_id.id.lthread);
#endif

  /* create notification thread */
  ret = blkclient_start_notification_thread(drv);
  if (ret)
    {
      Error("Create notification thread failed!\n");
      l4blk_driver_close(drv->driver_id,drv->handle,&exc);
      return -L4_ENOTHREAD;
    }

#if DEBUG_DRIVER_OPEN
  INFO("notification wait thread %x.%x\n",
       l4thread_l4_id(drv->notify_thread).id.task,
       l4thread_l4_id(drv->notify_thread).id.lthread);
#endif

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
  sm_exc_t exc;
  
  /* check driver handle */
  if ((driver < 0) || (driver >= BLKCLIENT_MAX_DRIVERS) ||
      (drivers[driver].notify_thread == -1))
    return -L4_EINVAL;
  drv = &drivers[driver];

  /* shutdown notification thread */
  blkclient_shutdown_notification_thread(drv);
  drv->notify_thread = -1;

  /* close driver */
  ret = l4blk_driver_close(drv->driver_id,drv->handle,&exc);
  if (ret || (exc._type != exc_l4_no_exception))
    {
      Error("Close driver failed ((ret %d, exc %d)\n",ret,exc._type);
      if (ret)
	return ret;
      else
	return -L4_EIPC;
    }

  /* done */
  return 0;
}
