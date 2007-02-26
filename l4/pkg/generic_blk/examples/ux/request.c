/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_blk/examples/ux/request.c
 * \brief  block driver for Fiasco/UX, request handling
 *
 * \date   09/24/2003
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 * This version of request.c is derived from request.c of the oskit example.
 */
/*****************************************************************************/

/* standard includes */
#include <string.h>
#include <stdlib.h>

/* LX/UX libc */
#include <l4/lxfuxlibc/lxfuxlc.h>

/* L4 includes */
#include <l4/thread/thread.h>
#include <l4/l4rm/l4rm.h>
#include <l4/semaphore/semaphore.h>
#include <l4/lock/lock.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>

/* generic_blk includes */
#include <l4/generic_blk/blk.h>
#include <l4/generic_blk/blk-server.h>
#include "blksrv.h"
#include "debug.h"

/*****************************************************************************
 *** global stuff
 *****************************************************************************/

/* request thread */
static l4_threadid_t req_id = L4_INVALID_ID;

/* request list */
static blksrv_request_t * requests;
static l4semaphore_t request_wait = L4SEMAPHORE_INIT(0);
static l4lock_t request_lock = L4LOCK_UNLOCKED;

/* driver specifics */
static char * device_name;
static int blk_fd;
static long blk_file_pos;
static int readwrite;

/* disk stats */
static unsigned long disk_size;              // disk size (blocks)

#define COPY_TO    1
#define COPY_FROM  2

//l4_ssize_t l4libc_heapsize = 1 << 20;

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Copy to/from dataspaces
 *
 * \param  buf           Local buffer
 * \param  ds_buf        Dataspace buffer descriptors
 * \param  num           Number of elements in ds_buf
 * \param  direction     Copy direction:
 *                       - #COPY_TO    copy data to dataspace buffers
 *                       - #COPY_FROM  copy data from dataspace buffers
 */
/*****************************************************************************/
static void
__copy_buffer(void * buf, blksrv_buffer_t * ds_buf, int num, int direction)
{
  int i;
  void * p;

  p = buf;
  for (i = 0; i < num; i++)
    {
      if (direction == COPY_TO)
        {
          LOGdL(DEBUG_COPY, "copy to client buffer, " \
                "0x%08lx -> 0x%08lx (%lu bytes)", (l4_addr_t)p,
                (l4_addr_t)ds_buf[i].map_addr, (l4_addr_t)ds_buf[i].size);
          memcpy(ds_buf[i].map_addr, p, ds_buf[i].size);
        }
      else
        {
          LOGdL(DEBUG_COPY, "copy from client buffer, " \
                "0x%08lx -> 0x%08lx (%lu bytes)",
                (l4_addr_t)ds_buf[i].map_addr, (l4_addr_t)p,
		(l4_addr_t)ds_buf[i].size);
          memcpy(p, ds_buf[i].map_addr, ds_buf[i].size);
        }

      p += ds_buf[i].size;
    }
}

/*****************************************************************************/
/**
 * \brief  Execute request
 *
 * \param  request       Request descriptor
 */
