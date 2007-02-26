/*
 *  fs/partitions/check.c
 *
 *  Code extracted from drivers/block/genhd.c
 *  Copyright (C) 1991-1998  Linus Torvalds
 *  Re-organised Feb 1998 Russell King
 *
 *  We now have independent partition support from the
 *  block drivers, which allows all the partition code to
 *  be grouped in one location, and it to be mostly self
 *  contained.
 *
 *  Added needed MAJORS for new pairs, {hdi,hdj}, {hdk,hdl}
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kmod.h>
#include <linux/ctype.h>
#include <linux/devfs_fs_kernel.h>

#include "check.h"
#include "devfs.h"

#include "acorn.h"
#include "amiga.h"
#include "atari.h"
#include "ldm.h"
#include "mac.h"
#include "msdos.h"
#include "nec98.h"
#include "osf.h"
#include "sgi.h"
#include "sun.h"
#include "ibm.h"
#include "ultrix.h"
#include "efi.h"

#ifdef CONFIG_BLK_DEV_MD
extern void md_autodetect_dev(dev_t dev);
#endif

int warn_no_part = 1; /*This is ugly: should make genhd removable media aware*/

static int (*check_part[])(struct parsed_partitions *, struct block_device *) = {
	/*
	 * Probe partition formats with tables at disk address 0
	 * that also have an ADFS boot block at 0xdc0.
	 */
#ifdef CONFIG_ACORN_PARTITION_ICS
	adfspart_check_ICS,
#endif
#ifdef CONFIG_ACORN_PARTITION_POWERTEC
	adfspart_check_POWERTEC,
#endif
#ifdef CONFIG_ACORN_PARTITION_EESOX
	adfspart_check_EESOX,
#endif

	/*
	 * Now move on to formats that only have partition info at
	 * disk address 0xdc0.  Since these may also have stale
	 * PC/BIOS partition tables, they need to come before
	 * the msdos entry.
	 */
#ifdef CONFIG_ACORN_PARTITION_CUMANA
	adfspart_check_CUMANA,
#endif
#ifdef CONFIG_ACORN_PARTITION_ADFS
	adfspart_check_ADFS,
#endif

#ifdef CONFIG_EFI_PARTITION
	efi_partition,		/* this must come before msdos */
#endif
#ifdef CONFIG_LDM_PARTITION
	ldm_partition,		/* this must come before msdos */
#endif
#ifdef CONFIG_NEC98_PARTITION
	nec98_partition,	/* must be come before `msdos_partition' */
#endif
#ifdef CONFIG_MSDOS_PARTITION
	msdos_partition,
#endif
#ifdef CONFIG_OSF_PARTITION
	osf_partition,
#endif
#ifdef CONFIG_SUN_PARTITION
	sun_partition,
#endif
#ifdef CONFIG_AMIGA_PARTITION
	amiga_partition,
#endif
#ifdef CONFIG_ATARI_PARTITION
	atari_partition,
#endif
#ifdef CONFIG_MAC_PARTITION
	mac_partition,
#endif
#ifdef CONFIG_SGI_PARTITION
	sgi_partition,
#endif
#ifdef CONFIG_ULTRIX_PARTITION
	ultrix_partition,
#endif
#ifdef CONFIG_IBM_PARTITION
	ibm_partition,
#endif
	NULL
};
 
/*
 * disk_name() is used by partition check code and the genhd driver.
 * It formats the devicename of the indicated disk into
 * the supplied buffer (of size at least 32), and returns
 * a pointer to that same buffer (for convenience).
 */

char *disk_name(struct gendisk *hd, int part, char *buf)
{
	if (!part)
		snprintf(buf, BDEVNAME_SIZE, "%s", hd->disk_name);
	else if (isdigit(hd->disk_name[strlen(hd->disk_name)-1]))
		snprintf(buf, BDEVNAME_SIZE, "%sp%d", hd->disk_name, part);
	else
		snprintf(buf, BDEVNAME_SIZE, "%s%d", hd->disk_name, part);

	return buf;
}

