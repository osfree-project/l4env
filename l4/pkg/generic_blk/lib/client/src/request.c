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
      Error("invalid request handle (%d)\n",req_handle);
      return;
    }

  if (l4blk_requests[req_handle] == NULL)
    {
      Error("No client request (handle %d, status %d, error %d)\n",
	    req_handle,status,error);
      return;
    }

  if ((status != L4BLK_DONE) && (status != L4BLK_ERROR) && (status != L4BLK_SKIPPED))
    {
      Error("invalid request status (%u)\n",status);
      l4blk_requests[req_handle]->status = L4BLK_ERROR;
      l4blk_requests[req_handle]->error = -L4_EINVAL;
      return;
    }

  /* set status */
  l4blk_requests[req_handle]->status = status;
  l4blk_requests[req_handle]->error = error; 

  /* check if someone is waiting for the request */
  if (l4blk_requests[req_handle]->wait != NULL)
    l4semaphore_up(l4blk_requests[req_handle]->wait);

  /* call done-callback */
  if (l4blk_requests[req_handle]->done != NULL)
    l4blk_requests[req_handle]->done(l4blk_requests[req_handle],status,error);

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
      if (cmpxchg32((l4_uint32_t *)&l4blk_requests[i],
		    (l4_uint32_t)NULL,(l4_uint32_t)request))
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
 * \brief Send request list to driver
 * 
 * \param  driver        Driver handle
 * \param  requests      Request list
 * \param  num           Number of requests
 *	
 * \return 0 on success (sent requests to driver), error code otherwise
 *         - -#L4_EINVAL  invalid driver handle 
 *         - -#L4_EIPC    IPC error calling driver
 */