/*****************************************************************************/
static void
__do_request(blksrv_request_t * request)
{
  unsigned long offset;
  unsigned long size;
  void * buf;
  int ret, i, retval;

  LOGdL(DEBUG_REQUEST, "%s request, block %u, %u blocks",
        (request->req.cmd == L4BLK_REQUEST_READ) ? "read" : "write",
        request->req.block, request->req.count);

  /* check request */
  if ((request->req.block + request->req.count) > disk_size)
    {
      LOG_Error("access beyond end of device, block %u, %u blocks, " \
                "disk size %lu", request->req.block, request->req.count,
                disk_size);
      blksrv_do_notification(request->driver, request->req.req_handle,
                             L4BLK_ERROR, -L4_EINVAL);
      return;
    }

  /* */
  offset = request->req.block * L4BLK_BLKSIZE;
  size =   request->req.count * L4BLK_BLKSIZE;

  /* allocate temp buffer */
  buf = malloc(size);
  if (buf == NULL)
    {
      LOG_Error("buffer allocation failed");
      blksrv_do_notification(request->driver, request->req.req_handle,
                             L4BLK_ERROR, -L4_ENOMEM);
      return;
    }
  l4_touch_rw(buf, size);

  /* do request */

  /* seek if necessary */
  if (blk_file_pos != offset)
    if ((blk_file_pos = lx_lseek(blk_fd, offset, SEEK_SET)) < 0)
      LOG_Error("Can't seek in device file (errno = %d)!", lx_errno);

  retval = 0;
  if (request->req.cmd == L4BLK_REQUEST_READ)
    {
      /* read */
      ret = lx_read(blk_fd, buf, size);
      if ((ret < 0) || (ret != size))
        {
          LOG_Error("read failed: %x, size %lu, got %d", ret, size, ret);
          retval = -L4_EIO;
        }

      /* copy data to dataspace buffers */
      __copy_buffer(buf, request->bufs, request->num, COPY_TO);
    }
  else
    {
      /* copy data from dataspace buffers */
      __copy_buffer(buf, request->bufs, request->num, COPY_FROM);

      /* write */
      ret = lx_write(blk_fd, buf, size);
      if ((ret < 0) || (ret != size))
        {
          LOG_Error("write failed: %x, size %lu, did %d", ret, size, ret);
          retval = -L4_EIO;
        }
    }

  blk_file_pos += size;


  /* unmap dataspace buffers */
  for (i = 0; i < request->num; i++)
    l4rm_detach(request->bufs[i].map_addr);

  //KDEBUG("request done.");

  /* notify client */
  blksrv_do_notification(request->driver, request->req.req_handle,
                         (retval == 0) ? L4BLK_DONE : L4BLK_ERROR, retval);

  /* done */
  free(buf);
}

/*****************************************************************************/
/**
 * \brief  Request handling thread
 *
 * \param  data          Thread data
 */
/*****************************************************************************/
static void
__request_thread(void * data)
{
  int ret, rw;
  int retval = -L4_EINVAL;
  struct lx_stat sbuf;
  blksrv_request_t * req;

  /* check device file */
  if (device_name == NULL)
    {
      LOG_Error("no file specified!");
      goto startup_error;
    }

  /* open device */
  rw = (readwrite) ? LX_O_RDWR : LX_O_RDONLY;
  if ((blk_fd = lx_open(device_name, rw, 0)) == -1)
    {
      LOG_Error("Cannot open device file: %s (errno = %d)!", device_name, lx_errno);
      retval = -L4_ENODEV;
      goto startup_error;
    }
  else
    LOGdL(DEBUG_OSKIT_STARTUP, "opened device file \'%s\' (%s)",
          device_name, (readwrite) ? "rw" : "ro");

  /* get disk size */
  if ((ret = lx_fstat(blk_fd, &sbuf)) < 0)
    LOG_Error("get disk size failed: %x", ret);
  else
    disk_size = (unsigned long)(sbuf.st_size / L4BLK_BLKSIZE);

  LOGdL(DEBUG_OSKIT_STARTUP, "disk size %lu MB, block size %d bytes",
        (unsigned long)(sbuf.st_size >> 20), L4BLK_BLKSIZE);

  /* request thread started */
  l4thread_started((void *)0);

  LOG("opened device %s (%s), disk size %lu MB",
      device_name, (readwrite) ? "rw" : "ro", (unsigned long)(sbuf.st_size >> 20));

  /* request handling loop */
  while (1)
    {
      /* wait for new request */
      l4semaphore_down(&request_wait);

      /* dequeue request */
      l4lock_lock(&request_lock);

      req = requests;
      if (req == NULL)
        LOG_Error("woke up but no request pending!?");
      else
        requests = req->next;

      l4lock_unlock(&request_lock);

      /* do request */
      if (req != NULL)
        {
          __do_request(req);
          free(req);
        }
    }

 startup_error:
  l4thread_started((void *)retval);
  l4thread_sleep_forever();
}

/*****************************************************************************
 *** local functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Start request service thread
 *
 * \return 0 if thread started successfully, errorcode otherwise
 */
