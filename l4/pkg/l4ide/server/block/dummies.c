#include <linux/blkdev.h>
#include <linux/device.h>
#include <linux/ctype.h>
#include <l4/util/macros.h>
#include <l4/l4rm/l4rm.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <linux/bio.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kmod.h>
#include <linux/list.h>
#include <linux/seq_file.h>
#include <linux/sysctl.h>
#include <driver/config.h>

#define DEBUG_MAP_DS 0

const unsigned char scsi_command_size[8] =
{
  6, 10, 10, 12,
  16, 12, 10, 10
};

static ctl_table root_table[];
static struct ctl_table_header root_table_header =
	{ root_table, LIST_HEAD_INIT(root_table_header.ctl_entry) };
	
static ctl_table root_table[] = {
/*	{
		.ctl_name = CTL_DEV,
		.procname = "dev",
		.mode = 0555,
		.child = dev_table,
	},
*/	{ .ctl_name = 0 }
};

struct ctl_table_header *register_sysctl_table(ctl_table * table, int insert_at_head)
{
	struct ctl_table_header *tmp;
	tmp = kmalloc(sizeof(struct ctl_table_header), GFP_KERNEL);
	if (!tmp)
		return NULL;
	tmp->ctl_table = table;
	INIT_LIST_HEAD(&tmp->ctl_entry);
	if (insert_at_head)
		list_add(&tmp->ctl_entry, &root_table_header.ctl_entry);
	else
		list_add_tail(&tmp->ctl_entry, &root_table_header.ctl_entry);
		
	return tmp;
}

void unregister_sysctl_table(struct ctl_table_header * header)
{
	list_del(&header->ctl_entry);
	kfree(header);
}

#include <l4/util/rdtsc.h>
extern l4_cpu_time_t s_total;
void *bio_data(struct bio *bio)
{
  struct bio_vec * bv = bio_iovec(bio);
  int retval;
//l4_cpu_time_t s_start;

  /* We shall return this bio's actual bio_vec's VAddress */
  if (!l4dm_is_invalid_ds(bv->bv_ds)) {
//s_start=l4_rdtsc();
    retval = l4rm_attach(&bv->bv_ds, bv->bv_ds_size, bv->bv_ds_offs,
                         L4DM_RW | L4RM_MAP, &bv->bv_map_addr);
    if (retval < 0) Panic("attach buffer ds failed: %s (%d)", l4env_errstr(retval), retval);
//printf("Atch: %lu ",(unsigned long)(l4_rdtsc()-s_start));

    LOGdL(DEBUG_MAP_DS, "mapped ds %d at "l4util_idfmt" to %p, size %u", 
          bv->bv_ds.id, l4util_idstr(bv->bv_ds.manager), 
          bv->bv_map_addr, bv->bv_ds_size);

//s_total+=l4_rdtsc()-s_start;
    return bv->bv_map_addr;
  }
  return NULL;
}

void blk_queue_bounce(request_queue_t *q, struct bio **bio)
{
    struct bio_vec *from;
    int i;
    
    bio_for_each_segment(from, *bio, i) {
	if (((unsigned long)from->bv_addr >> PAGE_SHIFT) < q->bounce_pfn)
	    continue;
	Panic("We shall bounce, but it isn't implemented yet!\n");
    }
}

void blk_queue_end_tag(request_queue_t *q, struct request *rq)
{
    LOG("Fixme blk_queue_end_tag");
}

int blk_queue_start_tag(request_queue_t *q, struct request *rq)
{
    LOG("Fixme blk_queue_start_tag");
    return 0;
}

int seq_printf(struct seq_file *m, const char *f, ...)
{
    LOG("Fixme seq_printf");
    return 0;
}

int blkdev_ioctl(struct inode *inode, struct file *file, unsigned cmd, unsigned long arg)
{
    LOG("Fixme blkdev_ioctl");
    // eigentlich kein problem
    return 0;
}

int scsi_cmd_ioctl(struct gendisk *disk, unsigned int cmd, unsigned long arg)
{
    LOG("Fixme scsi_cmd_ioctl");
    // steht in block/scsi_ioctl.c und ist eigentlich ganz einfach
    return 0;
}

int max_pfn=16384;
int max_low_pfn=16384;

