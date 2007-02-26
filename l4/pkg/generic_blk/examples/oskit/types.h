/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_blk/examples/oskit/types.h
 * \brief  OSKit block driver, typedefs 
 *
 * \date   09/13/2003
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _BLKSRV_TYPES_H
#define _BLKSRV_TYPES_H

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/thread/thread.h>
#include <l4/semaphore/semaphore.h>
#include <l4/lock/lock.h>

/* generic_blk includes */
#include <l4/generic_blk/blk.h>

/**
 * Client notification
 */
typedef struct blksrv_notification
{
  l4_uint32_t                  handle;   ///< client request handle
  l4_uint32_t                  status;   ///< request status
  int                          error;    ///< error code

  struct blksrv_notification * next;     ///< notification list

} blksrv_notification_t;

/**
 * Driver instance 
 */
typedef struct blksrv_driver
{
  l4blk_driver_id_t       driver;        ///< driver id

  /* client notifications */
  l4thread_t              notify_th;     ///< notification thread
  l4semaphore_t           wait;          ///< wakeup semaphore
  
  blksrv_notification_t * notifications; ///< notification list
  l4lock_t                lock;          ///< list lock
} blksrv_driver_t;

/**
 * Buffer descriptor 
 */
typedef struct blksrv_buffer
{
  l4dm_dataspace_t ds;                   ///< dataspace id
  l4_size_t        size;                 ///< size

  void *           map_addr;             ///< mapp address
} blksrv_buffer_t;

/**
 * Request descriptor 
 */
typedef struct blksrv_request
{
  blksrv_driver_t  *       driver;       ///< driver descriptor
  l4blk_blk_request_t      req;          ///< request
  blksrv_buffer_t *        bufs;         ///< buffer list
  int                      num;          ///< number of elements in buffer list

  struct blksrv_request *  next;         ///< pending request list
} blksrv_request_t;

#endif /* !_BLKSRV_TYPES_H */
