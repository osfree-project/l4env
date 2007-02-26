/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_blk/include/blk.h
 * \brief  DROPS Block Device Driver Interface, client API
 *
 * \date   02/09/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _GENERIC_BLK_BLK_H
#define _GENERIC_BLK_BLK_H

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/env/cdefs.h>

/* generic_blk includes */
#include <l4/generic_blk/types.h>

/*****************************************************************************
 *** Prototypes
 *****************************************************************************/

__BEGIN_DECLS;

/*****************************************************************************/
/**
 * \brief   Setup block device driver client library.
 * \ingroup api_init
 * 
 * Initialize the client library, this function must be called before any
 * of the other functions is called.
 */
/*****************************************************************************/ 
void
l4blk_init(void);

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
                  l4blk_setup_notify_callback_fn_t cb);
  
/*****************************************************************************/
/**
 * \brief   Close block device driver.
 * \ingroup api_driver
 * 
 * \param   driver       Driver handle.
 *	
 * \return  0 on success (close driver instance), error code otherwise:
 *          - -#L4_EINVAL  invalid driver handle
 *
 * Close driver instance.
 */
/*****************************************************************************/ 
int 
l4blk_close_driver(l4blk_driver_t driver);

/*****************************************************************************/
/**
 * \brief   Return thread id of command thread
 * \ingroup api_driver
 *
 * \param   driver       Driver handle
 *	
 * \return  Thread id of command thread, L4_INVALID_ID if invalid driver handle
 */
/*****************************************************************************/ 
l4_threadid_t 
l4blk_get_driver_thread(l4blk_driver_t driver);

/*****************************************************************************/
/**
 * \brief   Create real-time stream.
 * \ingroup api_stream
 * 
 * \param   driver       Driver handle
 * \param   device       Device id
 * \param   bandwidth    Stream bandwidth (bytes/s) 
 * \param   period       Period length (milliseconds)
 * \param   blk_size     Stream block size (bytes)
 * \param   q            Quality parameter 
 * \param   meta_int     Metadata request interval (number of regular 
 *                       requests per metadata request)
 * \retval stream        Stream handle
 *	
 * \return  0 on success (admission succeeded, \a stream contains a valid 
 *          handle), error code otherwise. The error code depends on the 
 *          driver.
 *
 * Create real-time stream. If the driver can handle the requested stream, 
 * it reserves the requeired resources and returns a valid stream handle.
 */
/*****************************************************************************/ 
int
l4blk_create_stream(l4blk_driver_t driver, l4_uint32_t device,
                    l4_uint32_t bandwidth, l4_uint32_t period, 
                    l4_uint32_t blk_size, float q, l4_uint32_t meta_int, 
                    l4blk_stream_t * stream);

/*****************************************************************************/
/**
 * \brief   Close real-time stream.
 * \ingroup api_stream
 * 
 * \param   driver       Driver handle
 * \param   stream       Stream handle
 *	
 * \return  0 on success (stream closed), error code otherwise.
 *         
 * Close real-time stream, release all resources assigned to the stream.
 */
/*****************************************************************************/ 
int
l4blk_close_stream(l4blk_driver_t driver, l4blk_stream_t stream);

/*****************************************************************************/
/**
 * \brief   Set start time of real-time stream
 * \ingroup api_stream
 * 
 * \param   driver       Driver handle
 * \param   stream       Stream handle
 * \param   time         Time (deadline) of first request (milliseconds)
 * \param   request_no   Request number of first request
 *	
 * \return  Time of first period on success, error code otherwise.
 */
/*****************************************************************************/ 
int l4blk_start_stream(l4blk_driver_t driver, l4blk_stream_t stream, 
                       l4_uint32_t time, l4_uint32_t request_no);

/*****************************************************************************/
/**
 * \brief   Execute request (synchronously)
 * \ingroup api_cmd
 * 
 * \param   request      Request structure
 *	
 * \return  0 on success (executed request), error code otherwise.
 *
 * Send the request to the driver an block until it is finished.
 */
/*****************************************************************************/ 
int
l4blk_do_request(l4blk_request_t * request);

