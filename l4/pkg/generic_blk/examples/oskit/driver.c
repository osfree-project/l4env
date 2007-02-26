/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_blk/examples/oskit/driver.c
 * \brief  OSKit block driver, driver-interface implementation
 *
 * \date   09/13/2003
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>

/* generic_blk includes */
#include <l4/generic_blk/blk.h>
#include <l4/generic_blk/blk-server.h>

/* private includes */
#include "types.h"
#include "blksrv.h"
#include "config.h"

/*****************************************************************************
 *** globals
 *****************************************************************************/

/* driver instances */
static blksrv_driver_t drivers[BLKSRV_MAX_CLIENTS];

/* command-interface thread */
extern l4_threadid_t blksrv_cmd_id;

/*****************************************************************************
 *** server interface functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Return command / notification thread id
 *  
 * \param  _dice_corba_obj    Request source
 * \retval drv                Driver handle
 * \retval cmd_id             Command thread id
 * \retval notify_id          Notification thread id
 * \param  _dice_corba_env    Server environment
 *	
 * \return 0 on success, error code otherwise
 */
/*****************************************************************************/ 
l4_int32_t 
l4blk_driver_open_component(CORBA_Object _dice_corba_obj,
                            l4blk_driver_id_t * drv,
                            l4_threadid_t * cmd_id,
                            l4_threadid_t * notify_id,
                            CORBA_Environment * _dice_corba_env)
{
  int i,ret;

  /* allocate new driver instance */
  i = 0;
  while ((i < BLKSRV_MAX_CLIENTS) && 
         (drivers[i].driver != L4BLK_INVALID_DRIVER))
    i++;
  if (i == BLKSRV_MAX_CLIENTS)
    return -L4_ENOHANDLE;

  /* setup driver instance */
  drivers[i].driver = i;
  drivers[i].wait = L4SEMAPHORE_INIT(0);
  drivers[i].lock = L4LOCK_UNLOCKED;
  drivers[i].notifications = NULL;

  /* start notification thread, we need a separate thread for each client 
   * because with Dice we cannot have several client threads blocking on 
   * the same server thread */
  ret = blksrv_start_notification_thread(&drivers[i]);
  if (ret < 0)
    {
      drivers[i].driver = L4BLK_INVALID_DRIVER;
      return ret;
    }
  *notify_id = l4thread_l4_id(drivers[i].notify_th);

  /* in this example we do not use a separate command thread for each client, 
   * just return the id of the default thread */
  *cmd_id = blksrv_cmd_id;

  /* return driver id */
  *drv = i;

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Close driver instance
 * 
 * \param  _dice_corba_obj    Request source
 * \param  drv                Driver handle
 * \param  _dice_corba_env    Server environment
 *	
 * \return 0 on success, error code otherwise
 */
/*****************************************************************************/ 
l4_int32_t 
l4blk_driver_close_component(CORBA_Object _dice_corba_obj,
                             l4blk_driver_id_t drv,
                             CORBA_Environment * _dice_corba_env)
{
  if ((drv < 0) || (drv >= BLKSRV_MAX_CLIENTS) ||
      (drivers[drv].driver == L4BLK_INVALID_DRIVER))
    return -L4_EINVAL;

  /* shutdown notification thread */
  blksrv_shutdown_notification_thread(&drivers[drv]);

  /* nothing else to do */
  drivers[drv].driver = L4BLK_INVALID_DRIVER;

  return 0;
}

/*****************************************************************************
 *** internal functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Start driver-interface server loop
 */
/*****************************************************************************/ 
void
blksrv_start_driver(void)
{
  int i;

  /* setup driver instances */
  for (i = 0; i < BLKSRV_MAX_CLIENTS; i++)
    drivers[i].driver = L4BLK_INVALID_DRIVER;

  /* start driver server loop */
  l4blk_driver_server_loop(NULL);
}

/*****************************************************************************/
/**
 * \brief  Return driver descriptor
 * 
 * \param  drv           Driver id
 *	
 * \return Pointer to driver descriptor, NULL if invalid descriptor
 */
/*****************************************************************************/ 
blksrv_driver_t *
blksrv_get_driver(l4blk_driver_id_t drv)
{
  if ((drv < 0) || (drv >= BLKSRV_MAX_CLIENTS) ||
      (drivers[drv].driver == L4BLK_INVALID_DRIVER))
    return NULL;

  return &drivers[drv];
}
