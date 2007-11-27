/** lib/src/arch/l4/inodes.c
 *
 * Assorted dummies implementing inode and superblock access functions,
 * which are used by the block layer stuff, but not needed in DDE_Linux.
 */

#include "local.h"

#include <linux/fs.h>

/**********************************
 * Inode stuff                    *
 **********************************/

void __mark_inode_dirty(struct inode *inode, int flags)
{
	WARN_UNIMPL;
}

void iput(struct inode *inode)
{
	WARN_UNIMPL;
}

void generic_delete_inode(struct inode *inode)
{
	WARN_UNIMPL;
}

int invalidate_inodes(struct super_block * sb)
{
	WARN_UNIMPL;
	return 0;
}

void truncate_inode_pages(struct address_space *mapping, loff_t lstart)
{
	WARN_UNIMPL;
}

void touch_atime(struct vfsmount *mnt, struct dentry *dentry)
{
	WARN_UNIMPL;
}

/**********************************
 * Superblock stuff               *
 **********************************/

struct super_block * get_super(struct block_device *bdev)
{
	WARN_UNIMPL;
	return NULL;
}

int simple_statfs(struct dentry *dentry, struct kstatfs *buf)
{
	WARN_UNIMPL;
	return 0;
}

void kill_anon_super(struct super_block *sb)
{
	WARN_UNIMPL;
}

void shrink_dcache_sb(struct super_block * sb)
{
	WARN_UNIMPL;
}

void drop_super(struct super_block *sb)
{
	WARN_UNIMPL;
}

struct inode *iget5_locked(struct super_block *sb, unsigned long hashval,
                           int (*test)(struct inode *, void *),
                           int (*set)(struct inode *, void *), void *data)
{
	WARN_UNIMPL;
	return NULL;
}

void unlock_new_inode(struct inode *inode)
{
	WARN_UNIMPL;
}

int get_sb_pseudo(struct file_system_type *fs_type, char *name,
                  struct super_operations *ops, unsigned long magic,
                  struct vfsmount *mnt)
{
	WARN_UNIMPL;
	return 0;
}
