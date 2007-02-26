/*
 *  linux/fs/block_dev.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *  Copyright (C) 2001  Andrea Arcangeli <andrea@suse.de> SuSE
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <linux/kmod.h>
#include <linux/major.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/smp_lock.h>
#include <linux/highmem.h>
#include <linux/blkdev.h>
#include <linux/module.h>
#include <linux/blkpg.h>
#include <linux/buffer_head.h>
#include <linux/mpage.h>
#include <linux/mount.h>
#include <linux/uio.h>
#include <linux/namei.h>
#include <asm/uaccess.h>

#include <l4/util/macros.h>
#include <driver/config.h>

/* Kill _all_ buffers, dirty or not.. */
static void kill_bdev(struct block_device *bdev)
{
	invalidate_bdev(bdev, 1);
}	

int set_blocksize(struct block_device *bdev, int size)
{
	int oldsize;

	/* Size must be a power of two, and between 512 and PAGE_SIZE */
	if (size > PAGE_SIZE || size < 512 || (size & (size-1)))
		return -EINVAL;

	/* Size cannot be smaller than the size supported by the device */
	if (size < bdev_hardsect_size(bdev))
		return -EINVAL;

	oldsize = bdev->bd_block_size;
	if (oldsize == size)
		return 0;

	/* Ok, we're actually changing the blocksize.. */
	LOG("Fixme set_blocksize");
//	sync_blockdev(bdev);
	bdev->bd_block_size = size;
//	bdev->bd_inode->i_blkbits = blksize_bits(size);
//	kill_bdev(bdev);
	return 0;
}

/*
 * pseudo-fs
 */

static spinlock_t bdev_lock __cacheline_aligned_in_smp = SPIN_LOCK_UNLOCKED;

/*
 * Most likely _very_ bad one - but then it's hardly critical for small
 * /dev and can be fixed when somebody will need really large one.
 * Keep in mind that it will be fed through icache hash function too.
 */
static inline unsigned long hash(dev_t dev)
{
	return MAJOR(dev)+MINOR(dev);
}

void bdput(struct block_device *bdev)
{
	LOG_Error("bdput called but not implemented");
	// decrement refcount?
//	iput(bdev->bd_inode);
}

int bd_claim(struct block_device *bdev, void *holder)
{
	int res;
	spin_lock(&bdev_lock);

	/* first decide result */
	if (bdev->bd_holder == holder)
		res = 0;	 /* already a holder */
	else if (bdev->bd_holder != NULL)
		res = -EBUSY; 	 /* held by someone else */
	else if (bdev->bd_contains == bdev)
		res = 0;  	 /* is a whole device which isn't held */

	else if (bdev->bd_contains->bd_holder == bd_claim)
		res = 0; 	 /* is a partition of a device that is being partitioned */
	else if (bdev->bd_contains->bd_holder != NULL)
		res = -EBUSY;	 /* is a partition of a held device */
	else
		res = 0;	 /* is a partition of an un-held device */

	/* now impose change */
	if (res==0) {
		/* note that for a whole device bd_holders
		 * will be incremented twice, and bd_holder will
		 * be set to bd_claim before being set to holder
		 */
		bdev->bd_contains->bd_holders ++;
		bdev->bd_contains->bd_holder = bd_claim;
		bdev->bd_holders++;
		bdev->bd_holder = holder;
	}
	spin_unlock(&bdev_lock);
	return res;
}

void bd_release(struct block_device *bdev)
{
	spin_lock(&bdev_lock);
	if (!--bdev->bd_contains->bd_holders)
		bdev->bd_contains->bd_holder = NULL;
	if (!--bdev->bd_holders)
		bdev->bd_holder = NULL;
	spin_unlock(&bdev_lock);
}

/*
 * Tries to open block device by device number.  Use it ONLY if you
 * really do not have anything better - i.e. when you are behind a
 * truly sucky interface and all you are given is a device number.  _Never_
 * to be used for internal purposes.  If you ever need it - reconsider
 * your API.
 */
struct block_device *open_by_devnum(dev_t dev, unsigned mode)
{
	struct block_device *bdev = bdget(dev, 0);
	int err = -ENOMEM;
	if (bdev)
		err = blkdev_get(bdev);
	return err ? ERR_PTR(err) : bdev;
}

/*
 * This routine checks whether a removable media has been changed,
 * and invalidates all buffer-cache-entries in that case. This
 * is a relatively slow routine, so we have to try to minimize using
 * it. Thus it is called only upon a 'mount' or 'open'. This
 * is the best way of combining speed and utility, I think.
 * People changing diskettes in the middle of an operation deserve
 * to lose :-)
 */
int check_disk_change(struct block_device *bdev)
{
	struct gendisk *disk = bdev->bd_disk;
	struct block_device_operations * bdops = disk->fops;

	if (!bdops->media_changed)
		return 0;
	if (!bdops->media_changed(bdev->bd_disk))
		return 0;

	if (__invalidate_device(bdev, 0))
		printk("VFS: busy inodes on changed media.\n");

	if (bdops->revalidate_disk)
		bdops->revalidate_disk(bdev->bd_disk);
	if (bdev->bd_disk->minors > 1)
		bdev->bd_invalidated = 1;
	return 1;
}

void bd_set_size(struct block_device *bdev, loff_t size)
{
	unsigned bsize = bdev_hardsect_size(bdev);

	bdev->bd_inode->i_size = size;
	while (bsize < PAGE_CACHE_SIZE) {
		if (size & bsize)
			break;
		bsize <<= 1;
	}
	bdev->bd_block_size = bsize;
}

