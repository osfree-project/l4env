/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_blk/lib/client/include/__driver.h
 * \brief  Client lib driver handling.
 *
 * \date   02/11/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _GENERIC_BLK___DRIVER_H
#define _GENERIC_BLK___DRIVER_H

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/thread/thread.h>

/* library includes */
#include <l4/generic_blk/blk.h>
#include <l4/generic_blk/blk-client.h>

/**
 * driver descriptor
 */
typedef struct blkclient_driver
{
  /* driver handle */
  l4blk_driver_id_t handle;

  /* driver threads */
  l4_threadid_t     driver_id;
  l4_threadid_t     cmd_id;
  l4_threadid_t     notify_id; 

  /* library stuff */
  l4thread_t                        notify_thread;
  l4blk_setup_notify_callback_fn_t  cb;
} blkclient_driver_t;

/* prototypes */
void
blkclient_init_drivers(void);

blkclient_driver_t *
blkclient_get_driver(l4blk_driver_t driver);

#endif /* !_GENERIC_BLK___DRIVER_H */
