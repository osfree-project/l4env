/*****************************************************************************/
/**
 * \file   l4ide/src/interface/command.c
 * \brief  L4IDE driver, cmd-interface implementation
 *
 * \date   01/27/2004
 * \author Marek Menzer <mm19@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* standard includes */
#include <stdlib.h>

/* L4/DROPS includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>
#include <l4/thread/thread.h>
#include <l4/generic_blk/blk-server.h>
#include <l4/dde_linux/dde.h>
#include <l4/dm_mem/dm_mem.h>

/* generic_blk includes */
#include <driver/types.h>
#include <driver/command.h>
#include <driver/driver.h>
#include <driver/notification.h>
#include <driver/config.h>
#include <driver/ctrl.h>

#include <linux/ide.h>

#include <l4/util/rdtsc.h>
l4_cpu_time_t s_total;

/*****************************************************************************
 *** helpers
 *****************************************************************************/

struct load_t {
  l4ide_driver_t * driver;
  l4_uint32_t req_handle;
};

static int bio_end_io(struct bio *bio, unsigned int bytes_done, int err)
{
  struct load_t * load = bio->bi_private;
  struct bio_vec * bv;
  int i, ret;
//l4_cpu_time_t s_start;

  if (bio->bi_size) return 1;

  bio_for_each_segment(bv, bio, i) {
    if (!l4dm_is_invalid_ds(bv->bv_ds)) {
//s_start=l4_rdtsc();
      ret=l4dm_mem_ds_unlock(&bv->bv_ds,bv->bv_ds_offs,bv->bv_ds_size);
      if (ret) LOG_Error("failed to unlock dataspace: %s (%d)", l4env_errstr(ret), ret);
//printf("Ulk: %lu ",(unsigned long)(l4_rdtsc()-s_start));
    }
    if (bv->bv_map_addr) {
//s_start=l4_rdtsc();
      ret=l4rm_detach(bv->bv_map_addr);
      if (ret) LOG_Error("failed to detach dataspace: %s (%d)", l4env_errstr(ret), ret);
//printf("Detach: %lu\n",(unsigned long)(l4_rdtsc()-s_start));
    }
  }

  bio_put(bio);

  /* notify client */
  l4ide_do_notification(load->driver, load->req_handle,
                        (err == 0) ? L4BLK_DONE : L4BLK_ERROR, err);
  free(load);

  return 0;
}

/*****************************************************************************/
/**
 * \brief  Command server thread
 * 
 * \param  data          Thread data
 */
/*****************************************************************************/ 
static void
__command_thread(void * data)
{
  CORBA_Server_Environment env = dice_default_server_environment;

  /* setup server environment */
  env.malloc = (dice_malloc_func)malloc;
  env.free = (dice_free_func)free;
  
  l4dde_process_add_worker();

  LOGd(CONFIG_L4IDE_DEBUG_COMMAND, "command interface thread started.");

  /* start server loop */
  l4blk_cmd_server_loop(&env);  
}

/*****************************************************************************
 *** server interface functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Create real-time strean, not supported
 */
/*****************************************************************************/ 
l4_int32_t 
l4blk_cmd_create_stream_component(CORBA_Object _dice_corba_obj,
                                  l4blk_driver_id_t drv,
                                  l4_uint32_t device,
                                  l4_uint32_t bandwidth,
                                  l4_uint32_t period,
                                  l4_uint32_t blk_size,
                                  float q,
                                  l4_uint32_t meta_int,
                                  l4blk_stream_t * stream,
                                  CORBA_Server_Environment * _dice_corba_env)
{
  /* not supported */
  return -L4_ENOTSUPP;
}

/*****************************************************************************/
/**
 * \brief  Close real-time strean, not supported
 */
/*****************************************************************************/ 
l4_int32_t 
l4blk_cmd_close_stream_component(CORBA_Object _dice_corba_obj,
                                 l4blk_driver_id_t drv,
                                 l4blk_stream_t stream,
                                 CORBA_Server_Environment * _dice_corba_env)
{
  /* not supported */
  return -L4_ENOTSUPP;
}

/*****************************************************************************/
/**
 * \brief  Start real-time strean, not supported
 */
/*****************************************************************************/ 
l4_int32_t 
l4blk_cmd_start_stream_component(CORBA_Object _dice_corba_obj,
                                 l4blk_driver_id_t drv,
                                 l4blk_stream_t stream,
                                 l4_uint32_t time,
                                 l4_uint32_t request_no,
                                 CORBA_Server_Environment * _dice_corba_env)
{
  /* not supported */
  return -L4_ENOTSUPP;
}

