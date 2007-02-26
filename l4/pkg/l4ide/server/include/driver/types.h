/*****************************************************************************/
/**
 * \file   l4ide/src/include/driver/types.h
 * \brief  L4IDE type definitions
 *
 * \date   01/27/2004
 * \author Marek Menzer <mm19@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

#ifndef _L4IDE_TYPES_H
#define _L4IDE_TYPES_H

/* Linux includes */
#include <linux/types.h>
#include <linux/blkdev.h>
#include <asm/scatterlist.h>

/* L4/DROPS includes */
#include <l4/sys/types.h>
#include <l4/lock/lock.h>
#include <l4/semaphore/semaphore.h>
#include <l4/generic_blk/blk.h>
#include <l4/generic_blk/blk-server.h>

#define L4IDE_NUM_READY_STREAMS ((CONFIG_L4IDE_NUM_STREAM_REQS) + 2)

/* typedefs */
typedef struct l4ide_driver       l4ide_driver_t;
typedef struct l4ide_notification l4ide_notification_t;
typedef struct l4ide_stream       l4ide_stream_t;
typedef struct l4ide_request      l4ide_request_t;

/*****************************************************************************
 *** Client driver instance
 *****************************************************************************/

/**
 * Driver instance descriptor
 */
struct l4ide_driver
{
  l4blk_driver_id_t       handle;     ///< client driver handle
  l4_threadid_t           client;     ///< client id

  l4thread_t              cmd_th;     ///< command thread 
  l4thread_t              notify_th;  ///< notification thread

  /* processed notification stuff */
  l4semaphore_t           wait;       ///< wakeup semaphore
  l4ide_notification_t *  pending;    ///< list of pending notifications
  l4lock_t                p_lock;     ///< list lock
};

/*****************************************************************************
 *** Pending client notification descriptor
 *****************************************************************************/

/**
 * Pending notification
 */
struct l4ide_notification
{
  l4_uint32_t                  handle;  ///< client request handle

  l4_uint32_t                  status;  ///< request status
  int                          error;   ///< error code

  struct l4ide_notification *  next;    ///< notification list
};

/*****************************************************************************
 *** Stream descriptor
 *****************************************************************************/

/**
 * Stream descriptor
 */
struct l4ide_stream
{
  l4blk_stream_t    handle;            ///< client stream handle

  l4_uint32_t       bandwidth;         ///< bandwidth
  l4_uint32_t       blk_size;          ///< block size
  float             q;                 ///< quality parameter

  l4ide_driver_t *  driver;            ///< driver descriptor 

  /* scheduling */
  l4_uint32_t       period0;           ///< first period
  l4_uint32_t       request0;          ///< first client request number
  double            req_per_period;    ///< number of requests per period
  l4_int32_t        reservation_time;  ///< reservation time (microseconds)
};

#endif /* !_L4IDE_TYPES_H */
