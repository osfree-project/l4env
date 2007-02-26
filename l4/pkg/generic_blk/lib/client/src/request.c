/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_blk/lib/client/src/request.c
 * \brief  Request handling.
 *
 * \date   02/11/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4/DROPS includes */
#include <l4/util/atomic.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>
#include <l4/env/errno.h>
#include <l4/semaphore/semaphore.h>

/* Library includes */
#include <l4/generic_blk/blk.h>
#include <l4/generic_blk/blk-client.h>
#include "__request.h"
#include "__driver.h"
#include "__config.h"

/*****************************************************************************
 *** Request descriptors
 *****************************************************************************/

static l4blk_request_t * l4blk_requests[BLKCLIENT_MAX_REQUESTS];

static int next = 0;

/*****************************************************************************
 *** internal stuff
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Init request handling
 */
/*****************************************************************************/ 
void
blkclient_init_requests(void)
{
  int i;

  for (i = 0; i < BLKCLIENT_MAX_REQUESTS; i++)
    l4blk_requests[i] = NULL;
}

/*****************************************************************************/
/**
 * \brief Set request status.
 * 
 * \param  req_handle    Request handle
 * \param  status        Request status
 * \param  error         Driver error code
 *
 * Set request status and wakeup client waiting for the request.
 */
/*****************************************************************************/ 
void
blkclient_set_request_status(l4_uint32_t req_handle, 
			     l4_uint32_t status,
			     int error)
{
  /* check request handle */
  if ((req_handle < 0) || (req_handle >= BLKCLIENT_MAX_REQUESTS))
    {
      LOG_Error("invalid request handle (%d)", req_handle);
      return;
    }

  if (l4blk_requests[req_handle] == NULL)
    {
      LOG_Error("no client request (handle %d, status %d, error %d)",
	    req_handle, status, error);
      return;
    }

  if ((status != L4BLK_DONE) && 
      (status != L4BLK_ERROR) && 
      (status != L4BLK_SKIPPED))
    {
      LOG_Error("invalid request status (%u)", status);
      l4blk_requests[req_handle]->status = L4BLK_ERROR;
      l4blk_requests[req_handle]->error = -L4_EINVAL;
      return;
    }

  /* set status */
  l4blk_requests[req_handle]->status = status;
  l4blk_requests[req_handle]->error = error; 

  /* call done-callback */
  if (l4blk_requests[req_handle]->done != NULL)
    l4blk_requests[req_handle]->done(l4blk_requests[req_handle], 
                                     status, error);

  /* check if someone is waiting for the request */
  if (l4blk_requests[req_handle]->wait != NULL)
    l4semaphore_up(l4blk_requests[req_handle]->wait);

  /* remove request from descriptor list */
  l4blk_requests[req_handle] = NULL;
}

/*****************************************************************************
 *** Helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Find unused request descriptor 
 * 
 * \param  request       Request structure
 *	
 * \return index of request descriptor (-> driver request handle), -1 if
 *         no descriptor found.
 */
/*****************************************************************************/ 
static inline int
__find_descriptor(l4blk_request_t * request)
{
  int i = next;
  int found = 0;

  do
    {
      if (l4util_cmpxchg32((l4_uint32_t *)&l4blk_requests[i],
			   (l4_uint32_t)NULL, (l4_uint32_t)request))
	{
	  /* found */
	  found = 1;
	  break;
	}
      i = (i + 1) % BLKCLIENT_MAX_REQUESTS;
    }
  while (i != next);
      
  if (!found)
    return -1;
  else
    {
      next = (i + 1) % BLKCLIENT_MAX_REQUESTS;
      return i;
    }
}

/*****************************************************************************/
/**
 * \brief Send request to driver
 * 
 * \param  request       Request list
 *	
 * \return 0 on success (sent request to driver), error code otherwise
 *         - -#L4_EINVAL  invalid driver handle 
 *         - -#L4_EIPC    IPC error calling driver
 */