const char *bdevname(struct block_device *bdev, char *buf)
{
	int part = MINOR(bdev->bd_dev) - bdev->bd_disk->first_minor;
	return disk_name(bdev->bd_disk, part, buf);
}

EXPORT_SYMBOL(bdevname);

/*
 * NOTE: this cannot be called from interrupt context.
 *
 * But in interrupt context you should really have a struct
 * block_device anyway and use bdevname() above.
 */
const char *__bdevname(dev_t dev, char *buffer)
{
	struct gendisk *disk;
	int part;

	disk = get_gendisk(dev, &part);
	if (disk) {
		buffer = disk_name(disk, part, buffer);
		put_disk(disk);
	} else {
		snprintf(buffer, BDEVNAME_SIZE, "unknown-block(%u,%u)",
				MAJOR(dev), MINOR(dev));
	}

	return buffer;
}

EXPORT_SYMBOL(__bdevname);

static struct parsed_partitions *
check_partition(struct gendisk *hd, struct block_device *bdev)
{
	struct parsed_partitions *state;
	int i, res;

	state = kmalloc(sizeof(struct parsed_partitions), GFP_KERNEL);
	if (!state)
		return NULL;

#ifdef CONFIG_DEVFS_FS
	if (hd->devfs_name[0] != '\0') {
		printk(KERN_INFO " /dev/%s:", hd->devfs_name);
		sprintf(state->name, "p");
	}
#endif
	else {
		disk_name(hd, 0, state->name);
		printk(KERN_INFO " %s:", state->name);
		if (isdigit(state->name[strlen(state->name)-1]))
			sprintf(state->name, "p");
	}
	state->limit = hd->minors;
	i = res = 0;
	while (!res && check_part[i]) {
		memset(&state->parts, 0, sizeof(state->parts));
		res = check_part[i++](state, bdev);
	}
	if (res > 0)
		return state;
	if (!res)
		printk(" unknown partition table\n");
	else if (warn_no_part)
		printk(" unable to read partition table\n");
	kfree(state);
	return NULL;
}

static void part_release(struct kobject *kobj)
{
	struct hd_struct * p = (struct hd_struct *)kobj;
	kfree(p);
}

struct kobj_type ktype_part = {
	.release	= part_release,
};

void delete_partition(struct gendisk *disk, int part)
{
	struct hd_struct *p = disk->part[part-1];
	if (!p)
		return;
	if (!p->nr_sects)
		return;
	disk->part[part-1] = NULL;
	p->start_sect = 0;
	p->nr_sects = 0;
	p->reads = p->writes = p->read_sectors = p->write_sectors = 0;
}

void add_partition(struct gendisk *disk, int part, sector_t start, sector_t len)
{
	struct hd_struct *p;

	p = kmalloc(sizeof(*p), GFP_KERNEL);
	if (!p)
		return;
	
	memset(p, 0, sizeof(*p));
	p->start_sect = start;
	p->nr_sects = len;
	p->partno = part;

	if (isdigit(disk->kobj.name[strlen(disk->kobj.name)-1]))
		snprintf(p->kobj.name,KOBJ_NAME_LEN,"%sp%d",disk->kobj.name,part); //check
	else
		snprintf(p->kobj.name,KOBJ_NAME_LEN,"%s%d",disk->kobj.name,part); //check
	p->kobj.parent = &disk->kobj;; //check
	p->kobj.ktype = &ktype_part;; //check
	disk->part[part-1] = p;
	add_bdev(disk,part);
}

