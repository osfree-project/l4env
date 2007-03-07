/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_blk/examples/linux/blk_stub.c
 * \brief  L4Linux block device stub, L4Linux version 2.2
 *
 * \date   02/17/2003
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* Linux kernel includes */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/malloc.h>
#include <linux/smp.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/personality.h>
#include <asm/uaccess.h>
#include <asm/pgtable.h>
#include <asm/hardirq.h>
#include <asm/string.h>
#include <asm/io.h>

#include <l4linux/x86/ids.h>
#include <l4linux/x86/l4_thread.h>
#include <l4linux/x86/config.h>

/* L4 / L4env includes */
#ifdef L4API_l4x0
#include <l4/sys/xadaption.h>
#endif
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/names/libnames.h>
#include <l4/rmgr/librmgr.h>
#include <l4/generic_blk/blk.h>
#include <l4/generic_blk/blk-client.h>

/* Stub includes */
#include "linux_emul.h"

/* Module stuff */
MODULE_AUTHOR("Lars Reuther <reuther@os.inf.tu-dresden.de>");
MODULE_DESCRIPTION("DROPS Block Device Driver Stub");

/* Module arguments */
static char * l4blk_name = "L4SCSI";   /* L4 block driver name */
MODULE_PARM(l4blk_name,"s");
MODULE_PARM_DESC(l4blk_name, "L4 block driver name (default 'L4SCSI')");

/*****************************************************************************
 *** Linux block device stuff
 *****************************************************************************/

/* device config, must be defined before blk.h is included */
#define MAJOR_NR              L4BLK_STUB_MAJOR   // see linux/major.h

#define DEVICE_NAME           "l4b"
#define DEVICE_REQUEST        l4blk_stub_request

#define L4BLK_DEV_SHIFT       4
#define L4BLK_MAX_REAL        16
#define L4BLK_PART_MASK       (~((1 << L4BLK_DEV_SHIFT) - 1))
#define DEVICE_NR(device)     (MINOR(device) >> L4BLK_DEV_SHIFT)

#define DEVICE_NO_RANDOM
#define DEVICE_ON(d)
#define DEVICE_OFF(d)

/* now we can include blk.h */
#include <linux/blk.h>
#include <linux/genhd.h>
#include <linux/hdreg.h>

#define L4BLK_SECT_SIZE       512   /* the default sector size */
#define L4BLK_BLK_SIZE        1024  /* the block size of l4blk interface */

#define L4BLK_SECT_TO_BLK(a)  ((a) >> 1)
#define L4BLK_BLK_TO_SECT(a)  ((a) << 1)

/* to allow linux to reassemble requests in the request queue we cannot
 * send all requests immediately to the driver, otherwise we would end up 
 * handling lots of tiny requests */
#define L4BLK_MAX_DRV_REQS    32     /* max. requests at driver */
#define DO_LIMIT_REQS         1

/* max. read-ahead (at block device level), linux default ist 8 sectors, 
 * SCSI disk (sd) uses 120 sectors for scatter gather capable devices */
#define L4BLK_READ_AHEAD      120

/* max. sectors in a single request. No real idea what to use here, linux 
 * default is 128, but bigger seems to be better ;-) and Linux seems to 
 * also use bigger values */
#define L4BLK_MAX_SECTORS     256

/* max. length of scatter gather list, this is the max. we can handle with 
 * the L4 block device interface, the max. number for a device is stored 
 * in max_segments[MAJOR_NR][dev] and is requested from the driver */
#define L4BLK_MAX_SG_LEN      256

/* number of (minor) devices really in use */
#define L4BLK_NUM_DEVICES     (l4blk_num_disks << L4BLK_DEV_SHIFT)

/* test if buffers described by the buffer heads are contiguous in memory */
#define CONTIGUOUS_BUFFERS(X,Y) ((X->b_data+X->b_size) == Y->b_data)

/* device function prototypes */
static int 
l4blk_stub_ioctl(struct inode *, struct file *, unsigned int, unsigned long);

static int 
l4blk_stub_open(struct inode *, struct file *);

static int 
l4blk_stub_release(struct inode *, struct file *);

/* device file operations */
static struct file_operations l4blk_fops = {
  NULL,                 /* llseek */
  block_read,           /* read,  general blk-dev read */
  block_write,          /* write, general blk-dev write */
  NULL,                 /* readdir */
  NULL,                 /* poll */
  l4blk_stub_ioctl,     /* iocltl */
  NULL,                 /* mmap */
  l4blk_stub_open,      /* open */
  NULL,                 /* flush */
  l4blk_stub_release,   /* release */
  block_fsync,          /* fsync */
  NULL,                 /* fasync */
  NULL,                 /* check_media_change */
  NULL,                 /* revalidate */
  NULL                  /* lock */
};

/* disk sizes */
static int * l4blk_sizes = NULL;

/* log. block sizes */
static int * l4blk_blksizes = NULL;

/* hardware sector sizes */
static int * l4blk_hardsizes = NULL;

/* max. sectors in requests */
static int * l4blk_max_sectors = NULL;

