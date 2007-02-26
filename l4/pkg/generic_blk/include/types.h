/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_blk/include/types.h
 * \brief  DROPS Block Device Driver Interface, API types
 * 
 * \date   08/30/2003
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _GENERIC_BLK_TYPES_H
#define _GENERIC_BLK_TYPES_H

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/semaphore/semaphore.h>
#include <l4/dm_generic/types.h>

/*****************************************************************************
 *** Types
 *****************************************************************************/

/**
 * Driver id, it is used to identify the driver instance at a block device 
 * driver
 * \ingroup idl_driver
 */
typedef l4_uint32_t l4blk_driver_id_t;

/**
 * Driver handle, it is used by the client library to identify a driver
 * \ingroup api_driver
 */
typedef l4_uint32_t l4blk_driver_t;

/**
 * Notification thread setup callback function
 * \ingroup api_driver
 */
typedef void (* l4blk_setup_notify_callback_fn_t) (void);

/**
 * Stream handle 
 * \ingroup api_stream
 */
typedef l4_uint32_t l4blk_stream_t;

/**
 * Request type
 * \ingroup api_cmd
 */
typedef struct l4blk_request l4blk_request_t;

/**
 * Request done callback function prototype.
 * \ingroup api_cmd
 *
 * \param request Request descriptor
 * \param status  Request status
 * \param error   Error code
 */
typedef void (* l4blk_callback_fn_t) (l4blk_request_t * request, 
                                      int status, 
                                      int error);

/**
 * Scatter-gather list element (phys. buffer address)
 * \ingroup api_cmd
 */
typedef struct l4blk_sg_phys_elem
{
  l4_addr_t addr;                  ///< buffer address
  l4_size_t size;                  ///< buffer size
} l4blk_sg_phys_elem_t;

/**
 * Scatter-gather list element (dataspace region)
 * \ingroup api_cmd
 */
typedef struct l4blk_sg_ds_elem
{
  l4dm_dataspace_t ds;             ///< dataspace id
  l4_offs_t        offs;           ///< offset in dataspace
  l4_size_t        size;           ///< size of buffer 
} l4blk_sg_ds_elem_t;

/**
 * Server request structure, it contains all information which are transfered
 * to a block device server
 * \ingroup api_cmd
 */
typedef struct l4blk_blk_request
{
  l4_uint32_t         req_handle;  /**< Client request handle, it is used by 
                                    **  the client library to tag the 
                                    **  outstanding requests
                                    **/

  l4_uint32_t         cmd;         /**< Command:
                                    **  - #L4BLK_REQUEST_READ
                                    **  - #L4BLK_REQUEST_WRITE 
                                    **/
  l4_uint32_t         device;      ///< Device 
  l4_uint32_t         block;       ///< Block number
  l4_uint32_t         count;       ///< Number of blocks

  /* real-time requests */
  l4blk_stream_t      stream;      /**< Stream handle, must be set to 
                                    **  #L4BLK_INVALID_STREAM for a 
                                    **  non-real-time request
                                    **/
  l4_uint32_t         req_no;      ///< Request number (for real-time streams)
  l4_uint32_t         flags;       /**< Flags:
                                    **  -#L4BLK_REQUEST_METADATA
                                    **/
} l4blk_blk_request_t;

/**
 * Client request structure, additionally to the server request structure it
 * contains local client data 
 * \ingroup api_cmd
 */
struct l4blk_request
{
  l4blk_driver_t      driver;      ///< Driver handle
 
  l4blk_blk_request_t request;     ///< Block request 

  /* buffer scatter-gather list */
  void *              sg_list;     /**< Scatter-gather list, it must be 
                                    **  set if no buffer address is specified 
                                    **  in the request structure. There are 
                                    **  two types of scatter-gather lists, 
                                    **  l4blk_sg_phys_elem_t which contains the
                                    **  phys. addresses of buffers and 
                                    **  l4blk_sg_ds_elem_t which contains 
                                    **  dataspace regions which describe the 
                                    **  buffer.
                                    **/
  int                 sg_num;      ///< Number of elements in sg_list
  int                 sg_type;     /**< Type of the scatter gather list:
                                    **  - #L4BLK_SG_PHYS
                                    **  - #L4BLK_SG_DS
                                    **/

