/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_blk/examples/oskit/request.c
 * \brief  OSKit block driver, request handling
 *
 * \date   09/20/2003
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* standard includes */
#include <string.h>
#include <stdlib.h>

/* OSKit includes */
#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>

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

/* OSKit driver */
static int use_ide = 0;
static int use_scsi = 0;
static int readwrite = 0;
static char * scsi_adapter = NULL;
static char * device_name = NULL;
static oskit_blkio_t * oskit_disk = NULL;    // OSKit block device object

/* disk stats */
static unsigned long disk_size;              // disk size (blocks)

#define COPY_TO    1
#define COPY_FROM  2

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
                "0x%08x -> 0x%08x (%u bytes)", (l4_addr_t)p, 
                (l4_addr_t)ds_buf[i].map_addr, (l4_addr_t)ds_buf[i].size);
          memcpy(ds_buf[i].map_addr, p, ds_buf[i].size);
        }
      else
        {
          LOGdL(DEBUG_COPY, "copy from client buffer, " \
                "0x%08x -> 0x%08x (%u bytes)",
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
  oskit_off_t offset;
  oskit_size_t size, got;
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

  /* OSKit uses byte offsets / sizes */
  offset = (oskit_off_t)request->req.block * L4BLK_BLKSIZE;
  size = (oskit_size_t)request->req.count * L4BLK_BLKSIZE;

  /* allocate temp buffer */
  buf = malloc(size);
  if (buf == NULL)
    {
      LOG_Error("buffer allocation failed");
      blksrv_do_notification(request->driver, request->req.req_handle, 
                             L4BLK_ERROR, -L4_ENOMEM);
      return;
    }

  /* do request */
  retval = 0;
  if (request->req.cmd == L4BLK_REQUEST_READ)
    {
      /* read */
      ret = oskit_blkio_read(oskit_disk, buf, offset, size, &got);
      if ((ret < 0) || (got != size))
        {
          LOG_Error("read failed: %x, size %u, got %u", ret, size, got);
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
      ret = oskit_blkio_write(oskit_disk, buf, offset, size, &got);
      if ((ret < 0) || (got != size))
        {
          LOG_Error("write failed: %x, size %u, did %u", ret, size, got);
          retval = -L4_EIO;
        }
    }

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
  oskit_osenv_t *osenv;
  int ret, rw;
  int retval = -L4_EINVAL;
  oskit_off_t tmp;
  blksrv_request_t * req;

  /* setup OSKit */
  if (device_name == NULL)
    {
      LOG_Error("no device specified!");
      goto startup_error;
    }

  if (!use_ide && !use_scsi)
    {
      LOG_Error("no driver specified!");
      goto startup_error;
    }

  /* create OSKit os environment */
  osenv = oskit_osenv_create_default();
  ret = oskit_register(&oskit_osenv_iid, (void *) osenv);
  if (ret < 0)
    {
      LOG_Error("register OS environment failed: %x", ret);
      goto startup_error;
    }

  oskit_dev_init(osenv);
  oskit_linux_init_osenv(osenv);

  /* setup block devices - IDE */
  if (use_ide)
    {
      ret = oskit_linux_init_ide();
      if (ret < 0)
        {
          LOG_Error("open IDE driver failed: %x", ret);
          goto startup_error;
        }
    }

  /* setup block devices - SCSI */
  if ((use_scsi) && (scsi_adapter != NULL))
    {
      if (strcmp(scsi_adapter, "ncr53c8xx") == 0)
        ret = oskit_linux_init_scsi_ncr53c8xx();
      else if (strcmp(scsi_adapter, "buslogic") == 0)
        ret = oskit_linux_init_scsi_buslogic();      
      else
        {
          LOG_Error("unknown SCSI driver \'%s\'!", scsi_adapter);
          goto startup_error;
        }

      if (ret < 0)
        {
          LOG_Error("open SCSI driver \'%s\' failed: %x", scsi_adapter, ret);
          goto startup_error;
        }
    }
  oskit_dev_probe();
  LOGdL(DEBUG_OSKIT_STARTUP, "initialized drivers");

  /* open device */
  rw = (readwrite) ? OSKIT_DEV_OPEN_WRITE : OSKIT_DEV_OPEN_READ;
  ret = oskit_linux_block_open(device_name, rw, &oskit_disk);
  if (ret < 0)
    {
      LOG_Error("open device \'%s\' failed: %x", device_name, ret);
      retval = -L4_ENODEV;
      oskit_disk = NULL;
      goto startup_error;
    }
  else
    LOGdL(DEBUG_OSKIT_STARTUP, "opened device \'%s\' (%s)", 
          device_name, (readwrite) ? "rw" : "ro");
      
  /* get disk size */
  ret = oskit_blkio_getsize(oskit_disk, &tmp);
  if (ret < 0)
    LOG_Error("get disk size failed: %x", ret);
  else
    disk_size = (unsigned long)(tmp / L4BLK_BLKSIZE);

  LOGdL(DEBUG_OSKIT_STARTUP, "disk size %lu MB, block size %d bytes", 
        (unsigned long)(tmp >> 20), L4BLK_BLKSIZE);

  /* request thread started */
  l4thread_started((void *)0);

  LOG("opened device %s (%s), disk size %lu MB",
      device_name, (readwrite) ? "rw" : "ro", (unsigned long)(tmp >> 20));

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
 * \brief  Enable IDE driver
 */
/*****************************************************************************/ 
void
blksrv_dev_use_ide(void)
{
  use_ide = 1;
}

/*****************************************************************************/
/**
 * \brief  Enable SCSI driver
 * 
 * \param  adapter       Name of SCSI adapter
 */
/*****************************************************************************/ 
void
blksrv_dev_use_scsi(const char * adapter)
{
  if (adapter != NULL)
    {
      use_scsi = 1;
      scsi_adapter = strdup(adapter);
      if (scsi_adapter == NULL)
        use_scsi = 0;
    }
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
  if (oskit_disk != NULL)
    return 1;
  else
    return 0;
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