/* max. segments (sg list elements) in a single request */
static int * l4blk_max_segments = NULL;

/* generic hd struct */
static struct gendisk l4blk_gendisk = {
  major:       MAJOR_NR,             /* device major number */
  major_name:  DEVICE_NAME,          /* device name */
  minor_shift: L4BLK_DEV_SHIFT,      /* shift to get disk number from 
                                      * minor number */
  max_p:       1 << L4BLK_DEV_SHIFT, /* max. number of partitions */
  max_nr:      L4BLK_MAX_REAL        /* max. number of real disks */
};

/* partitions */
static struct hd_struct * l4blk_partitions = NULL;

/*****************************************************************************
 *** L4 block driver 
 *****************************************************************************/
  
/* driver thread ids */
static l4_threadid_t l4blk_drv_id;      /* main service thread */
static l4_threadid_t l4blk_cmd_id;      /* command thread */
static l4_threadid_t l4blk_notify_id;   /* request done notification thread */

/* driver handle */
static l4blk_driver_id_t l4blk_drv_handle = -1;

/* number of disks */
static int l4blk_num_disks = 0;

/* IRQ of host adapter */
static int l4blk_irq = 0;

/*****************************************************************************
 *** Linux stub stuff
 *****************************************************************************/

/* threads */
#define L4BLK_NOTIFY_THREAD       41     /* see include/l4linux/x86/config.h */

static l4_threadid_t l4blk_notify_thread;
static void * l4blk_notify_stack;

/* IRQ thread priorities, defined in arch/l4/x86/kernel/irq.c */
extern char irq_prio[];

/* things we need to build a pseudo interrupt thread */
extern void irq_enter(int cpu, unsigned int irq);
extern void irq_exit(int cpu, unsigned int irq);
extern void execute_bottom_halves(int irq);

/* stub request queue */
typedef struct l4blk_req
{
  int              handle;  /* L4blk request handle */
  struct request * req;     /* Linux request */
} l4blk_req_t;

#define L4BLK_NUM_REQUESTS 256

static l4blk_req_t l4blk_requests[L4BLK_NUM_REQUESTS];

static int l4blk_next_request = 0;

/* number of requests currently handled by the L4 driver */
static int l4blk_req_at_driver = 0;

/*****************************************************************************
 *** Debug stuff
 *****************************************************************************/

static void
__msg(const char * tag, int line, const char * fn, const char * format, ...)
{
  va_list args;
  static char buf[256];
  char * f = strrchr(__FILE__, '/');

  printk("%sl4blk [%s:%d] %s: ", tag, f != NULL ? ++f : __FILE__, line, fn);
  va_start(args,format);
  vsprintf(buf,format,args);
  va_end(args);
  printk(buf);
  printk("\n");
}
 
#define Error(format...)   __msg(KERN_ERR,__LINE__,__FUNCTION__, ##format)

#ifdef DEBUG
#  define INFO(format...)  __msg(KERN_INFO,__LINE__,__FUNCTION__, ##format)
#  define INFOd(doit,format...) if (doit) INFO(format)
#else
#  define INFO(format...)
#  define INFOd(doit,format...)
#endif

#ifndef l4util_idstr
#  define l4util_idfmt       "%x.%x"
#  define l4util_idstr(tid)  (tid).id.task,(tid).id.lthread
#endif

#define DEBUG_INIT      1
#define DEBUG_OPEN      1
#define DEBUG_IOCTL     1
#define DEBUG_REQUEST   0
#define DEBUG_NOTIFY    0
#define DEBUG_FINISH    0
#define DEBUG_BLOCK     0

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Calculate absolute block number
 * 
 * \param  dev           Device
 * \param  sector        Block number
 *	
 * \return Block number
 */
/*****************************************************************************/ 
static inline unsigned long
__get_block_number(kdev_t dev, unsigned long sector)
{
  int minor = MINOR(dev);
  unsigned long block = sector;

  INFOd(DEBUG_BLOCK, "device 0x%x, minor 0x%x \n" \
        "  sector %lu, start_sect %lu", dev, minor, sector,
        l4blk_partitions[minor].start_sect);

  /* add partition offset */
  block += l4blk_partitions[minor].start_sect;

  /* make 1KB block */
  block >>= 1;

  INFOd(DEBUG_BLOCK, "block %lu", block);

  return block;
}

/*****************************************************************************/
/**
 * \brief  Do l4blk_ctrl
 *	
 * \return l4blk_ctrl restult, -1 if call failed
 */
/*****************************************************************************/ 
static int
__l4blk_ctrl(unsigned int cmd, unsigned long arg, void * ret_buf, int ret_len)
{
  int ret;
  CORBA_Environment _env = dice_default_environment;
  
  /* do blk_ctrl */
  ret = l4blk_cmd_ctrl_call(&l4blk_cmd_id, l4blk_drv_handle,
                            cmd, &arg, sizeof(unsigned long), 
                            &ret_buf, &ret_len, &_env);
  if (DICE_HAS_EXCEPTION(&_env))
    {
      Error("calling block driver failed (exc %d)!", DICE_EXCEPTION_MAJOR(&_env));
      return -1;
    }
  
  /* done */
  return ret;
}

