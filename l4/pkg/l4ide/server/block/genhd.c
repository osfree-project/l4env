/*
 *  gendisk handling
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/kmod.h>
#include <linux/kobj_map.h>

#define MAX_PROBE_HASH 255	/* random */

static struct subsystem block_subsys;

/*
 * Can be deleted altogether. Later.
 *
 * Modified under both block_subsys.rwsem and major_names_lock.
 */
static struct blk_major_name {
	struct blk_major_name *next;
	int major;
	char name[16];
} *major_names[MAX_PROBE_HASH];

static spinlock_t major_names_lock = SPIN_LOCK_UNLOCKED;

/* index in the above - for now: assume no multimajor ranges */
static inline int major_to_index(int major)
{
	return major % MAX_PROBE_HASH;
}

/* get block device names in somewhat random order */
int get_blkdev_list(char *p)
{
	struct blk_major_name *n;
	int i, len;

	len = sprintf(p, "\nBlock devices:\n");

	down_read(&block_subsys.rwsem);
	for (i = 0; i < ARRAY_SIZE(major_names); i++) {
		for (n = major_names[i]; n; n = n->next)
			len += sprintf(p+len, "%3d %s\n",
				       n->major, n->name);
	}
	up_read(&block_subsys.rwsem);

	return len;
}