static LIST_HEAD(all_bdevs);
static spinlock_t bdev_lock __cacheline_aligned_in_smp = SPIN_LOCK_UNLOCKED;

int get_disk_num(void)
{
    int num = 0;
    struct list_head *p;
    
    list_for_each(p,&all_bdevs) {
	num++;
    }
    
    return num;
}

int get_disk_name(int number)
{
    int num = 0;
    struct list_head *p;
    struct block_device *bdev;
    
    list_for_each_prev(p,&all_bdevs) {
        bdev = list_entry(p, struct block_device, bd_list);
        if (num++ == number) {
    	    return (int)(bdev->bd_dev);
	}
    }
    return 0;
}

int __init init_bdevs(void)
{
	return 0;
}

module_init(init_bdevs);

void invalidate_bdev(struct block_device *bdev, int destroy_dirty_buffers)
{
    /* We don't do anything here, because we don't maintain any block cache */
    LOGd(CONFIG_L4IDE_DEBUG_MSG, "invalidate_bdev called for %i.%i",
	    MAJOR(bdev->bd_inode->i_rdev),MINOR(bdev->bd_inode->i_rdev));
}

int __invalidate_device(struct block_device *bdev, int do_sync)
{
    // Maybe we should pay attention to do_sync
    invalidate_bdev(bdev,0);
    return 0;
}

struct block_device *bdget(dev_t dev, int create_new)
{
	struct block_device *bdev;
	struct list_head *p;

	list_for_each(p,&all_bdevs) {
	    bdev = list_entry(p, struct block_device, bd_list);
	    if (bdev->bd_dev == dev) {
		// increment refcount
		return bdev;
	    }
	}
	
	if (!create_new) return NULL;

	bdev=kmalloc(sizeof(struct block_device),0);
	if (!bdev) {
	    LOG_Error("out of memory!");
	    return NULL;
	}
	memset(bdev,0,sizeof(struct block_device));
	
	bdev->bd_inode = kmalloc(sizeof(struct inode),0);
	if (!bdev->bd_inode) {
	    LOG_Error("out of memory!");
	    kfree(bdev);
	    return NULL;
	}

	bdev->bd_inode->i_bdev = bdev;
	bdev->bd_inode->i_rdev = dev;
	bdev->bd_block_size = 512; // Have to fix this?
	bdev->bd_dev = dev;
	sema_init(&bdev->bd_sem, 1);
	INIT_LIST_HEAD(&bdev->bd_list);
	spin_lock(&bdev_lock);
	list_add(&bdev->bd_list, &all_bdevs);
	spin_unlock(&bdev_lock);
	LOGd(CONFIG_L4IDE_DEBUG_MSG, "created new bdev %i.%i",MAJOR(dev),MINOR(dev));
	return bdev;
}

void add_bdev(struct gendisk *disk, int part)
{
	struct block_device *bdev,*mummy = bdget(MKDEV(disk->major,disk->first_minor), 0);

	if (mummy == NULL) {
	    LOG_Error("couldn't find block_device %i.%i", disk->major, disk->first_minor);
	    return;
	}

	bdev=kmalloc(sizeof(struct block_device), 0);
	if (!bdev) {
	    LOG_Error("out of memory!");
	    return;
	}

	memset(bdev,0,sizeof(struct block_device));
	
	bdev->bd_inode = kmalloc(sizeof(struct inode), 0);
	if (!bdev->bd_inode) {
	    LOG_Error("out of memory!");
	    kfree(bdev);
	    return;
	}

	bdev->bd_inode->i_bdev = bdev;
	bdev->bd_dev = bdev->bd_inode->i_rdev = MKDEV(disk->major,disk->first_minor+part);
	bdev->bd_contains = mummy;
	bdev->bd_part = disk->part[part-1];
	bdev->bd_disk = disk;
	bdev->bd_block_size = mummy->bd_block_size;
	bdev->bd_size = ((loff_t)bdev->bd_part->nr_sects) * bdev_hardsect_size(bdev);
	sema_init(&bdev->bd_sem, 1);
	INIT_LIST_HEAD(&bdev->bd_list);
	spin_lock(&bdev_lock);
	list_add(&bdev->bd_list, &all_bdevs);
	spin_unlock(&bdev_lock);
	LOGd(CONFIG_L4IDE_DEBUG_MSG, "added new bdev %i.%i",disk->major,disk->first_minor+part);
}