/*****************************************************************************/
/**
 * \brief  Finish request
 * 
 * \param  req           Request descriptor 
 * \param  uptodate      Buffer successfully read/written, 0 if error
 *
 * Because we execute a whole request at once, We do have our own end_request
 * which is much simpled than end_that_request_first/last
 */
/*****************************************************************************/ 
static void
__finish_request(l4blk_req_t * req, int uptodate)
{
  struct request * r = req->req;
  struct buffer_head * bh;

  if (r == NULL)
    panic("corrupted request list");

  INFOd(DEBUG_FINISH,
        "request at %p, device 0x%x\n" \
        "  cmd % d, sector %lu, %lu sectors, uptodate %d",
        r, r->rq_dev, r->cmd, r->sector, r->nr_sectors, uptodate);
  
  /* error? */
  r->errors = 0;
  if (!uptodate)
    Error("I/O error, dev %s, sector %lu", kdevname(r->rq_dev), r->sector);
  
  /* mark all buffers uptodate */
  bh = r->bh;
  while (r->bh)
    {
      bh = r->bh;
      r->bh = r->bh->b_reqnext;
      bh->b_reqnext = NULL;
      bh->b_end_io(bh, uptodate);
    }
  
  /* release Linux request */
  if (r->sem != NULL)
    up(r->sem);
  r->rq_status = RQ_INACTIVE;
  wake_up(&wait_for_request);
  
  /* release stub request */
  req->req = NULL;
}

/*****************************************************************************/
/**
 * \brief Send request to block driver 
 */
/*****************************************************************************/ 
static void
__l4blk_do_request(void)
{
  struct request * req;
  l4blk_req_t * r;
  int i,ret,found,num;
  struct buffer_head * bh, * bhp;
  l4blk_sg_phys_elem_t * sg_list;
  l4blk_blk_request_t blk_req;
  CORBA_Environment _env = dice_default_environment;
  
#if DO_LIMIT_REQS
  while (l4blk_req_at_driver < L4BLK_MAX_DRV_REQS)
#else
  while (1)
#endif
    {
      req = CURRENT;
      
      /* sanity checks */
      if (!req)
        /* nothing to do */
        return;
      
      /* remove request from device request queue */
      CURRENT = req->next;
      req->next = NULL;
      
      if (MAJOR(req->rq_dev) != MAJOR_NR)
        {
          if (MAJOR(req->rq_dev) != 0)
            /* ugly: Linux seems to have a problem with locking, FIXME!!! */
            Error(l4util_idfmt": invalid device, is 0x%x, should be 0x%x",
                  l4util_idstr(l4_myself()), MAJOR(req->rq_dev), MAJOR_NR);
          
          continue;
        }
      
      if ((req->bh) && !(buffer_locked(req->bh)))
        panic("l4blk: buffer not locked");
      
      /* allocate stub request descriptor */
      i = l4blk_next_request;
      found = 0;
      do
        {
          if (l4blk_requests[i].req == NULL)
            found = 1;
          else
            i = (i + 1) % L4BLK_NUM_REQUESTS;
        }
      while (!found && (i != l4blk_next_request));
      
      if (!found)
        {
          /* no descriptor found, this should actually never happen, the number
           * of outstanding I/O requests is limited. Nevertheless we should
           * handle this more intelligently (wait until a request becomes free)
           */
          panic("l4blk: no request descriptor found");
        }
      
      r = &l4blk_requests[i];
      r->handle = i;
      r->req = req;
      l4blk_next_request = (i + 1) % L4BLK_NUM_REQUESTS;
      
      /* setup scatter-gather list */
      if (req->nr_sectors != req->current_nr_sectors)
        {
          /* clustered request  */
          INFOd(DEBUG_REQUEST, "\n" \
                "  request at %p, device 0x%x, %s, sector %lu, nr_sectors %lu\n" \
                "  first buffer at %p, size %u",
                req, req->rq_dev, (req->cmd == READ) ? "read" : "write",
                req->sector, req->nr_sectors, req->bh->b_data, req->bh->b_size);
          
          /* sanity checks */
          if (!req->bh)
            {
              Error("no buffer list for clustered request!");
              goto req_error_req;
            }
          
          /* allocate sg list */
          sg_list = kmalloc((L4BLK_MAX_SG_LEN * sizeof(l4blk_sg_phys_elem_t)),
                            GFP_ATOMIC);
          if (sg_list == NULL)
            {
              Error("scatter gather list allocation failed!");
              goto req_error_req;
            }
          
          /* setup sg list */
          bh = req->bh;
          bhp = NULL;
          num = 0;
          while (bh)
            {
              if (!buffer_locked(bh))
                {
                  Error("buffer not locked!");
                  goto req_error_mem;
                }
              
              if ((bhp != NULL) && (CONTIGUOUS_BUFFERS(bhp,bh)))
                sg_list[num - 1].size += bh->b_size;
              else
                {
                  num++;
                  sg_list[num - 1].addr = virt_to_phys(bh->b_data);
                  sg_list[num - 1].size = bh->b_size;
                }
              
              bhp = bh;
              bh = bh->b_reqnext;
            }
          
#if 0
          INFO("SG list: ");
          for (i = 0; i < num; i++)
            printk("  buffer at 0x%08x, size %lu\n", sg_list[i].addr,
                   sg_list[i].size);
#endif
        }                   
      else
        {
          /* single buffer request */
          INFOd(DEBUG_REQUEST, "\n" \
                "  request at %p, device 0x%x, %s\n" \
                "  sector %lu, nr_sectors %lu, current_nr_sectors %lu\n" \
                "  buffer at %p, size %lu", 
                req, req->rq_dev, (req->cmd == READ) ? "read" : "write",
                req->sector, req->nr_sectors, req->current_nr_sectors,
                req->bh->b_data, req->bh->b_size);

          /* allocate single sg list element */
          sg_list = kmalloc(sizeof(l4blk_sg_phys_elem_t), GFP_ATOMIC);
          if (sg_list == NULL)
            {
              Error("scatter gather list allocation failed!");
              goto req_error_req;
            }

          /* setup sg list */
          sg_list->addr = virt_to_phys(req->buffer);
          sg_list->size = req->nr_sectors * L4BLK_SECT_SIZE;
          num = 1;
        }

      /* setup request */
      blk_req.req_handle = r->handle;
      switch (req->cmd)
        {
        case READ:
          blk_req.cmd = L4BLK_REQUEST_READ;
          break;
        case WRITE:
          blk_req.cmd = L4BLK_REQUEST_WRITE;
          break;
        default:
          /* invalid request */
          Error("invalid request command %d", req->cmd);
          goto req_error_mem;
          continue;
        }
      blk_req.device = MINOR(req->rq_dev) & L4BLK_PART_MASK;
      blk_req.block = __get_block_number(req->rq_dev, req->sector);
      blk_req.count = L4BLK_SECT_TO_BLK(req->nr_sectors);
      blk_req.stream = L4BLK_INVALID_STREAM;
      blk_req.req_no = 0;
      blk_req.flags = 0;
      
      /* send request to driver */
      ret = l4blk_cmd_put_request_call(&l4blk_cmd_id, l4blk_drv_handle,
                                       &blk_req, sg_list, 
                                       num * sizeof(l4blk_sg_phys_elem_t), 
                                       num, L4BLK_SG_PHYS, &_env);
      if ((ret < 0) || DICE_HAS_EXCEPTION(&_env))
        {
          Error("send request to driver failed (ret %d, exc %d)",
                ret, DICE_EXCEPTION_MAJOR(&_env));
          goto req_error_mem;
        }
      
      l4blk_req_at_driver++;
      
      /* release sg list */
      kfree(sg_list);
          
      /* done */
      goto req_done;
          
    req_error_mem:
      kfree(sg_list);
      
    req_error_req:
      __finish_request(r,0);
      
    req_done:
    }
}