/*****************************************************************************/
/**
 * \brief  Enqueue request
 * 
 * \param  _dice_corba_obj    Request source
 * \param  drv                Driver id
 * \param  request            Request descriptor 
 * \param  sg_list            Scatter-gather list
 * \param  sg_size            Size of scatter-gather list
 * \param  sg_num             Number of elements in scatter-gather list
 * \param  sg_type            Scatter-gather list type:
 *                            - #L4BLK_SG_PHYS
 *                            - #L4BLK_SG_DS
 * \param  _dice_corba_env    Server environment
 *	
 * \return 0 on success, error code otherwise
 */
/*****************************************************************************/ 
l4_int32_t
l4blk_cmd_put_request_component(CORBA_Object _dice_corba_obj,
                                l4blk_driver_id_t drv,
                                const l4blk_blk_request_t *request,
                                const void *sg_list,
                                l4_int32_t sg_size,
                                l4_int32_t sg_num,
                                l4_int32_t sg_type,
                                CORBA_Server_Environment *_dice_corba_env)
{
  int i, ret;
  struct bio * bio;
  struct load_t * load = malloc(sizeof(struct load_t));
  l4ide_driver_t * driver = l4ide_get_driver(drv);
//l4_cpu_time_t s_start;

  if (load == NULL)
    return -L4_ENOMEM;

  if (driver == NULL) {
    free(load);
    return -L4_EINVAL;
  }
  
  if (!l4_task_equal(*_dice_corba_obj, driver->client)) {
    free(load);
    return -L4_EPERM;
  }

  /* sanity checks */
  if ((sg_num == 0) || (sg_list == NULL)) {
    free(load);
    return -L4_EINVAL;
  }

  load->req_handle = request->req_handle;
  load->driver = driver;

  bio = bio_alloc(GFP_NOIO, sg_num); // Ich kann nicht davon ausgehen, daß die Teile contiguous sind
  bio->bi_sector = request->block<<1; // Only valid for 512B sector devices
  bio->bi_bdev = bdget(request->device, 0);
  if (bio->bi_bdev == NULL) {
    free(load);
    bio_put(bio);
    return -L4_EINVAL;
  }
  bio->bi_vcnt = sg_num;
  bio->bi_idx = 0;
  bio->bi_size = request->count * L4BLK_BLKSIZE; // Das kann zu viel sein
  bio->bi_end_io = bio_end_io;
  bio->bi_private = load;

  if (sg_type == L4BLK_SG_PHYS) {
    for (i=0;i<sg_num;i++) {
      bio->bi_io_vec[i].bv_addr = (void *)((l4blk_sg_phys_elem_t *)sg_list)[i].addr;
      bio->bi_io_vec[i].bv_len = ((l4blk_sg_phys_elem_t *)sg_list)[i].size;
      bio->bi_io_vec[i].bv_offset = 0;
      bio->bi_io_vec[i].bv_ds = L4DM_INVALID_DATASPACE; 
      bio->bi_io_vec[i].bv_map_addr = 0; // no mapping actually
    }
  } else if (sg_type == L4BLK_SG_DS) {
    for (i=0;i<sg_num;i++) {
      bio->bi_io_vec[i].bv_offset = 0;
      bio->bi_io_vec[i].bv_map_addr = 0; // no mapping actually
//s_total=0;
//s_start=l4_rdtsc();
      ret=l4dm_share(&((l4blk_sg_ds_elem_t *)sg_list)[i].ds,
                     bio->bi_bdev->bd_disk->queue->irq_thread_id, L4DM_RW);
      if (ret) LOG_Error("failed to share dataspace: %s (%d)", l4env_errstr(ret), ret);
      // We have to lock this dataspace. It is unlocked on completion.
//printf("Shr: %lu ",(unsigned long)(l4_rdtsc()-s_start));
//s_start=l4_rdtsc();
      ret=l4dm_mem_ds_lock(&((l4blk_sg_ds_elem_t *)sg_list)[i].ds,
                           ((l4blk_sg_ds_elem_t *)sg_list)[i].offs,
			   ((l4blk_sg_ds_elem_t *)sg_list)[i].size);
      if (ret) LOG_Error("failed to lock dataspace: %s (%d)", l4env_errstr(ret), ret);
      // Get the physical address of this dataspace.
//printf("Lk: %lu ",(unsigned long)(l4_rdtsc()-s_start));
//s_start=l4_rdtsc();
      ret = l4dm_mem_ds_phys_addr(&((l4blk_sg_ds_elem_t *)sg_list)[i].ds,
                                  ((l4blk_sg_ds_elem_t *)sg_list)[i].offs,
			          ((l4blk_sg_ds_elem_t *)sg_list)[i].size,
			          (l4_addr_t *)&bio->bi_io_vec[i].bv_addr,
			          (l4_size_t *)&bio->bi_io_vec[i].bv_len);
      if (ret) {
        LOG_Error("getting physical address failed: %s (%d)", l4env_errstr(ret), ret);
      }
//s_total+=l4_rdtsc()-s_start;
//printf("Phys: %lu ",(unsigned long)(l4_rdtsc()-s_start));
      // If the size of the physical part is smaller than of the virtual part,
      // then this is not contiguous. We could work around this, but it would
      // take twice the time.
      if (bio->bi_io_vec[i].bv_len < ((l4blk_sg_ds_elem_t *)sg_list)[i].size)
        LOG_Error("Dataspace element is not contiguous");
      // In case of PIO we need this information in order to get a mapping.
      bio->bi_io_vec[i].bv_ds = ((l4blk_sg_ds_elem_t *)sg_list)[i].ds;
      bio->bi_io_vec[i].bv_ds_offs = ((l4blk_sg_ds_elem_t *)sg_list)[i].offs;
      bio->bi_io_vec[i].bv_ds_size = ((l4blk_sg_ds_elem_t *)sg_list)[i].size;
    }
  } else {
    free(load);
    bio_put(bio);
    return -L4_EINVAL;
  }

  submit_bio((request->cmd == L4BLK_REQUEST_WRITE) ? WRITE : READ, bio);

  return 0;
}