static int do_open(struct block_device *bdev)
{
	struct gendisk *disk;
	int ret = -ENXIO;
	int part;
	void * holder = current;

	disk = get_gendisk(bdev->bd_dev, &part);
	if (!disk) {
		bdput(bdev);
		return ret;
	}

	down(&bdev->bd_sem);
	if (!bdev->bd_openers) {
		bdev->bd_disk = disk;
		bdev->bd_contains = bdev;
		if (!part) {
			if (disk->fops->open) {
				ret = disk->fops->open(bdev->bd_inode, holder);
				if (ret)
					goto out_first;
			}
			if (!bdev->bd_openers) {
				bd_set_size(bdev,(loff_t)get_capacity(disk)<<9);
			}
			if (bdev->bd_invalidated)
				rescan_partitions(disk, bdev);
		} else {
			struct hd_struct *p;
			struct block_device *whole;
LOG("Fixme do_open part");
return 0;
			whole = bdget_disk(disk, 0, 0);
			ret = -ENOMEM;
			if (!whole)
				goto out_first;
			ret = blkdev_get(whole);
			if (ret)
				goto out_first;
			bdev->bd_contains = whole;
			down(&whole->bd_sem);
			whole->bd_part_count++;
			p = disk->part[part - 1];
			if (!(disk->flags & GENHD_FL_UP) || !p || !p->nr_sects) {
				whole->bd_part_count--;
				up(&whole->bd_sem);
				ret = -ENXIO;
				goto out_first;
			}
			bdev->bd_part = p;
			bd_set_size(bdev, (loff_t) p->nr_sects << 9);
			up(&whole->bd_sem);
		}
	} else {
		put_disk(disk);
LOG("Fixme do_open");
return 0;
		if (bdev->bd_contains == bdev) {
			if (bdev->bd_disk->fops->open) {
				ret = bdev->bd_disk->fops->open(bdev->bd_inode, holder);
				if (ret)
					goto out;
			}
			if (bdev->bd_invalidated)
				rescan_partitions(bdev->bd_disk, bdev);
		} else {
			down(&bdev->bd_contains->bd_sem);
			bdev->bd_contains->bd_part_count++;
			up(&bdev->bd_contains->bd_sem);
		}
	}
	bdev->bd_openers++;
	up(&bdev->bd_sem);
	return 0;

out_first:
//	bdev->bd_disk = NULL;
//	if (bdev != bdev->bd_contains)
//		blkdev_put(bdev->bd_contains);
//	bdev->bd_contains = NULL;
	put_disk(disk);
out:
	up(&bdev->bd_sem);
//	if (ret)
//		bdput(bdev);
	return ret;
}

int blkdev_get(struct block_device *bdev)
{
	/*
	 * This crockload is due to bad choice of ->open() type.
	 * It will go away.
	 * For now, block device ->open() routine must _not_
	 * examine anything in 'inode' argument except ->i_rdev.
	 */

	return do_open(bdev);
}

int blkdev_open(struct block_device * bdev, void * holder)
{
	int res;
LOG("Fixme blkdev_open");
	/*
	 * Preserve backwards compatibility and allow large file access
	 * even if userspace doesn't ask for it explicitly. Some mkfs
	 * binary needs it. We might want to drop this workaround
	 * during an unstable branch.
	 */

	res = do_open(bdev);
	if (res)
		return res;

	if (!(res = bd_claim(bdev, holder)))
		return 0;

	blkdev_put(bdev);
	return res;
}

int blkdev_put(struct block_device *bdev)
{
	int ret = 0;
	struct gendisk *disk = bdev->bd_disk;
	struct inode *bd_inode; // init for blkdev_put() use
LOGd(CONFIG_L4IDE_DEBUG_MSG, "Fixme blkdev_put %i.%i",MAJOR(bdev->bd_dev),MINOR(bdev->bd_dev));
return 0;
	down(&bdev->bd_sem);
	if (!--bdev->bd_openers) {
		kill_bdev(bdev);
	}
	if (bdev->bd_contains == bdev) {
		if (disk->fops->release)
			ret = disk->fops->release(bd_inode, NULL);
	} else {
		down(&bdev->bd_contains->bd_sem);
		bdev->bd_contains->bd_part_count--;
		up(&bdev->bd_contains->bd_sem);
	}
	if (!bdev->bd_openers) {
		put_disk(disk);

		if (bdev->bd_contains != bdev) {
			bdev->bd_part = NULL;
		}
		bdev->bd_disk = NULL;
		if (bdev != bdev->bd_contains) {
			blkdev_put(bdev->bd_contains);
		}
		bdev->bd_contains = NULL;
	}
	up(&bdev->bd_sem);
	bdput(bdev);
	return ret;
}

int blkdev_close(struct block_device * bdev, void * holder)
{
	if (bdev->bd_holder == holder)
		bd_release(bdev);
	return blkdev_put(bdev);
}

struct file_operations def_blk_fops = {
	.open		= blkdev_open,
	.release	= blkdev_close,
	.ioctl		= blkdev_ioctl,
};

int ioctl_by_bdev(struct block_device *bdev, unsigned cmd, unsigned long arg)
{
	int res;
	res = blkdev_ioctl(bdev->bd_inode, NULL, cmd, arg);
	return res;
}