/*****************************************************************************/
/**
 * \brief  Pseudo interrupt thread, wait for notifiacation from block driver
 */
/*****************************************************************************/ 
static void
__l4blk_notify_thread(void)
{
  int ret,status,error;
  unsigned int handle;
  unsigned long flags;
  CORBA_Environment _env = dice_default_environment;
  l4_threadid_t id = get_l4_id_from_stack();

  INFOd(DEBUG_INIT, "started notification thread at "l4util_idfmt, l4util_idstr(id));
  
  while (1)
    {
      /* wait for request to be done */
      ret = l4blk_notify_wait_call(&l4blk_notify_id, l4blk_drv_handle,
                                   &handle, &status, &error, &_env);

      /* we must disable the interrupts, otherwise we might get in conflict
       * with real interrupts 
       */
      cli();

      INFOd(DEBUG_NOTIFY, "\n" \
            "  ret %d, handle %u,status %d, error %d",
            ret, handle, status, error);
      
      /* we are handling an 'interrupt', tell the rest of the kernel */ 
      irq_enter(smp_processor_id(), l4blk_irq);
      
      if ((ret < 0) || DICE_HAS_EXCEPTION(&_env))
        {
          Error("error waiting for notification (ret %d,  exc %d)",
                ret, DICE_EXCEPTION_MAJOR(&_env));
          goto notify_error;
        }
      
      if ((handle >= L4BLK_NUM_REQUESTS) || 
          (l4blk_requests[handle].req == NULL))
        {
          Error("received notification for invalid/unused request (%u)",
                handle);
          goto notify_error;
        }
      
      INFOd(DEBUG_NOTIFY, "request finished, statux 0x%02x, error 0x%x",
            status, error);
      
      l4blk_req_at_driver--;
      
      if (status == L4BLK_DONE)
        /* request finished successfully */
        __finish_request(&l4blk_requests[handle], 1);
      else
        {
          Error("request error (status 0x%02x, error 0x%02x)!", status, error);
          __finish_request(&l4blk_requests[handle], 0);
        }
      
    notify_error:
      
      /* finished handling the 'interrupt' */
      irq_exit(smp_processor_id(), l4blk_irq);
      
#if DO_LIMIT_REQS
      /* done with an old request, more requests to do? */
      spin_lock_irqsave(&io_request_lock, flags);
      
      __l4blk_do_request();
      
      spin_unlock_irqrestore(&io_request_lock, flags);
#endif
      
      /* we do not have a bh, but this also wakes up the idle thread which 
       * takes care of really waking up the user process which waited for the
       * current request */
      execute_bottom_halves(l4blk_irq);

      /* enable interrupts again */
      sti();
    }
}

