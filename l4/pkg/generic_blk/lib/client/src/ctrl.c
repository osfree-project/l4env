/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_blk/lib/client/src/ctrl.c
 * \brief  Generic driver ctrl.
 *
 * \date   03/07/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4/DROPS includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>

/* Library includes */
#include <l4/generic_blk/blk.h>
#include <l4/generic_blk/blk-client.h>
#include "__driver.h"
#include "__config.h"

/*****************************************************************************
 *** API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Generic driver control.
 * 
 * \param  driver        Driver handle.
 * \param  cmd           Control command.
 * \param  in            Input buffer
 * \param  in_size       Size of input buffer
 * \param  out           Output buffer
 * \param  out_size      Size of output buffer
 *	
 * \return result of ctrl-call to driver, -#L4_EIPC if call failed.
 *
 * This function is the 'swiss army knife' to manipulate various driver
 * parameters. The possible commands depend on the used driver.
 */
/*****************************************************************************/ 
int
l4blk_ctrl(l4blk_driver_t driver, 
           l4_uint32_t cmd, 
           void * in, 
           int in_size, 
           void * out, 
           int out_size)
{
  blkclient_driver_t * drv;
  int ret;
  CORBA_Environment _env = dice_default_environment;

  /* get driver descriptor */
  drv = blkclient_get_driver(driver);
  if (drv == NULL)
    {
      LOG_Error("invalid driver handle (%d)", driver);
      return -L4_EINVAL;
    }

  /* call driver */
  if (in == NULL)
    in_size = 0;
  if (out == NULL)
    out_size = 0;
  ret = l4blk_cmd_ctrl_call(&drv->cmd_id, drv->handle, cmd, in, in_size, 
                            &out, &out_size, &_env);
  if (DICE_HAS_EXCEPTION(&_env))
    {
      LOG_Error("IPC error calling driver: 0x%02x", DICE_EXCEPTION_MAJOR(&_env));
      return -L4_EIPC;
    }

  /* done */
  return ret;
}
 
/*****************************************************************************
 *** some of the predefined ctrls
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Return number of disks
 * 
 * \param  driver
 *	
 * \return Number of disks connected to the driver, error code (< 0) if failed
 */
/*****************************************************************************/ 
int
l4blk_ctrl_get_num_disks(l4blk_driver_t driver)
{
  return l4blk_ctrl(driver,L4BLK_CTRL_NUM_DISKS,NULL,0,NULL,0);
}

/*****************************************************************************/
/**
 * \brief  Return disk size
 * 
 * \param  driver        Driver handle
 * \param  device        Device id
 *	
 * \return Disk size in blocks (1KB), error code (< 0) if failed
 */
/*****************************************************************************/ 
int
l4blk_ctrl_get_disk_size(l4blk_driver_t driver, l4_uint32_t dev)
{  
  return l4blk_ctrl(driver,L4BLK_CTRL_DISK_SIZE,&dev,sizeof(dev),NULL,0);
}

/*****************************************************************************/
/**
 * \brief   Return period for stream requests
 * \ingroup api_ctrl
 * 
 * \param   driver       Driver handle
 * \param   dev          Device id
 * \retval  period_len   Period length (microseconds)
 * \retval  period_offs  Period offset 
 *                       (relative to kernel klock, i.e. period0 % period_len)
 *	
 * \return 0 on success, error code if failed
 */
/*****************************************************************************/ 
int
l4blk_ctrl_get_stream_period(l4blk_driver_t driver, l4_uint32_t dev, 
                             l4_uint32_t * period_len, 
                             l4_uint32_t * period_offs)
{
  l4blk_disk_period_t args;
  int ret;
  
  ret = l4blk_ctrl(driver, L4BLK_CTRL_STREAM_PERIOD, &dev, sizeof(dev),
                   &args, sizeof(l4blk_disk_period_t));
  if (ret < 0)
    return ret;

  *period_len = args.period_len;
  *period_offs = args.period_offs;

  return 0;
}
