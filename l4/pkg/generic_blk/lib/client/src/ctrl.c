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
#include <l4/util/macros.h>

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
  sm_exc_t exc;
  l4_strdope_t strdope;

  /* get driver descriptor */
  drv = blkclient_get_driver(driver);
  if (drv == NULL)
    {
      Error("Invalid driver handle (%d)",driver);
      return -L4_EINVAL;
    }

  /* set input/output buffer */
  strdope = sndrcv_refstring(in_size,in,out_size,out);
  if (in == NULL)
    strdope.snd_size = 0;
  if (out == NULL)
    strdope.rcv_size = 0;

  /* call driver */
  ret = l4blk_cmd_ctrl(drv->cmd_id,drv->handle,cmd,&strdope,&exc);
  if (exc._type != exc_l4_no_exception)
    {
      Error("IPC error calling driver: 0x%02x",exc._type);
      return -L4_EIPC;
    }

#if 0
  LOGI("out at 0x%08x, rcv_str at 0x%08x",(unsigned)out,strdope.rcv_str);
  enter_kdebug("-");
#endif

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