/*****************************************************************************/
/**
 * \brief  Revalidate disk, adapted from sd.c
 * 
 * \param  dev           Device
 * 
 * \retval 0 on succes, errorcode if failed
 */
/*****************************************************************************/ 
static int
__revalidate_disk(kdev_t dev)
{
  int target,max_p,start,i,index,size,ret;
  struct super_block * sb;
  kdev_t devi;
  
  target = DEVICE_NR(dev);
  if (target >= l4blk_num_disks)
    return -ENODEV;
  
  /* first, flush buffers and such stuff */
  max_p = l4blk_gendisk.max_p;
  start = target << l4blk_gendisk.minor_shift;
  for (i = max_p - 1; i >= 0; i--)
    {
      index = start + i;
      devi = MKDEV(MAJOR_NR, index);
      sb = get_super(devi);
      
      if (sb) 
        invalidate_inodes(sb);
      invalidate_buffers(devi);
      
      l4blk_gendisk.part[index].start_sect = 0;
      l4blk_gendisk.part[index].nr_sects = 0;
    }
  
  /* reset disk size */
  size = __l4blk_ctrl(L4BLK_CTRL_DISK_SIZE, target, NULL, 0);
  if (size < 0)
    {
      Error("get disk size failed: %d", size);
      return -EIO;
    }
  l4blk_partitions[start].nr_sects = L4BLK_BLK_TO_SECT(size);
  
  /* reread at driver */
  ret = __l4blk_ctrl(L4BLK_CTRL_RREAD_PART, MINOR(dev), NULL, 0);
  if (ret < 0)
    {
      Error("reread partition table failed: %d", ret);
      return -EIO;
    }
  
  /* last, reread Linux partition table */
  resetup_one_dev(&l4blk_gendisk, target);
  
  /* done */
  return 0;
}

/*****************************************************************************
 *** Block device functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Open device
 * 
 * \param  inode         Device inode
 * \param  filp          Device file pointer
 *	
 * \return 0 on success, error if open device failed.
 */
