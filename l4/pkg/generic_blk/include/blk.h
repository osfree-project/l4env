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
#include <l4/semaphore/semaphore.h>

/*****************************************************************************
 *** Types
 *****************************************************************************/

/**
 * Driver handle
 */
typedef l4_uint32_t l4blk_driver_t;

/**
 * Stream handle 
 */
typedef l4_uint32_t l4blk_stream_t;

/**
 * Request type
 */
typedef struct l4blk_request l4blk_request_t;

/**
 * Callback function prototype.
 *
 * \param request Request descriptor
 * \param status  Request status
 * \param error   Error code
 */
typedef void (* l4blk_callback_fn_t) (l4blk_request_t * request, 
                                      int status, 
                                      int error);

/**
 * Request structure
 */
struct l4blk_request
{
  l4blk_driver_t      driver;      ///< driver handle
 
  l4_uint32_t         cmd;         ///< command
  l4_uint32_t         device;      ///< device 
  l4_uint32_t         block;       ///< block number
  l4_uint32_t         count;       ///< number of blocks
  l4_addr_t           buf;         ///< address of target/source buffer

  /* real-time requests */
  l4blk_stream_t      stream;      ///< stream handle
  l4_uint32_t         req_no;      ///< request number
  l4_uint32_t         flags;       ///< flags

  /* client request handling */
  l4_uint32_t         status;      ///< request status
  int                 error;       ///< driver error code
  l4semaphore_t *     wait;        ///< client semaphore
  l4blk_callback_fn_t done;        ///< done callback function

  /* client data */
  void *              data;        ///< private client data
};

/* request commands */
#define L4BLK_REQUEST_READ      0x00000001  ///< read blocks
#define L4BLK_REQUEST_WRITE     0x00000002  ///< write blocks

/* request flags */
#define L4BLK_REQUEST_METADATA  0x00000001  ///< metadata request

/* request status */
#define L4BLK_UNPROCESSED       0x00000000  ///< not yet proccessed
#define L4BLK_DONE              0x00000001  ///< successfully done
#define L4BLK_ERROR             0x00000002  ///< I/O error
#define L4BLK_SKIPPED           0x00000003  ///< skipped

/* misc. */
#define L4BLK_INVALID_DRIVER    (-1)        ///< invalid driver handle
#define L4BLK_INVALID_STREAM    (-1)        ///< invalid stream handle

/* dafault ctrls */
#define L4BLK_CTRL_NUM_DISKS    0x00000001  ///< return number of disks 
#define L4BLK_CTRL_DISK_SIZE    0x00000002  ///< return size of disk
#define L4BLK_CTRL_DISK_GEOM    0x00000003  ///< return disk geometry
#define L4BLK_CTRL_RREAD_PART   0x00000004  ///< reread partition table
#define L4BLK_CTRL_MAX_SG_LEN   0x00000005  /**< return max. length of 
                                             **  scatter gather list
                                             **/

/* ctrls needed by L4Linux stub */
#define L4BLK_CTRL_DRV_IRQ      0x00008000  ///< return IRQ of block driver

/**
 * Disk geometry structure (CHS), returned by L4BLK_CTRL_DISK_GEOM ctrl
 */
typedef struct l4blk_disk_geometry
{
  l4_uint32_t heads;               ///< number of heads
  l4_uint32_t cylinders;           ///< number of cylinders (tracks)
  l4_uint32_t sectors;             ///< number of sectors / cylinder
  l4_uint32_t start;               /**< partition start sector 
                                    **  (if called for partition)
                                    **/
} l4blk_disk_geometry_t;

/*****************************************************************************
 *** Prototypes
 *****************************************************************************/

__BEGIN_DECLS;

/*****************************************************************************/
/**
 * \brief Setup block device driver client library.
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
 *	
 * \return 0 on success (\a driver contains a valid handle), error code 
 *         otherwise:
 *         - \c -L4_EINVAL     invalid name
 *         - \c -L4_ENOTFOUND  driver not found
 *         other error codes depend on the requested driver.
 *
 * \a l4blk_open_driver does two things:
 * - request the id of the driver at the DROPS nameserver using \a name
 * - call the driver to open new instances of the command and wait threads
 */
/*****************************************************************************/ 
int
l4blk_open_driver(const char * name, 
									l4blk_driver_t * driver);
  
/*****************************************************************************/
/**
 * \brief Close block device driver.
 * 
 * \param  driver        Driver handle.
 *	
 * \return 0 on success (close driver instance), error code otherwise:
 *         - \c -L4_EINVAL  invalid driver handle
 *
 * Close driver instance.
 */