/*****************************************************************************/
/**
 * \brief   Send request to driver.
 * \ingroup api_cmd
 * 
 * \param   request      Request structure, it describes the block request
 *	
 * \return  0 on succcess (sent requests to the driver), error code otherwise.
 *
 * Send the request to the driver and return immediately. 
 * There are several ways to check the status of the requests:
 * - a semaphore can be specified for each request (\a wait element of the
 *   request structure) which is incremented if the request is finished
 * - a callback function can be specified in the request structure (\a done)
 *   which will be called if the request is finished. Note that this function 
 *   is be called in the context of a different thread 
 * - the status of a request can be checked at any time using the 
 *   l4blk_get_status() function
 */
/*****************************************************************************/ 
int
l4blk_put_request(l4blk_request_t * request);

/*****************************************************************************/
/**
 * \brief   Check status of a request.
 * \ingroup api_cmd
 * 
 * \param   request      Request structure
 *	
 * \return  Request status (>= 0):
 *          - #L4BLK_UNPROCESSED request not yet processed
 *          - #L4BLK_DONE        request finished successfully
 *          - #L4BLK_ERROR       I/O error processing request 
 *          - #L4BLK_SKIPPED     skipped real-time request 
 *                               (no time left in request period)
 *          error code (< 0):
 *          - -#L4_EINVAL        invalid request structure
 */
/*****************************************************************************/ 
int 
l4blk_get_status(l4blk_request_t * request);

/*****************************************************************************/
/**
 * \brief   Return driver error code.
 * \ingroup api_cmd
 * 
 * \param   request      Request structure
 *	
 * \return  Error code returned by ther device driver, -#L4_EINVAL if 
 *          invalid request structure.
 *
 * Return the error code returned by the driver, it can be used to check 
 * the error reason if the request status was set to #L4BLK_ERROR.
 */
/*****************************************************************************/ 
int 
l4blk_get_error(l4blk_request_t * request);

/*****************************************************************************/
/**
 * \brief   Generic driver control.
 * \ingroup api_ctrl
 * 
 * \param   driver       Driver handle
 * \param   cmd          Control command
 * \param   in           Input buffer
 * \param   in_size      Size of input buffer
 * \param   out          Output buffer
 * \param   out_size     Size of output buffer
 *	
 * \return  result of ctrl-call to driver, -#L4_EIPC if call failed.
 *
 * This function is the 'swiss army knife' to manipulate various driver
 * parameters. The possible commands depend on the used driver.
 */
/*****************************************************************************/ 
int
l4blk_ctrl(l4blk_driver_t driver, l4_uint32_t cmd, void * in, int in_size, 
           void * out, int out_size);

/*****************************************************************************/
/**
 * \brief   Return number of disks
 * \ingroup api_ctrl
 * 
 * \param   driver       Driver handle
 *	
 * \return  Number of disks connected to the driver, error code (< 0) if failed
 */
/*****************************************************************************/ 
int
l4blk_ctrl_get_num_disks(l4blk_driver_t driver);

/*****************************************************************************/
/**
 * \brief   Return disk size
 * \ingroup api_ctrl
 * 
 * \param   driver       Driver handle
 * \param   dev          Device id
 *	
 * \return  Disk size in blocks (1KB), error code (< 0) if failed
 */
/*****************************************************************************/ 
int
l4blk_ctrl_get_disk_size(l4blk_driver_t driver, l4_uint32_t dev);

/*****************************************************************************/
/**
 * \brief   Return period for stream requests
 * \ingroup api_ctrl
 * 
 * \param   driver       Driver handle
 * \param   stream       Stream handle
 * \retval  period_len   Period length (microseconds)
 * \retval  period_offs  Period offset 
 *                       (relative to kernel klock, i.e. period0 % period_len)
 *	
 * \return 0 on success, error code if failed
 */
/*****************************************************************************/ 
int
l4blk_ctrl_get_stream_period(l4blk_driver_t driver, l4blk_stream_t stream, 
                             l4_uint32_t * period_len, 
                             l4_uint32_t * period_offs);

__END_DECLS;

#endif /* !_GENERIC_BLK_BLK_H */