/* Not exported, helper to add_disk(). */
void register_disk(struct gendisk *disk)
{
	struct parsed_partitions *state;
	struct block_device *bdev;
	char *s;
	int j;

	strlcpy(disk->kobj.name,disk->disk_name,KOBJ_NAME_LEN); //check
	/* ewww... some of these buggers have / in name... */
	s = strchr(disk->kobj.name, '/'); //check
	if (s)
		*s = '!';

	/* No such device (e.g., media were just removed) */
	if (!get_capacity(disk))
		return;

	if ((bdev = bdget_disk(disk, 0, 1))==NULL) return;
	if (blkdev_get(bdev) < 0)
		return;
	bdev->bd_disk = disk;
	bdev->bd_size = ((loff_t)get_capacity(disk)) << 9;
	/* No minors to use for partition */
	if (disk->minors > 1) {
	    state = check_partition(disk, bdev);
	    if (state) {
		for (j = 1; j < state->limit; j++) {
			sector_t size = state->parts[j].size;
			sector_t from = state->parts[j].from;
			if (!size)
				continue;
			add_partition(disk, j, from, size);
#ifdef CONFIG_BLK_DEV_MD
			if (!state->parts[j].flags)
				continue;
			md_autodetect_dev(bdev->bd_dev+j);
#endif
		}
		kfree(state);
	    }
	}
	blkdev_put(bdev);
}

int rescan_partitions(struct gendisk *disk, struct block_device *bdev)
{
	struct parsed_partitions *state;
	int p, res;

	if (bdev->bd_part_count)
		return -EBUSY;
	res = invalidate_partition(disk, 0);
	if (res)
		return res;
	bdev->bd_invalidated = 0;
	for (p = 1; p < disk->minors; p++)
		delete_partition(disk, p);
	if (disk->fops->revalidate_disk)
		disk->fops->revalidate_disk(disk);
	if (!get_capacity(disk) || !(state = check_partition(disk, bdev)))
		return res;
	for (p = 1; p < state->limit; p++) {
		sector_t size = state->parts[p].size;
		sector_t from = state->parts[p].from;
		if (!size)
			continue;
		add_partition(disk, p, from, size);
#ifdef CONFIG_BLK_DEV_MD
		if (state->parts[p].flags)
			md_autodetect_dev(bdev->bd_dev+p);
#endif
	}
	kfree(state);
	return res;
}

static int __read_dev_sector_end_io(struct bio *bio, unsigned int bytes_done, int err)
{
	if (bio->bi_size) return 1;
	complete(bio->bi_private);

	return 0;
}

unsigned char *read_dev_sector(struct block_device *bdev, sector_t n, Sector *p)
{
	struct bio *bio;
	DECLARE_COMPLETION(wait);

	p->v = (struct page *)kmalloc(bdev->bd_block_size,GFP_KERNEL);
	if (!p->v) return NULL;
	
	bio = bio_alloc(GFP_NOIO, 1);
	bio->bi_sector = n;
	bio->bi_bdev = bdev;
	bio->bi_io_vec[0].bv_addr = (void *)__pa((void *)p->v);
	bio->bi_io_vec[0].bv_len = bdev->bd_block_size;
	bio->bi_vcnt = 1;
	bio->bi_idx = 0;
	bio->bi_size = bdev->bd_block_size;
	bio->bi_end_io = __read_dev_sector_end_io;
	bio->bi_private = &wait;
	
	submit_bio(READ, bio);
	
	wait_for_completion(&wait);
	if (!bio_flagged(bio,BIO_UPTODATE)) {
	    bio_put(bio);
	    return NULL;
	}

	bio_put(bio);
	return (unsigned char *)p->v;
}

EXPORT_SYMBOL(read_dev_sector);

void del_gendisk(struct gendisk *disk)
{
	int p;

	/* invalidate stuff */
	for (p = disk->minors - 1; p > 0; p--) {
		invalidate_partition(disk, p);
		delete_partition(disk, p);
	}
	invalidate_partition(disk, 0);
	disk->capacity = 0;
	disk->flags &= ~GENHD_FL_UP;
	unlink_gendisk(disk);
//	disk_stat_set_all(disk, 0); //??? For later use???
	disk->stamp = disk->stamp_idle = 0;
}
