/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_blk/examples/oskit/command.c
 * \brief  OSKit block driver, cmd-interface implementation
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
#include <l4/log/l4log.h>

/* generic_blk includes */
#include <l4/generic_blk/blk.h>
#include <l4/generic_blk/blk-server.h>
#include "blksrv.h"
#include "config.h"

/*****************************************************************************
 *** globals
 *****************************************************************************/

/** command thread id */
l4_threadid_t blksrv_cmd_id = L4_INVALID_ID;

/*****************************************************************************/
/**
 * \brief  Command server thread
 * 
 * \param  data          Thread data
 */
/*****************************************************************************/ 
static void
__command_thread(void * data)
{
  CORBA_Environment env = dice_default_server_environment;

  /* setup server environment */
  env.malloc = (dice_malloc_func)malloc;
  env.free = (dice_free_func)free;
  
  LOGL("command interface thread started.");

  /* start server loop */
  l4blk_cmd_server_loop(&env);  
}

/*****************************************************************************
 *** server interface functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Create real-time strean, not supported
 */
/*****************************************************************************/ 
l4_int32_t 
l4blk_cmd_create_stream_component(CORBA_Object _dice_corba_obj,
                                  l4blk_driver_id_t drv,
                                  l4_uint32_t bandwidth,
                                  l4_uint32_t blk_size,
                                  float q,
                                  l4_uint32_t meta_int,
                                  l4blk_stream_t * stream,
                                  CORBA_Environment * _dice_corba_env)
{
  /* not supported */
  return -L4_ENOTSUPP;
}

/*****************************************************************************/
/**
 * \brief  Close real-time strean, not supported
 */
/*****************************************************************************/ 
l4_int32_t 
l4blk_cmd_close_stream_component(CORBA_Object _dice_corba_obj,
                                 l4blk_driver_id_t drv,
                                 l4blk_stream_t stream,
                                 CORBA_Environment * _dice_corba_env)
{
  /* not supported */
  return -L4_ENOTSUPP;
}

/*****************************************************************************/
/**
 * \brief  Start real-time strean, not supported
 */
/*****************************************************************************/ 
l4_int32_t 
l4blk_cmd_start_stream_component(CORBA_Object _dice_corba_obj,
                                 l4blk_driver_id_t drv,
                                 l4blk_stream_t stream,
                                 l4_uint32_t time,
                                 l4_uint32_t request_no,
                                 CORBA_Environment * _dice_corba_env)
{
  /* not supported */
  return -L4_ENOTSUPP;
}

/*****************************************************************************/
/**
 * \brief  Enqueue request
 * 
 * \param  _dice_corba_obj    Request source
 * \param  drv                Driver id
 * \param  request            Request descriptor 
 * \param  sg_list            Scatter-gather list
 * \param  sg_size            Size of scatter-gather list
 * \param  sg_num             Number of elements in scatter-gather list
 * \param  sg_type            Scatter-gather list type:
 *                            - #L4BLK_SG_PHYS
 *                            - #L4BLK_SG_DS
 * \param  _dice_corba_env    Server environment
 *	
 * \return 0 on success, error code otherwise
 */
/*****************************************************************************/ 
l4_int32_t
l4blk_cmd_put_request_component(CORBA_Object _dice_corba_obj,
                                l4blk_driver_id_t drv,
                                const l4blk_blk_request_t *request,
                                const void *sg_list,
                                l4_int32_t sg_size,
                                l4_int32_t sg_num,
                                l4_int32_t sg_type,
                                CORBA_Environment *_dice_corba_env)
{
  blksrv_driver_t * driver = blksrv_get_driver(drv);

  if (driver == NULL)
    return -L4_EINVAL;

  /* sanity checks */
  if ((sg_num == 0) || (sg_list == NULL))
    return -L4_EINVAL;

  if (sg_type == L4BLK_SG_PHYS)
    return -L4_ENOTSUPP;

  /* enqueue request */
  return blksrv_enqueue_request(driver, request, 
                                (l4blk_sg_ds_elem_t *)sg_list, sg_num);
}

/*****************************************************************************/
/**
 * \brief  Driver ctrl
 * 
 * \param  _dice_corba_obj    Request source
 * \param  drv                Driver id
 * \param  command            Ctrl command
 * \param  in_args            Input buffer
 * \param  in_size            Size of input buffer
 * \param  out_size           Max. size of output buffer
 * \retval out_args           Output buffer
 * \retval out_size           Size of output buffer
 * \param  _dice_corba_env    Server environment
 *	
 * \return 0 on success, error code otherwise
 */
/*****************************************************************************/ 
l4_int32_t 
l4blk_cmd_ctrl_component(CORBA_Object _dice_corba_obj,
                         l4blk_driver_id_t drv,
                         l4_uint32_t command,
                         const void * in_args,
                         l4_int32_t in_size,
                         void ** out_args,
                         l4_int32_t * out_size,
                         CORBA_Environment * _dice_corba_env)
{
  /* default return buffer */
  *out_size = 0;

  switch (command)
    {
    case L4BLK_CTRL_NUM_DISKS:
      /* return number of disks */
      return blksrv_dev_num();

    case L4BLK_CTRL_DISK_SIZE:
      /* return disk size */
      return blksrv_dev_size();

    default: 
      return -L4_EINVAL;
    }

  /* done */
  return 0;
}

/*****************************************************************************
 *** internal functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Start command server thread
 * 
 * \return 0 on success, error code if failed
 */
/*****************************************************************************/ 
int
blksrv_start_command_thread(void)
{
  l4thread_t t;

  /* start thread */
  t = l4thread_create_long(L4THREAD_INVALID_ID, __command_thread,
                           L4THREAD_INVALID_SP, BLKSRV_CMD_STACK_SIZE,
                           L4THREAD_DEFAULT_PRIO, NULL, L4THREAD_CREATE_ASYNC);
  if (t < 0)
    return t;

  /* done */
  blksrv_cmd_id = l4thread_l4_id(t);
  return 0;
}