/*****************************************************************************/
/**
 * \brief  Driver ctrl
 * 
 * \param  _dice_corba_obj    Request source
 * \param  drv                Driver id
 * \param  command            Ctrl command
 * \param  in_args            Input buffer
 * \param  in_size            Size of input buffer
 * \param  out_size           Max. size of output buffer
 * \retval out_args           Output buffer
 * \retval out_size           Size of output buffer
 * \param  _dice_corba_env    Server environment
 *	
 * \return 0 on success, error code otherwise
 */
/*****************************************************************************/ 
l4_int32_t 
l4blk_cmd_ctrl_component(CORBA_Object _dice_corba_obj,
                         l4blk_driver_id_t drv,
                         l4_uint32_t command,
                         const void * in_args,
                         l4_int32_t in_size,
                         void ** out_args,
                         l4_int32_t * out_size,
                         CORBA_Server_Environment * _dice_corba_env)
{
  struct block_device * bdev;
  l4ide_driver_t * driver = l4ide_get_driver(drv);
  
  if (driver == NULL)
    return -L4_EINVAL;
    
  if (!l4_task_equal(*_dice_corba_obj, driver->client))
    return -L4_EPERM;

  /* default return buffer */
  *out_size = 0;

  switch (command)
    {
    case L4BLK_CTRL_NUM_DISKS:
      /* return number of disks */
      return get_disk_num();

    case L4BLK_CTRL_DISK_SIZE:
      /* return disk size */
      if (in_size != sizeof(int))
        return -L4_EINVAL;
      bdev = bdget(*(int *)in_args, 0);
      if (bdev == NULL)
        return -L4_EINVAL;
      return bdev->bd_size>>10;

    case L4IDE_CTRL_DISK_NAME:
      /* return disk name */
      if (in_size != sizeof(int))
        return -L4_EINVAL;
      return get_disk_name(*(int *)in_args);

    default: 
      return -L4_EINVAL;
    }

  /* done */
  return 0;
}

/*****************************************************************************
 *** internal functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Start command server thread
 * 
 * \return 0 on success, error code if failed
 */
/*****************************************************************************/ 
int
l4ide_start_command_thread(l4ide_driver_t * driver)
{
  l4thread_t t;

  /* start thread */
  t = l4thread_create_long(CONFIG_L4IDE_THREAD_BASE + 2*(driver->handle),
			   __command_thread, "L4IDE_CMD", L4THREAD_INVALID_SP,
			   CONFIG_L4IDE_COMMAND_STACK_SIZE,
                           CONFIG_L4IDE_CMD_PRIO, driver, L4THREAD_CREATE_ASYNC);
  if (t < 0) {
    driver->cmd_th = L4THREAD_INVALID_ID;
    return t;
  } else {
    driver->cmd_th = t;
    return 0;
  }
}

/*****************************************************************************/
/**
 * \brief  Shutdown command server thread
 * 
 * \param  driver  driver descriptor
 */
/*****************************************************************************/ 
void
l4ide_shutdown_command_thread(l4ide_driver_t * driver)
{
  l4thread_shutdown(driver->cmd_th);
}