/*****************************************************************************/ 
static int
__send_requests(l4blk_driver_t driver, 
		l4blk_cmd_request_list_t requests,
		int num)
{
  blkclient_driver_t * drv;
  sm_exc_t exc;
  int ret;

  if (num == 0)
    /* nothing to do */
    return 0;

  /* get driver descriptor */
  drv = blkclient_get_driver(driver);
  if (drv == NULL)
    {
      Error("Invalid driver handle (%d)\n",driver);
      return -L4_EINVAL;
    }
  
  /* send request list to driver */
  ret = l4blk_cmd_put_requests(drv->cmd_id,drv->handle,requests,num,&exc);
  if (ret || (exc._type != exc_l4_no_exception))
    {
      Error("Error sending requests to driver (ret %d, exc %d)\n",
	    ret,exc._type);
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
 * \brief Cleanup request list.
 * 
 * \param  requests      Request list
 * \param  num           number of requests
 *
 * Error handling: remove all requests in the request list from the 
 * descriptor list.
 */
/*****************************************************************************/ 
static void
__cleanup_request_list(l4blk_cmd_request_list_t requests, 
		       int num)
{
  int i;

  for (i = 0; i < num; i++)
    {
      if ((requests[i].req_handle >= 0) && 
	  (requests[i].req_handle < BLKCLIENT_MAX_REQUESTS))
	l4blk_requests[requests[i].req_handle] = NULL;
    }
}

/*****************************************************************************
 *** API functions
 *****************************************************************************/

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
 *         handle), error code otherwise:
 *         - -#L4_EINVAL  invalid driver handle
 *         - -#L4_EIPC    IPC error calling driver
 * 
 * Call the driver to create real-time stream.
 */
/*****************************************************************************/ 
int
l4blk_create_stream(l4blk_driver_t driver, 
		    l4_uint32_t bandwidth, 
		    l4_uint32_t blk_size,
		    float q, 
		    l4_uint32_t meta_int, 
		    l4blk_stream_t * stream)
{
  blkclient_driver_t * drv;
  int ret;
  sm_exc_t exc;
  
  /* return invalid stream id on error */
  *stream = L4BLK_INVALID_STREAM;

  /* get driver descriptor */
  drv = blkclient_get_driver(driver);
  if (drv == NULL)
    {
      Error("Invalid driver handle (%d)\n",driver);
      return -L4_EINVAL;
    }

  /* call driver to create new stream */
  ret = l4blk_cmd_create_stream(drv->cmd_id,drv->handle,bandwidth,blk_size,
				q,meta_int,(l4blk_cmd_stream_handle_t *)stream,
				&exc);
  if (ret || (exc._type != exc_l4_no_exception))
    {
      Error("Create stream failed (ret %d, exc %d)\n",ret,exc._type);
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
l4blk_close_stream(l4blk_driver_t driver, 
		   l4blk_stream_t stream)
{
  blkclient_driver_t * drv;
  int ret;
  sm_exc_t exc;

  /* get driver descriptor */
  drv = blkclient_get_driver(driver);
  if (drv == NULL)
    {
      Error("Invalid driver handle (%d)\n",driver);
      return -L4_EINVAL;
    }

  /* call driver to close stream */
  ret = l4blk_cmd_close_stream(drv->cmd_id,drv->handle,stream,&exc);
  if (ret || (exc._type != exc_l4_no_exception))
    {
      Error("Close stream failed (ret %d, exc %d)\n",ret,exc._type);
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
int l4blk_start_stream(l4blk_driver_t driver, 
		       l4blk_stream_t stream, 
		       l4_uint32_t time, 
		       l4_uint32_t request_no)
{
  blkclient_driver_t * drv;
  int ret;
  sm_exc_t exc;

  /* get driver descriptor */
  drv = blkclient_get_driver(driver);
  if (drv == NULL)
    {
      Error("Invalid driver handle (%d)\n",driver);
      return -L4_EINVAL;
    }

  /* call driver to start stream */
  ret = l4blk_cmd_start_stream(drv->cmd_id,drv->handle,stream,time,request_no,
			     &exc);
  if ((ret < 0) || (exc._type != exc_l4_no_exception))
    {
      Error("Set start time failed (ret %d, exc %d)\n",ret,exc._type);
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
  int r,i,ret;
  l4blk_cmd_request_list_t req_list;
  l4blk_cmd_request_t * req;
  l4semaphore_t sem = L4SEMAPHORE_LOCKED;

  /* find request descriptor */
  r = __find_descriptor(request);
  if (r == -1)
    {
      Error("No request descriptor available!\n");
      return -L4_EBUSY;
    }

  /* setup RPC request list */
  req = &req_list[0];
  req->req_handle = r;
  req->cmd = request->cmd;
  req->device = request->device;
  req->block = request->block;
  req->count = request->count;
  req->buf = request->buf;
  req->stream = request->stream;
  req->request_no = request->req_no;
  req->flags = request->flags;

  /* replace streams not yet supported */
  for (i = 0; i < BLKCLIENT_MAX_REPLACE; i++)
    req->replaces[i] = L4BLK_INVALID_STREAM;

  /* set request status / return value */
  request->status = L4BLK_UNPROCESSED;
  request->error = 0;

  /* set wait semaphore/callback */
  request->wait = &sem;
  request->done = NULL;

  /* send request to the driver */
  ret = __send_requests(request->driver,req_list,1);
  if (ret)
    {
      /* error sending request */
      l4blk_requests[r] = NULL;
      return ret;
    }

  /* wait for request to be finished */
  l4semaphore_down(&sem);

  /* release request descriptor */
  l4blk_requests[r] = NULL;

  /* check request status */
  switch (request->status)
    {
    case L4BLK_UNPROCESSED:
      /* this should never happen */
      Error("got processed notification but request status is UNPROCESSED!\n");
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
      Error("invalid request status %d\n",request->status);
      return -L4_EINVAL;
    }
}

/*****************************************************************************/
/**
 * \brief Send request list to driver.
 * 
 * \param  requests      Request list
 * \param  num           Number of requests in request list
 *	
 * \return 0 on succcess (sent requests to the driver), error code otherwise:
 *         - -#L4_EINVAL  invalid request structure
 */
/*****************************************************************************/ 
int
l4blk_put_requests(l4blk_request_t requests[], 
		   int num)
{
  int i,j,k,r;
  int ret,error;
  l4blk_driver_t cur_drv;
  l4blk_request_t * request;
  l4blk_cmd_request_list_t req_list;
  l4blk_cmd_request_t * req;

  if (num == 0)
    /* nothing to do */
    return 0;

  /* create request list */
  j = 0;
  error = 0;
  cur_drv = requests[0].driver;
  for (i = 0; i < num; i++)
    {
      request = &requests[i];
      if ((j >= BLKCLIENT_MAX_RPC_REQUESTS) || (cur_drv != request->driver))
	{
	  /* send request list to driver */
	  ret = __send_requests(cur_drv,req_list,j);
	  if (ret)
	    {
	      /* error sending requests */
	      error = ret;
	      __cleanup_request_list(req_list,j);
	    }
	  j = 0;
	  cur_drv = request->driver;
	}

      /* find request descriptor */
      r = __find_descriptor(request);
      if (r == -1)
	{
	  Error("No request descriptor available!\n");
	  error = -L4_EBUSY;
	  break;
	}
      
      /* insert request into request list */
      req = &req_list[j];
      req->req_handle = r;
      req->cmd = request->cmd;
      req->device = request->device;
      req->block = request->block;
      req->count = request->count;
      req->buf = request->buf;
      req->stream = request->stream;
      req->request_no = request->req_no;
      req->flags = request->flags;
      
      /* replace streams not yet supported */
      for (k = 0; k < BLKCLIENT_MAX_REPLACE; k++)
	req->replaces[k] = L4BLK_INVALID_STREAM;

      /* set request status / return value */
      request->status = L4BLK_UNPROCESSED;
      request->error = 0;

      /* next request */
      j++;
    }
      
  if (j > 0)
    {
      /* send remaining requests to driver */
      ret = __send_requests(cur_drv,req_list,j);
      if (ret)
	{
	  /* error sending requests */
	  error = ret;
	  __cleanup_request_list(req_list,j);
	}
    }
  
  /* return immediately  */
  return error;
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