/*****************************************************************************/ 
static int
__send_request(l4blk_request_t * request)
{
  blkclient_driver_t * drv;
  CORBA_Environment _env = dice_default_environment;
  int ret, sg_size;

  /* get driver descriptor */
  drv = blkclient_get_driver(request->driver);
  if (drv == NULL)
    {
      LOG_Error("invalid driver handle (%d)", request->driver);
      return -L4_EINVAL;
    }
  
  /* some sanity checks */
  if ((request->sg_num == 0) || (request->sg_list == NULL))
    {
      LOG_Error("no target buffer specified!");
      return -L4_EINVAL;
    }

  if ((request->sg_type != L4BLK_SG_PHYS) && (request->sg_type != L4BLK_SG_DS))
    {
      LOG_Error("invalid scatter-gather list type!");
      return -L4_EINVAL;
    }

  /* send request to driver */
  if (request->sg_type == L4BLK_SG_DS)
    sg_size = request->sg_num * sizeof(l4blk_sg_ds_elem_t);
  else
    sg_size = request->sg_num * sizeof(l4blk_sg_phys_elem_t);

  ret = l4blk_cmd_put_request_call(&drv->cmd_id, drv->handle, &request->request,
                                   request->sg_list, sg_size, request->sg_num, 
                                   request->sg_type, &_env);
  if (ret || (_env.major != CORBA_NO_EXCEPTION))
    {
      LOG_Error("error sending request to driver (ret %d, exc %d)",
                ret, _env.major);
      if (ret)
	return ret;
      else
	return -L4_EIPC;
    }
    
  /* done */
  return 0;
}

/*****************************************************************************
 *** API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Create real-time stream.
 * 
 * \param  driver        Driver handle
 * \param  device        Device id
 * \param  bandwidth     Stream bandwidth (bytes/s) 
 * \param  period        Period length (milliseconds)
 * \param  blk_size      Stream block size (bytes)
 * \param  q             Quality parameter 
 * \param  meta_int      Metadata request interval (number of regular 
 *                       requests per metadata request)
 * \retval stream        Stream handle
 *	
 * \return 0 on success (admission succeeded, \a stream contains a valid 
 *         handle), error code otherwise:
 *         - -#L4_EINVAL  invalid driver handle
 *         - -#L4_EIPC    IPC error calling driver
 * 
 * Call the driver to create real-time stream.
 */