/*****************************************************************************/
int
blksrv_start_request_thread(void)
{
  l4thread_t t;
  int retval;

  /* start thread */
  t = l4thread_create(__request_thread, NULL, L4THREAD_CREATE_SYNC);
  if (t < 0)
    return t;

  /* check startup return */
  retval = (int)l4thread_startup_return(t);
  if (retval < 0)
    {
      /* shutdown thread */
      l4thread_shutdown(t);
      return retval;
    }

  req_id = l4thread_l4_id(t);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Enqueue new request
 *
 * \param  driver        Driver descriptor
 * \param  request       Request descriptor
 * \param  sg_list       Scatter gather list
 * \param  sg_num        Number of elements in scatter gather list
 *
 * \return 0 if successfully enqueued request, errorcode otherwise
 */
/*****************************************************************************/
int
blksrv_enqueue_request(blksrv_driver_t * driver,
                       const l4blk_blk_request_t * request,
                       const l4blk_sg_ds_elem_t * sg_list,
                       l4_int32_t sg_num)
{
  blksrv_request_t * req;
  blksrv_request_t * rp;
  int i, j;
  int retval = L4_ENOMEM;

  /* enqueue request */
  req = malloc(sizeof(blksrv_request_t));
  if (req == NULL)
    goto error;

  req->bufs = malloc(sg_num * sizeof(blksrv_buffer_t));
  if (req->bufs == NULL)
    goto error_req;

  /* copy request descriptor */
  req->driver = driver;
  memcpy(&req->req, request, sizeof(l4blk_blk_request_t));
  req->num = sg_num;

  /* map dataspaces */
  for (i = 0; i < sg_num; i++)
    {
      /* attach dataspace */
      retval = l4rm_attach((l4dm_dataspace_t *)&sg_list[i].ds, sg_list[i].size,
                           sg_list[i].offs, L4DM_RW | L4RM_MAP,
                           &req->bufs[i].map_addr);
      if (retval < 0)
        {
          LOG_Error("attach buffer ds failed: %s (%d)",
                    l4env_errstr(retval), retval);
          for (j = 0; j < i - 1; j++)
            l4rm_detach(req->bufs[j].map_addr);

          goto error_bufs;
        }

      /* we must give the request thread access rights to the dataspace,
       * it detaches the dataspace */
      retval = l4dm_share((l4dm_dataspace_t *)&sg_list[i].ds, req_id, L4DM_RW);
      if (retval < 0)
        {
          LOG_Error("share buffer ds with request thread failed: %s (%d)",
                    l4env_errstr(retval), retval);
          for (j = 0; j <= i; j++)
            l4rm_detach(req->bufs[j].map_addr);

          goto error_bufs;
        }

      req->bufs[i].ds = sg_list[i].ds;
      req->bufs[i].size = sg_list[i].size;

      LOGdL(DEBUG_MAP_DS, "mapped ds %d at "l4util_idfmt" to %p, size %u",
            sg_list[i].ds.id, l4util_idstr(sg_list[i].ds.manager),
            req->bufs[i].map_addr, req->bufs[i].size);
    }

  /* enqueue request */
  l4lock_lock(&request_lock);

  req->next = NULL;
  if (requests == NULL)
    requests = req;
  else
    {
      rp = requests;
      while (rp->next != NULL)
        rp = rp->next;

      rp->next = req;
    }

  l4lock_unlock(&request_lock);

  /* wakeup request thread */
  l4semaphore_up(&request_wait);

  /* done */
  return 0;

 error_bufs:
  free(req->bufs);

 error_req:
  free(req);

 error:
  return retval;
}

/*****************************************************************************/
/**
 * \brief  Set device name
 *
 * \param  device        Device name
 */
/*****************************************************************************/
void
blksrv_dev_set_device(const char * device)
{
  if (device)
    device_name = strdup(device);
}

/*****************************************************************************/
/**
 * \brief  Open device read/write
 */
/*****************************************************************************/
void
blksrv_dev_read_write(void)
{
  readwrite = 1;
}

/*****************************************************************************/
/**
 * \brief  Return number of disks
 */
/*****************************************************************************/
int
blksrv_dev_num(void)
{
  return 1;
}

/*****************************************************************************/
/**
 * \brief  Return disk size
 *
 * \return Disk size
 */
/*****************************************************************************/
unsigned long
blksrv_dev_size(void)
{
  return disk_size;
}
