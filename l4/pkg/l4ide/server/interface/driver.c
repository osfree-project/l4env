/*****************************************************************************/
/**
 * \file   l4ide/src/interface/driver.c
 * \brief  L4IDE Driver Interface
 *
 * \date   01/27/2004
 * \author Marek Menzer <mm19@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4/DROPS includes */
#include <l4/env/errno.h>
#include <l4/sys/types.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>
#include <l4/thread/thread.h>
#include <l4/lock/lock.h>
#include <l4/semaphore/semaphore.h>
#include <l4/generic_blk/blk-server.h>

/* OSKit includes */
#include <stdlib.h>

/* Driver Framework includes */
#include <driver/config.h>
#include <driver/driver.h>
#include <driver/command.h>
#include <driver/notification.h>
#include <driver/types.h>


/*****************************************************************************
 *** Global stuff
 *****************************************************************************/

/* driver instance descriptors */
static l4ide_driver_t drivers[CONFIG_L4IDE_MAX_CLIENTS];

/*****************************************************************************/
/**
 * \brief Return driver instance descriptor
 * 
 * \param  driver        Driver handle
 *	
 * \return driver descriptor, NULL if invalid driver handle
 */
/*****************************************************************************/ 
l4ide_driver_t *
l4ide_get_driver(l4blk_driver_id_t driver)
{
  /* check driver handle */
  if ((driver < 0) || (driver >= CONFIG_L4IDE_MAX_CLIENTS) ||
      (drivers[driver].handle == -1))
    return NULL;

  /* return descriptor */
  return &drivers[driver];
}

/*****************************************************************************
 *** Server RPC functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Open new driver instance.
 * 
 * \param  _dice_corba_obj    Request source
 * \param  _dice_corba_env    Server environment
 * \retval driver             Driver handle
 * \retval cmd_id             Command thread id
 * \retval notify_id          Notify thread id
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_ENOHANDLE  no driver handle available
 *         - -#L4_ENOTHREAD  command / notification thread creation failed
 *
 * Create a new instance of the driver.
 */
/*****************************************************************************/ 
l4_int32_t 
l4blk_driver_open_component(CORBA_Object _dice_corba_obj,
                            l4blk_driver_id_t * driver,
                            l4_threadid_t * cmd_id,
                            l4_threadid_t * notify_id,
                            CORBA_Server_Environment * _dice_corba_env)
{
  int i,ret;

  /* find unused driver descriptor */
  i = 0;
  while (i < CONFIG_L4IDE_MAX_CLIENTS)
    {
      if (drivers[i].handle == -1)
        break;
      i++;
    }
  
  if (i == CONFIG_L4IDE_MAX_CLIENTS)
    {
      LOG_Error("no driver instance descriptor available!");
      return -L4_ENOHANDLE;
    }

  /* setup driver instance */
  drivers[i].handle = i;
  drivers[i].client = *_dice_corba_obj;
  drivers[i].wait = L4SEMAPHORE_LOCKED;
  drivers[i].pending = NULL;
  drivers[i].p_lock = L4LOCK_UNLOCKED;

  /* start command thread */
  ret = l4ide_start_command_thread(&drivers[i]);
  if (ret < 0)
    {
      LOG_Error("create command thread failed (%d)", ret);
      drivers[i].handle = -1;
      return -L4_ENOTHREAD;
    }

  /* start notification thread */
  ret = l4ide_start_notification_thread(&drivers[i]);
  if (ret < 0)
    {
      LOG_Error("create command thread failed (%d)", ret);
      l4ide_shutdown_command_thread(&drivers[i]);
      drivers[i].handle = -1;
      return -L4_ENOTHREAD;
    }

  /* setup reply */
  *driver = i;
  *cmd_id = l4thread_l4_id(drivers[i].cmd_th);
  *notify_id = l4thread_l4_id(drivers[i].notify_th);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Close driver instance.
 * 
 * \param  _dice_corba_obj    Request source
 * \param  driver             Driver handle
 * \param  _dice_corba_env    Server environment
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_EINVAL  invalid driver handle
 *         - -#L4_EPERM   client not owns the driver instance
 *
 */
/*****************************************************************************/ 
l4_int32_t 
l4blk_driver_close_component(CORBA_Object _dice_corba_obj,
                             l4blk_driver_id_t driver,
                             CORBA_Server_Environment * _dice_corba_env)
{
  /* check driver handle */
  if ((driver < 0) || (driver >= CONFIG_L4IDE_MAX_CLIENTS) || 
      (drivers[driver].handle == -1))
    return -L4_EINVAL;

  if (!l4_task_equal(*_dice_corba_obj, drivers[driver].client))
    return -L4_EPERM;

  /* close driver instance */
  l4ide_shutdown_command_thread(&drivers[driver]);
  l4ide_shutdown_notification_thread(&drivers[driver]);
  drivers[driver].handle = -1;

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Flick Driver Interface server loop
 *
 * Implementation of the DROPS Block Device Driver Interface driver 
 * interface. See generik_blk package.
 */
/*****************************************************************************/ 
void
l4ide_driver_service_loop(void)
{
  l4thread_set_prio(l4thread_myself(), CONFIG_L4IDE_WORK_PRIO);

  /* start driver service loop */
  l4blk_driver_server_loop(NULL);
  
  /* this should never happen */
  Panic("left driver loop");
}

/*****************************************************************************/
/**
 * \brief Init driver instance handling.
 */
/*****************************************************************************/ 
void
l4ide_init_driver_instances(void)
{
  int i;

  /* mark all driver descriptors unused */
  for (i = 0; i < CONFIG_L4IDE_MAX_CLIENTS; i++)
    drivers[i].handle = -1;
}