/*****************************************************************************/ 
int
l4blk_create_stream(l4blk_driver_t driver, l4_uint32_t device,
                    l4_uint32_t bandwidth,  l4_uint32_t period, 
                    l4_uint32_t blk_size, float q, l4_uint32_t meta_int, 
		    l4blk_stream_t * stream)
{
  blkclient_driver_t * drv;
  int ret;
  CORBA_Environment _env = dice_default_environment;
  
  /* return invalid stream id on error */
  *stream = L4BLK_INVALID_STREAM;

  /* get driver descriptor */
  drv = blkclient_get_driver(driver);
  if (drv == NULL)
    {
      LOG_Error("invalid driver handle (%d)", driver);
      return -L4_EINVAL;
    }

  /* call driver to create new stream */
  ret = l4blk_cmd_create_stream_call(&drv->cmd_id, drv->handle, device, 
                                     bandwidth, period, blk_size, q, meta_int, 
                                     stream, &_env);
  if (ret || (_env.major != CORBA_NO_EXCEPTION))
    {
      LOG_Error("create stream failed (ret %d, exc %d)\n", 
                ret, _env.major);
      if (ret)
	return ret;
      else
	return -L4_EIPC;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Close real-time stream.
 * 
 * \param  driver        Driver handle
 * \param  stream        Stream handle
 *	
 * \return 0 on success (stream closed), error code otherwise:
 *         - -#L4_EINVAL  invalid driver handle
 *         - -#L4_EIPC    IPC error calling driver
 */
/*****************************************************************************/ 
int
l4blk_close_stream(l4blk_driver_t driver, l4blk_stream_t stream)
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

  /* call driver to close stream */
  ret = l4blk_cmd_close_stream_call(&drv->cmd_id, drv->handle, stream, &_env);
  if (ret || (_env.major != CORBA_NO_EXCEPTION))
    {
      LOG_Error("close stream failed (ret %d, exc %d)",
                ret, _env.major);
      if (ret)
	return ret;
      else
	return -L4_EIPC;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Set start time of real-time stream
 * 
 * \param  driver        Driver handle
 * \param  stream        Stream handle
 * \param  time          Time (deadline) of first request (milliseconds)
 * \param  request_no    Request number of first request
 *	
 * \return time of first period (< 0, milliseconds), error code otherwise:
 *         - -#L4_EIPC  IPC error calling driver
 */
/*****************************************************************************/ 
int l4blk_start_stream(l4blk_driver_t driver, l4blk_stream_t stream, 
		       l4_uint32_t time, l4_uint32_t request_no)
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

  /* call driver to start stream */
  ret = l4blk_cmd_start_stream_call(&drv->cmd_id, drv->handle, stream,
                                    time, request_no, &_env);
  if ((ret < 0) || (_env.major != CORBA_NO_EXCEPTION))
    {
      LOG_Error("set start time failed (ret %d, exc %d)",
                ret, _env.major);
      if (ret < 0)
	return ret;
      else
	return -L4_EIPC;
    }

  /* done */
  return ret;
}

/*****************************************************************************/
/**
 * \brief Execute request (synchronously)
 * 
 * \param  request       Request structure
 *	
 * \return 0 on success (executed request), error code otherwise:
 *         - -#L4_EBUSY     no request descriptor available
 *         - -#L4_EINVAL    invalid request structure 
 *         - -#L4_EIPC      error calling device driver
 *
 * Send the request to the driver an block until it is finished.
 *
 * \note We allocate the request list for the RPC function on the stack,
 *       be carefully if you change the size of the request list.
 */
/*****************************************************************************/ 
int
l4blk_do_request(l4blk_request_t * request)
{
  int r, ret;
  l4semaphore_t sem = L4SEMAPHORE_LOCKED;

  /* find request descriptor */
  r = __find_descriptor(request);
  if (r == -1)
    {
      LOG_Error("no request descriptor available!");
      return -L4_EBUSY;
    }

  /* setup request */
  request->request.req_handle = r;
  request->status = L4BLK_UNPROCESSED;
  request->error = 0;

  /* set wait semaphore/callback */
  request->wait = &sem;
  request->done = NULL;

  /* send request to the driver */
  ret = __send_request(request);
  if (ret)
    {
      /* error sending request */
      l4blk_requests[r] = NULL;
      return ret;
    }

  /* wait for request to be finished */
  l4semaphore_down(&sem);

  /* check request status */
  switch (request->status)
    {
    case L4BLK_UNPROCESSED:
      /* this should never happen */
      LOG_Error("got processed notification but request status " \
                "is UNPROCESSED!");
      return -L4_EINVAL;

    case L4BLK_DONE:
      /* success */
      return 0;
      
    case L4BLK_ERROR:
      /* driver error */
      return request->error;

    case L4BLK_SKIPPED:
      /* request skipped by the driver */
      return -L4_ESKIPPED;

    default:
      /* ??? */
      LOG_Error("invalid request status %d", request->status);
      return -L4_EINVAL;
    }
}

/*****************************************************************************/
/**
 * \brief Send request to driver.
 * 
 * \param  request       Request
 *	
 * \return 0 on succcess (sent request to the driver), error code otherwise:
 *         - -#L4_EINVAL  invalid request structure
 */
/*****************************************************************************/ 
int
l4blk_put_request(l4blk_request_t * request) 
{
  int r, ret;

  /* find request descriptor */
  r = __find_descriptor(request);
  if (r == -1)
    {
      LOG_Error("no request descriptor available!");
      return -L4_EBUSY;
    }
      
  /* setup request */
  request->request.req_handle = r;
  request->status = L4BLK_UNPROCESSED;
  request->error = 0;

  /* send request to driver */
  ret = __send_request(request);
  if (ret)
    {
      /* error sending request */
      l4blk_requests[r] = NULL;
      return ret;
    }
  
  /* return immediately  */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Check status of a request.
 * 
 * \param  request       Request structure
 *	
 * \return Request status (>= 0):
 *         - #L4BLK_UNPROCESSED  request not yet processed
 *         - #L4BLK_DONE         request finished successfully
 *         - #L4BLK_ERROR        I/O error processing request 
 *         - #L4BLK_SKIPPED      skipped real-time request 
 *                               (no time left in request period)
 *         error code (< 0):
 *         - -#L4_EINVAL         invalid request structure
 */
/*****************************************************************************/ 
int 
l4blk_get_status(l4blk_request_t * request)
{
  if (request == NULL)
    return -L4_EINVAL;

  /* return request status */
  return request->status;
}

/*****************************************************************************/
/**
 * \brief Return driver error code.
 * 
 * \param  request       Request structure
 *	
 * \return Error code returned by ther device driver, -#L4_EINVAL if 
 *         invalid request structure.
 */
/*****************************************************************************/ 
int 
l4blk_get_error(l4blk_request_t * request)
{
  if (request == NULL)
    return -L4_EINVAL;

  /* return error code */
  return request->error;
}