int register_blkdev(unsigned int major, const char *name)
{
	struct blk_major_name **n, *p;
	int index, ret = 0;
	unsigned long flags;

	down_write(&block_subsys.rwsem);

	/* temporary */
	if (major == 0) {
		for (index = ARRAY_SIZE(major_names)-1; index > 0; index--) {
			if (major_names[index] == NULL)
				break;
		}

		if (index == 0) {
			printk("register_blkdev: failed to get major for %s\n",
			       name);
			ret = -EBUSY;
			goto out;
		}
		major = index;
		ret = major;
	}

	p = kmalloc(sizeof(struct blk_major_name), GFP_KERNEL);
	if (p == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	p->major = major;
	strlcpy(p->name, name, sizeof(p->name));
	p->next = 0;
	index = major_to_index(major);

	spin_lock_irqsave(&major_names_lock, flags);
	for (n = &major_names[index]; *n; n = &(*n)->next) {
		if ((*n)->major == major)
			break;
	}
	if (!*n)
		*n = p;
	else
		ret = -EBUSY;
	spin_unlock_irqrestore(&major_names_lock, flags);

	if (ret < 0) {
		printk("register_blkdev: cannot get major %d for %s\n",
		       major, name);
		kfree(p);
	}
out:
	up_write(&block_subsys.rwsem);
	return ret;
}

EXPORT_SYMBOL(register_blkdev);

/* todo: make void - error printk here */
int unregister_blkdev(unsigned int major, const char *name)
{
	struct blk_major_name **n;
	struct blk_major_name *p = NULL;
	int index = major_to_index(major);
	unsigned long flags;
	int ret = 0;

	down_write(&block_subsys.rwsem);
	spin_lock_irqsave(&major_names_lock, flags);
	for (n = &major_names[index]; *n; n = &(*n)->next)
		if ((*n)->major == major)
			break;
	if (!*n || strcmp((*n)->name, name))
		ret = -EINVAL;
	else {
		p = *n;
		*n = p->next;
	}
	spin_unlock_irqrestore(&major_names_lock, flags);
	up_write(&block_subsys.rwsem);
	kfree(p);

	return ret;
}

EXPORT_SYMBOL(unregister_blkdev);

static struct kobj_map *bdev_map;

/*
 * Register device numbers dev..(dev+range-1)
 * range must be nonzero
 * The hash chain is sorted on range, so that subranges can override.
 */
void blk_register_region(dev_t dev, unsigned long range, struct module *module,
			 struct kobject *(*probe)(dev_t, int *, void *),
			 int (*lock)(dev_t, void *), void *data)
{
	kobj_map(bdev_map, dev, range, module, probe, lock, data);
}

EXPORT_SYMBOL(blk_register_region);

void blk_unregister_region(dev_t dev, unsigned long range)
{
	kobj_unmap(bdev_map, dev, range);
}

EXPORT_SYMBOL(blk_unregister_region);

static struct kobject *exact_match(dev_t dev, int *part, void *data)
{
	return (struct kobject *)data;
}

static int exact_lock(dev_t dev, void *data)
{
	struct gendisk *p = data;

	if (!get_disk(p))
		return -1;
	return 0;
}

/**
 * add_disk - add partitioning information to kernel list
 * @disk: per-device partitioning information
 *
 * This function registers the partitioning information in @disk
 * with the kernel.
 */
void add_disk(struct gendisk *disk)
{
	disk->flags |= GENHD_FL_UP;
	blk_register_region(MKDEV(disk->major, disk->first_minor),
			    disk->minors, NULL, exact_match, exact_lock, disk);
	register_disk(disk);
	blk_register_queue(disk);
}

EXPORT_SYMBOL(add_disk);
EXPORT_SYMBOL(del_gendisk);	/* in partitions/check.c */

void unlink_gendisk(struct gendisk *disk)
{
	blk_unregister_queue(disk);
	blk_unregister_region(MKDEV(disk->major, disk->first_minor),
			      disk->minors);
}

/**
 * get_gendisk - get partitioning information for a given device
 * @dev: device to get partitioning information for
 *
 * This function gets the structure containing partitioning
 * information for the given device @dev.
 */
struct gendisk *get_gendisk(dev_t dev, int *part)
{
	struct kobject *kobj = kobj_lookup(bdev_map, dev, part);
	return (struct gendisk *)kobj;
}

extern int blk_dev_init(void);

static struct kobject *base_probe(dev_t dev, int *part, void *data)
{
	if (request_module("block-major-%d-%d", MAJOR(dev), MINOR(dev)) > 0)
	    /* Make old-style 2.4 aliases work */
	    request_module("block-major-%d", MAJOR(dev));
	return NULL;
}

#include <linux/rwsem.h>

int __init device_init(void)
{
	bdev_map = kobj_map_init(base_probe, &block_subsys);
	blk_dev_init();
//	subsystem_register(&block_subsys);
	init_rwsem(&block_subsys.rwsem);
	return 0;
}

module_init(device_init);


/* eventually for later use
static void disk_release(struct kobject * kobj)
{
	struct gendisk *disk = (struct gendisk *)kobj;
	kfree(disk->random);
	kfree(disk->part);
	free_disk_stats(disk);
	kfree(disk);
}
*/

struct gendisk *alloc_disk(int minors)
{
	struct gendisk *disk = kmalloc(sizeof(struct gendisk), GFP_KERNEL);
	if (disk) {
		memset(disk, 0, sizeof(struct gendisk));
		if (minors > 1) {
			int size = (minors - 1) * sizeof(struct hd_struct *);
			disk->part = kmalloc(size, GFP_KERNEL);
			if (!disk->part) {
				kfree(disk);
				return NULL;
			}
			memset(disk->part, 0, size);
		}
		disk->minors = minors;
		rand_initialize_disk(disk);
	}
	return disk;
}

EXPORT_SYMBOL(alloc_disk);

struct kobject *get_disk(struct gendisk *disk)
{
	struct module *owner;

	if (!disk->fops)
		return NULL;
	owner = disk->fops->owner;
	if (owner && !try_module_get(owner))
		return NULL;
//	kobj = kobject_get(&disk->kobj);
 	// evtl einen busy check machen
/*	if (kobj == NULL) {
		module_put(owner);
		return NULL;
	}
*/	return (struct kobject *)disk;

}

EXPORT_SYMBOL(get_disk);

void put_disk(struct gendisk *disk)
{
//	if (disk)
//		kobject_put(&disk->kobj);
// evtl refcount emulieren
}

EXPORT_SYMBOL(put_disk);
/*
void set_device_ro(struct block_device *bdev, int flag)
{
	if (bdev->bd_contains != bdev)
		bdev->bd_part->policy = flag;
	else
		bdev->bd_disk->policy = flag;
}

EXPORT_SYMBOL(set_device_ro);
*/
void set_disk_ro(struct gendisk *disk, int flag)
{
	int i;
	disk->policy = flag;
	for (i = 0; i < disk->minors - 1; i++)
		if (disk->part[i]) disk->part[i]->policy = flag;
}

EXPORT_SYMBOL(set_disk_ro);
/*
int bdev_read_only(struct block_device *bdev)
{
	if (!bdev)
		return 0;
	else if (bdev->bd_contains != bdev)
		return bdev->bd_part->policy;
	else
		return bdev->bd_disk->policy;
}

EXPORT_SYMBOL(bdev_read_only);
*/
int invalidate_partition(struct gendisk *disk, int index)
{
	int res = 0;
	struct block_device *bdev = bdget_disk(disk, index, 0);
	if (bdev)
		res = __invalidate_device(bdev, 1);
	bdput(bdev);
	return res;
}

EXPORT_SYMBOL(invalidate_partition);