/*****************************************************************************/ 
int 
l4blk_close_driver(l4blk_driver_t driver);

/*****************************************************************************/
/**
 * \brief Create real-time stream.
 * 
 * \param  driver        Driver handle
 * \param  bandwidth     Stream bandwidth (bytes/s) 
 * \param  blk_size      Stream block size (bytes)
 * \param  q             Quality parameter 
 * \param  meta_int      Metadata request interval (number of regular 
 *                       requests per metadata request)
 * \retval stream        Stream handle
 *	
 * \return 0 on success (admission succeeded, \a stream contains a valid 
 *         handle), error code otherwise. The error code depends on the 
 *         driver.
 *
 * Create real-time stream. If the driver can handle the requested stream, 
 * it reserves the requeired resources and returns a valid stream handle.
 */
/*****************************************************************************/ 
int
l4blk_create_stream(l4blk_driver_t driver, 
										l4_uint32_t bandwidth, 
										l4_uint32_t blk_size, 
										float q, 
										l4_uint32_t meta_int, 
										l4blk_stream_t * stream);

/*****************************************************************************/
/**
 * \brief Close real-time stream.
 * 
 * \param  driver        Driver handle
 * \param  stream        Stream handle
 *	
 * \return 0 on success (stream closed), error code otherwise.
 *         
 * Close real-time stream, release all resources assigned to the stream.
 */
/*****************************************************************************/ 
int
l4blk_close_stream(l4blk_driver_t driver, 
									 l4blk_stream_t stream);

/*****************************************************************************/
/**
 * \brief Set start time of real-time stream
 * 
 * \param  driver        Driver handle
 * \param  stream        Stream handle
 * \param  time          Time (deadline) of first request (milliseconds)
 * \param  request_no    Request number of first request
 *	
 * \return Time of first period on success, error code otherwise:
 *         - \c -L4_EIPC  IPC error calling driver
 */
/*****************************************************************************/ 
int l4blk_start_stream(l4blk_driver_t driver, 
											 l4blk_stream_t stream, 
											 l4_uint32_t time, 
											 l4_uint32_t request_no);

/*****************************************************************************/
/**
 * \brief Execute request (synchronously)
 * 
 * \param  request       Request structure
 *	
 * \return 0 on success (executed request), error code otherwise.
 *
 * Send the request to the driver an block until it is finished.
 */
/*****************************************************************************/ 
int
l4blk_do_request(l4blk_request_t * request);

/*****************************************************************************/
/**
 * \brief Send request list to driver.
 * 
 * \param  requests      Request list
 * \param  num           Number of requests in request list
 *	
 * \return 0 on succcess (sent requests to the driver), error code otherwise.
 *
 * Send all requests in \a requests to the driver and return immediately. 
 * There are two ways to check the status of the requests:
 * - a semaphore can be specified for each request (\a wait element of the
 *   request structure) which is incremented if the request is finished
 * - the status of a request can be checked at any time using the 
 *   l4blk_get_status() function
 */
/*****************************************************************************/ 
int
l4blk_put_requests(l4blk_request_t requests[], 
									 int num);

/*****************************************************************************/
/**
 * \brief Check status of a request.
 * 
 * \param  request       Request structure
 *	
 * \return Request status (>= 0):
 *         - #L4BLK_UNPROCESSED request not yet processed
 *         - #L4BLK_DONE        request finished successfully
 *         - #L4BLK_ERROR       I/O error processing request 
 *         - #L4BLK_SKIPPED     skipped real-time request 
 *                              (no time left in request period)
 *         error code (< 0):
 *         - -#L4_EINVAL        invalid request structure
 */
/*****************************************************************************/ 
int 
l4blk_get_status(l4blk_request_t * request);

/*****************************************************************************/
/**
 * \brief Return driver error code.
 * 
 * \param  request       Request structure
 *	
 * \return Error code returned by ther device driver, \c -L4_EINVAL if 
 *         invalid request structure.
 *
 * Return the error code returned by the driver, it can be used to check 
 * the error reason if the request status was set to \c L4BLK_ERROR.
 */
/*****************************************************************************/ 
int 
l4blk_get_error(l4blk_request_t * request);

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
 * \return result of ctrl-call to driver, \c -L4_EIPC if call failed.
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
					 int out_size);

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
l4blk_ctrl_get_num_disks(l4blk_driver_t driver);

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
l4blk_ctrl_get_disk_size(l4blk_driver_t driver, l4_uint32_t dev);

__END_DECLS;

#endif /* !_GENERIC_BLK_BLK_H */