/*****************************************************************************/ 
static int 
l4blk_stub_open(struct inode * inode, struct file * filp)
{
  int dev = inode->i_rdev;
  int disk = DEVICE_NR(dev);
  
  INFOd(DEBUG_OPEN, "l4blk_stub_open called\n" \
        "  major 0x%x, minor 0x%x, device nr %d",
        MAJOR(dev), MINOR(dev), DEVICE_NR(dev));
  
  if (disk >= l4blk_num_disks)
    {
      Error("invalid disk %d (dev 0x%02x)!", disk, MINOR(dev));
      return -ENODEV;
    }
  
  /* increment module use count */
  MOD_INC_USE_COUNT;
  
  /* nothing else to do */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Release device
 * 
 * \param  inode         Device inode
 * \param  filp          Device file pointer
 *	
 * \return 0 on success
 */
/*****************************************************************************/ 
static int 
l4blk_stub_release(struct inode * inode, struct file * filp)
{
  INFOd(DEBUG_OPEN, "l4blk_stub_release called.");
  
  /* decrement module use count */
  MOD_DEC_USE_COUNT;
  
  /* nothing else to do until now */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Device ioctl
 * 
 * \param  inode         Device inode
 * \param  filp          Device file pointer
 * \param  cmd           Ioctl command
 * \param  arg           Ioctl argument
 *
 * Used sd_ioctl as template.
 */
/*****************************************************************************/ 
static int 
l4blk_stub_ioctl(struct inode * inode, struct file * filp, 
                 unsigned int cmd , unsigned long arg)
{
  int dev = inode->i_rdev;
  int ret;
  l4blk_disk_geometry_t geom;
  struct hd_geometry * loc = (struct hd_geometry *) arg;
  
  INFOd(DEBUG_IOCTL, "l4blk_stub_ioctl called:\n" \
        "  dev 0x%08x, cmd 0x%08x", dev, cmd);
  
  if (MAJOR(dev) != MAJOR_NR)
    return -EINVAL;
  
  if (DEVICE_NR(dev) >= l4blk_num_disks)
    return -ENODEV;
  
  switch (cmd)
    {
    case HDIO_GETGEO:
      /* return BIOS disk parameters */
      ret = __l4blk_ctrl(L4BLK_CTRL_DISK_GEOM, MINOR(dev), &geom,
                         sizeof(l4blk_disk_geometry_t));
      if (ret < 0)
        {
          Error("get disk geometry failed: %d", ret);
          return -EIO;
        }
      else
        {
          INFOd(DEBUG_IOCTL, "disk geometry:\n" \
                "  %u heads, %u cylinders, %u sectors, partition offset %u",
                geom.heads, geom.cylinders, geom.sectors, geom.start);
          
          ret = put_user(geom.heads, &loc->heads);
          ret |= put_user(geom.sectors, &loc->sectors);
          ret |= put_user(geom.cylinders, &loc->cylinders);
          ret |= put_user(geom.start, &loc->start);
          
          return ret;
        }
      
    case BLKGETSIZE:   
      /* return device size (sectors) */
      ret = __l4blk_ctrl(L4BLK_CTRL_DISK_SIZE, MINOR(dev), NULL, 0);
      if (ret < 0)
        {
          Error("get disk size failed: %d", ret);	  
          return -EIO;
        }
      INFOd(DEBUG_IOCTL, "disk size: %d KB (%d MB)", ret, ret >> 10);

      return put_user(L4BLK_BLK_TO_SECT(ret), (long *)arg);

    case BLKRASET:
      /* set read-ahead value for device */
      if (!capable(CAP_SYS_ADMIN))
        return -EACCES;

      if(arg > 0xff) 
        return -EINVAL;

      INFOd(DEBUG_IOCTL, "set read-ahead to %lu", arg);
      read_ahead[MAJOR(inode->i_rdev)] = arg;

      return 0;
      
    case BLKRAGET:
      /* return read-ahead value for device */
      return put_user(read_ahead[MAJOR(inode->i_rdev)], (long *) arg);

    case BLKFLSBUF:
      /* flush buffers */
      if(!capable(CAP_SYS_ADMIN))  
        return -EACCES;

      fsync_dev(inode->i_rdev);
      invalidate_buffers(inode->i_rdev);
      
      return 0;
	
    case BLKRRPART: 
      /* re-read partition tables */
      if (!capable(CAP_SYS_ADMIN))
        return -EACCES;

      INFOd(DEBUG_IOCTL, "reread partition table");

      return __revalidate_disk(inode->i_rdev);

    case BLKSSZGET:
      /* get block size of media */
      return put_user(blksize_size[MAJOR(dev)][MINOR(dev)&0x0F], (int *)arg);
      
    case BLKELVGET:
    case BLKELVSET:
      /* elevator sorting, default implementation */
      return blkelv_ioctl(inode->i_rdev, cmd, arg);
      
      /* get/set read-only */
      RO_IOCTLS(dev, arg);
    }

  return -EINVAL;
}

/*****************************************************************************/
/**
 * \brief  Device request function
 */
/*****************************************************************************/ 
static void
l4blk_stub_request(void)
{  
  INFOd(DEBUG_REQUEST, "l4blk_stub_request called.");

  /* start processing request queue */
  __l4blk_do_request();
}

/*****************************************************************************
 *** Module initialization
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Init module
 *	
 * \return 0 if initialization failed, != 0 otherwise
 */
/*****************************************************************************/ 
static int
l4blk_stub_init(void)
{
  int ret, dev, i;
  CORBA_Environment _env = dice_default_environment;
  unsigned int * sp;
  l4_threadid_t tid; 

  INFOd(DEBUG_INIT, "init module");

  /* find L4 block driver */
  ret = names_waitfor_name(l4blk_name, &l4blk_drv_id, 500);
  if (!ret)
    {
      Error("L4 block driver (\'%s\') not found!", l4blk_name);
      return -ENODEV;
    }

  INFOd(DEBUG_INIT, "found L4 block driver \'%s\' at "l4util_idfmt, 
        l4blk_name, l4util_idstr(l4blk_drv_id));

  /* open block driver */
  ret = l4blk_driver_open_call(&l4blk_drv_id, &l4blk_drv_handle,
                               &l4blk_cmd_id, &l4blk_notify_id, &_env);
  if ((ret < 0) || DICE_HAS_EXCEPTION(&_env))
    {
      Error("open L4 block driver failed (ret %d, exc %d)", ret, 
	  DICE_EXCEPTION_MAJOR(&_env));
      return -ENODEV;
    }

  INFOd(DEBUG_INIT, "driver handle %d, cmd at "l4util_idfmt", notify at "l4util_idfmt,
        l4blk_drv_handle, l4util_idstr(l4blk_cmd_id), l4util_idstr(l4blk_notify_id));

  /* get number of disks */
  ret = -ENODEV;
  l4blk_num_disks = __l4blk_ctrl(L4BLK_CTRL_NUM_DISKS, 0, NULL, 0);
  if (l4blk_num_disks < 0)
    {
      Error("get number of disks failed: %d!", l4blk_num_disks);
      goto init_error_drv;
    }
  
  INFOd(DEBUG_INIT, "got %d disk(s)", l4blk_num_disks);

  if (l4blk_num_disks == 0)
    {
      Error("no disks found, exiting...\n");
      goto init_error_drv;
    }

  /* get the IRQ number of the host adapter the first disk is connected to,
   * we need this to pretend that our notification thread is a Linux 
   * interrupt thread */
  l4blk_irq = __l4blk_ctrl(L4BLK_CTRL_DRV_IRQ, 0, NULL, 0);
  if (l4blk_irq < 0)
    {
      Error("failed to get the host adapter IRQ: %d!\n", l4blk_irq);
      goto init_error_drv;
    }

  INFOd(DEBUG_INIT, "using IRQ %d for notify thread, prio %d",
        l4blk_irq, PRIO_IRQ(l4blk_irq));

  /* allocate stack for notification thread */
  tid = l4_myself();
  tid.id.lthread = L4BLK_NOTIFY_THREAD;
  l4blk_notify_stack = l4blk_allocate_stack();

  put_l4_id_to_stack((unsigned)l4blk_notify_stack, tid);
  put_l4_prio_to_stack((unsigned)l4blk_notify_stack, PRIO_IRQ(l4blk_irq));

  /* start notification thread */
  sp = l4blk_notify_stack;
  *(--sp) = 0;			/* fake return address */
  l4blk_notify_thread = 
    create_thread(L4BLK_NOTIFY_THREAD, __l4blk_notify_thread, sp);

  if(l4_is_invalid_id(l4blk_notify_thread))
    {
      Error("create notification thread failed!\n");
      ret = -EINVAL;
      goto init_error_drv;
    }
  
  /* set the priority of the notification thread to the corresponding 
   * interrupt thread priority, so we do somehow fit into the L4Linux
   * priority scheme */
  rmgr_set_prio(l4blk_notify_thread, PRIO_IRQ(l4blk_irq));
  
  /* register block device */
  ret = register_blkdev(MAJOR_NR, DEVICE_NAME, &l4blk_fops);
  if (ret < 0)
    {
      Error("register block device failed (%d)!\n", ret);
      goto init_error_thread;
    }

  /* set request function */
  blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;

  /* setup the tons of arrays linux needs */
  ret = -ENOMEM;
  l4blk_blksizes = kmalloc(L4BLK_NUM_DEVICES * sizeof(int), GFP_KERNEL);
  l4blk_hardsizes = kmalloc(L4BLK_NUM_DEVICES * sizeof(int), GFP_KERNEL);
  l4blk_max_sectors = kmalloc(L4BLK_NUM_DEVICES * sizeof(int), GFP_KERNEL);
  l4blk_max_segments = kmalloc(L4BLK_NUM_DEVICES * sizeof(int), GFP_KERNEL);
  l4blk_sizes = kmalloc(L4BLK_NUM_DEVICES * sizeof(int), GFP_KERNEL);
  l4blk_partitions = 
    kmalloc(L4BLK_NUM_DEVICES * sizeof(struct hd_struct), GFP_KERNEL);

  if ((l4blk_blksizes == NULL) || 
      (l4blk_hardsizes == NULL) || 
      (l4blk_sizes == NULL) ||
      (l4blk_max_sectors == NULL) ||
      (l4blk_partitions == NULL))
    goto init_error_malloc;

  blksize_size[MAJOR_NR] = l4blk_blksizes;
  hardsect_size[MAJOR_NR] = l4blk_hardsizes;
  max_sectors[MAJOR_NR] = l4blk_max_sectors;
  max_segments[MAJOR_NR] = l4blk_max_segments;
  blk_size[MAJOR_NR] = l4blk_sizes;
  l4blk_gendisk.sizes = l4blk_sizes;

  for (i = 0; i < L4BLK_NUM_DEVICES; i++)
    {
      l4blk_blksizes[i] = L4BLK_BLK_SIZE;
      l4blk_hardsizes[i] = L4BLK_SECT_SIZE;  
      l4blk_max_sectors[i] = L4BLK_MAX_SECTORS;
    }

  /* set up disk sizes / sg list lengths */
  memset(l4blk_sizes,0,L4BLK_NUM_DEVICES * sizeof(int));
  for (dev = 0; dev < L4BLK_NUM_DEVICES; dev += (1 << L4BLK_DEV_SHIFT))
    {
      l4blk_sizes[dev] = __l4blk_ctrl(L4BLK_CTRL_DISK_SIZE, dev, NULL, 0);
      if (l4blk_sizes[dev] < 0)
        {
          Error("get disk size failed: %d", l4blk_sizes[dev]);
          ret = -EIO;
          goto init_error_malloc;
        }
      INFOd(DEBUG_INIT, "device 0x%02x: %d KB (%d MB)", dev, l4blk_sizes[dev],
            l4blk_sizes[dev] >> 10);

      l4blk_max_segments[dev] = 
        __l4blk_ctrl(L4BLK_CTRL_MAX_SG_LEN, dev, NULL, 0);
      if (l4blk_max_segments[dev] < 0)
        {
          Error("get max. scatter gather list length failed: %d",
                l4blk_max_segments[dev]);
          ret = -EIO;
          goto init_error_malloc;
        }
      if (l4blk_max_segments[dev] > L4BLK_MAX_SG_LEN)
        l4blk_max_segments[dev] = L4BLK_MAX_SG_LEN;
      INFOd(DEBUG_INIT, "device 0x%02x: max. %d elements in sg list",
            dev, l4blk_max_segments[dev]);

      /* copy to all minors */
      for (i = 1; i < (1 << L4BLK_DEV_SHIFT); i++)
        l4blk_max_segments[dev + i] = l4blk_max_segments[dev];
    }

  /* read-ahead */
  read_ahead[MAJOR_NR] = L4BLK_READ_AHEAD;

  /* setup partitions */
  memset(l4blk_partitions,0,
         (l4blk_num_disks << L4BLK_DEV_SHIFT) * sizeof(struct hd_struct));
  for (dev = 0; 
       dev < (l4blk_num_disks << L4BLK_DEV_SHIFT); 
       dev += (1 << L4BLK_DEV_SHIFT))
    l4blk_partitions[dev].nr_sects = L4BLK_BLK_TO_SECT(l4blk_sizes[dev]);
  l4blk_gendisk.part = l4blk_partitions;
  l4blk_gendisk.nr_real = l4blk_num_disks;

  /* insert to gendisk list */
  l4blk_gendisk.next = gendisk_head;
  gendisk_head = &l4blk_gendisk;
  
  /* setup partitions */
  printk("Partition check:\n");
  for (i = 0; i < l4blk_num_disks; i++)
    resetup_one_dev(&l4blk_gendisk, i);

  /* stub initialization */
  for (i = 0; i < L4BLK_NUM_REQUESTS; i++)
    {
      l4blk_requests[i].req = NULL;
    }

  /* done */
  return 0;

  /* error handling */
 init_error_malloc:
  /* free size arrays */
  if (l4blk_sizes != NULL)
    kfree(l4blk_sizes);
  blk_size[MAJOR_NR] = NULL;

  if (l4blk_blksizes != NULL)
    kfree(l4blk_blksizes);
  blksize_size[MAJOR_NR] = NULL;

  if (l4blk_hardsizes != NULL)
    kfree(l4blk_hardsizes);
  hardsect_size[MAJOR_NR] = NULL;

  if (l4blk_max_sectors != NULL)
    kfree(l4blk_max_sectors);
  max_sectors[MAJOR_NR] = NULL;

  if (l4blk_max_segments != NULL)
    kfree(l4blk_max_segments);
  max_segments[MAJOR_NR] = NULL;

  if (l4blk_partitions != NULL)
    kfree(l4blk_partitions);

  /* unregister device */
  unregister_blkdev(MAJOR_NR, DEVICE_NAME);

 init_error_thread:
  /* shutdown notification thread */
  destroy_thread(L4BLK_NOTIFY_THREAD);
  l4blk_release_stack(l4blk_notify_stack);

 init_error_drv:
  /* close L4 block driver instance */
  l4blk_driver_close_call(&l4blk_drv_id, l4blk_drv_handle, &_env);

  return ret;
}

module_init(l4blk_stub_init);

/*****************************************************************************/
/**
 * \brief Cleanup module
 */
/*****************************************************************************/ 
static void
l4blk_stub_cleanup(void)
{
  int ret, i;
  CORBA_Environment _env = dice_default_environment;
  struct gendisk ** gdp;
  
  INFOd(DEBUG_INIT,"l4blk_stub: cleanup module");
  
  /* flush disks */
  for (i = 0; i < (l4blk_num_disks << L4BLK_DEV_SHIFT); i++)
    fsync_dev(MKDEV(MAJOR_NR, i));

  /* free memory */
  if (l4blk_sizes != NULL)
    kfree(l4blk_sizes);
  blk_size[MAJOR_NR] = NULL;

  if (l4blk_blksizes != NULL)
    kfree(l4blk_blksizes);
  blksize_size[MAJOR_NR] = NULL;

  if (l4blk_hardsizes != NULL)
    kfree(l4blk_hardsizes);
  hardsect_size[MAJOR_NR] = NULL;

  if (l4blk_max_sectors != NULL)
    kfree(l4blk_max_sectors);
  max_sectors[MAJOR_NR] = NULL;

  if (l4blk_max_segments != NULL)
    kfree(l4blk_max_segments);
  max_segments[MAJOR_NR] = NULL;

  kfree(l4blk_partitions);

  /* remove gendisk struct from list */
  for (gdp = &gendisk_head; *gdp; gdp = &((*gdp)->next))
    {
      if (*gdp == &l4blk_gendisk)
        {
          *gdp = (*gdp)->next;
          break;
        }
    }

  /* unregister block device */
  unregister_blkdev(MAJOR_NR, DEVICE_NAME);

  /* destroy notification thread */
  destroy_thread(L4BLK_NOTIFY_THREAD);
  l4blk_release_stack(l4blk_notify_stack);
  
  /* close L4 block driver instance */
  if (l4blk_drv_handle != -1)
    {
      ret = l4blk_driver_close_call(&l4blk_drv_id, l4blk_drv_handle, &_env);
      if ((ret < 0) || DICE_HAS_EXCEPTION(&_env))
        {
          Error("close L4 block driver failed (ret %d, exc %d)",
                ret, DICE_EXCEPTION_MAJOR(&_env));
          return;
        }
    }
}

module_exit(l4blk_stub_cleanup);