  /* client request handling */
  l4_uint32_t         status;      ///< Request status
  int                 error;       ///< Driver error code
  l4semaphore_t *     wait;        ///< Client semaphore
  l4blk_callback_fn_t done;        ///< Done callback function

  /* client data */
  void *              data;        ///< Private client data
};

/* request commands */
#define L4BLK_REQUEST_READ      0x00000001  /**< \ingroup api_cmd 
                                             **  Block read request
                                             **/
#define L4BLK_REQUEST_WRITE     0x00000002  /**< \ingroup api_cmd
                                             **  Block write request
                                             **/

/* block size in bytes */
#define L4BLK_BLKSIZE           1024        ///< blocksize

/* request flags */
#define L4BLK_REQUEST_METADATA  0x00000001  /**< \ingroup api_cmd
                                             **  Metadata request
                                             **/

/* scatter-gather list types */
#define L4BLK_SG_PHYS           0x00000001  /**< l4blk_sg_phys_elem_t,
                                             **  the scatter-gather list
                                             **  contains the phys. addresses
                                             **  of the buffers
                                             **/
#define L4BLK_SG_DS             0x00000002  /**< l4blk_sg_ds_elem_t,
                                             **  the scatter-gather list 
                                             **  contains dataspace regions
                                             **  which describe the buffers
                                             **/
/* request status */
#define L4BLK_UNPROCESSED       0x00000000  /**< \ingroup api_cmd
                                             **  Not yet proccessed
                                             **/
#define L4BLK_DONE              0x00000001  /**< \ingroup api_cmd
                                             **  Successfully done
                                             **/
#define L4BLK_ERROR             0x00000002  /**< \ingroup api_cmd
                                             **  I/O error
                                             **/
#define L4BLK_SKIPPED           0x00000003  /**< \ingroup api_cmd
                                             **  Skipped
                                             **/

/* misc. */
#define L4BLK_INVALID_DRIVER    (-1)        ///< invalid driver handle
#define L4BLK_INVALID_STREAM    (-1)        /**< \ingroup api_stream
                                             **  Invalid stream handle
                                             **/

/* dafault ctrls */
#define L4BLK_CTRL_NUM_DISKS     0x00000001 /**< \ingroup api_ctrl
                                             **  Return number of disks 
                                             **/
#define L4BLK_CTRL_DISK_SIZE     0x00000002 /**< \ingroup api_ctrl
                                             **  Return size of disk
                                             **/
#define L4BLK_CTRL_DISK_GEOM     0x00000003 /**< \ingroup api_ctrl
                                             **  Return disk geometry
                                             **/
#define L4BLK_CTRL_RREAD_PART    0x00000004 /**< \ingroup api_ctrl
                                             **  Reread partition table
                                             **/
#define L4BLK_CTRL_MAX_SG_LEN    0x00000005 /**< \ingroup api_ctrl
                                             **  Return max. length of 
                                             **  scatter gather list
                                             **/
#define L4BLK_CTRL_STREAM_PERIOD 0x00000006 /**< \ingroup api_ctrl
                                             **  Return disk timings for 
                                             **  stream requests
                                             **/
/* ctrls needed by L4Linux stub */
#define L4BLK_CTRL_DRV_IRQ       0x00008000 /**< \ingroup api_ctrl
                                             **  Return IRQ of block driver
                                             **/

/**
 * Disk geometry structure (CHS), returned by L4BLK_CTRL_DISK_GEOM ctrl
 * \ingroup api_ctrl
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

/**
 * Disk timings structure, used by L4BLK_CTRL_DISK_TIMING ctrl
 * \ingroup api_ctrl
 */
typedef struct l4blk_disk_period
{
  l4_uint32_t period_len;          ///< period length
  l4_uint32_t period_offs;         ///< period offset
} l4blk_disk_period_t;

#endif /* !_GENERIC_BLK_TYPES_H */
