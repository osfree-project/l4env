#include <l4/log/l4log.h>

#include <linux/vmalloc.h>
#include <linux/proc_fs.h>

#include <linux/config.h>
#include <linux/compiler.h>
#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>

struct proc_dir_entry *proc_net;
struct proc_dir_entry *proc_net_stat;
struct proc_dir_entry *proc_sys_root;


/* we do not supply any fs functionality */
static struct file_operations proc_dir_operations;
static struct inode_operations proc_dir_inode_operations;
static struct file_operations proc_root_operations;
static struct inode_operations proc_root_inode_operations;
static struct file_operations proc_file_operations;

static struct proc_dir_entry proc_root = {
	low_ino:PROC_ROOT_INO,
	namelen:5,
	name:"/proc",
	mode:S_IFDIR | S_IRUGO | S_IXUGO,
	nlink:2,
	proc_iops:&proc_root_inode_operations,
	proc_fops:&proc_root_operations,
	parent:&proc_root,
};

/** UTILITY: TEST IF PROC ENTRY EQUALS THE GIVEN STRING
 *
 * This function is only used by xlate_proc_name.
 * It is taken from the original fs/proc/generic.c but the inode stuff
 * is kicked out.
 */
int proc_match(int len, const char *name, struct proc_dir_entry *de)
{
	if (de->namelen != len)
		return 0;
	return !memcmp(name, de->name, len);
}


/** UTILITY: RETURN PROC ENTRY STRUCTURE OF A GIVEN PATH
 *
 * This function parses a name such as "tty/driver/serial", and
 * returns the struct proc_dir_entry for "/proc/tty/driver", and
 * returns "serial" in residual.
 */
static int xlate_proc_name(const char *name,
                           struct proc_dir_entry **ret,
                           const char **residual)
{
	const char *cp = name, *next;
	struct proc_dir_entry *de;
	int len;

	de = &proc_root;
	while (1) {
		next = strchr(cp, '/');
		if (!next)
			break;

		len = next - cp;
		for (de = de->subdir; de; de = de->next) {
			if (proc_match(len, cp, de))
				break;
		}
		if (!de)
			return -ENOENT;
		cp += len + 1;
	}
	*residual = cp;
	*ret = de;
	return 0;
}


/** RETURNS PROC ENTRY AT THE SPECIFIED PATH
 */
static struct proc_dir_entry *get_proc_dir_entry_by_name(char *path)
{
	struct proc_dir_entry *dp = &proc_root;
	int cur_len;
	int left_len = strlen(path);

	while (1) {
		while (dp) {
			cur_len = strlen(dp->name);
			if (!strncmp(path, dp->name, cur_len))
				break;
			dp = dp->next;
		}
		if (!dp)
			return NULL;
		path = path + cur_len + 1;
		left_len -= cur_len + 1;
		if (left_len <= 0)
			return dp;
		dp = dp->subdir;
	}
}


/** UTILITIY: REGISTER PROC ENTRY
 *
 * Initialises the given proc entry.
 * This function is taken from fs/proc/generic.c but simplified a bit.
 */
static int proc_register(struct proc_dir_entry *dir,
                         struct proc_dir_entry *dp)
{
	dp->next = dir->subdir;
	dp->parent = dir;
	dir->subdir = dp;
	if (S_ISDIR(dp->mode)) {
		if (dp->proc_iops == NULL) {
			dp->proc_fops = &proc_dir_operations;
			dp->proc_iops = &proc_dir_inode_operations;
		}
		dir->nlink++;
	} else if (S_ISREG(dp->mode)) {
		if (dp->proc_fops == NULL)
			dp->proc_fops = &proc_file_operations;
	}
	return 0;
}


/** UTILITY: CREATE NEW PROC ENTRY
 *
 * This function is copied from fs/proc/generic.c
 */
static struct proc_dir_entry *proc_create(struct proc_dir_entry **parent,
                                          const char *name,
                                          mode_t mode, nlink_t nlink)
{
	struct proc_dir_entry *ent = NULL;
	const char *fn = name;
	int len;

	/* make sure name is valid */
	if (!name || !strlen(name))
		goto out;

	if (!(*parent) && xlate_proc_name(name, parent, &fn) != 0)
		goto out;
	len = strlen(fn);

	ent = kmalloc(sizeof(struct proc_dir_entry) + len + 1, GFP_KERNEL);
	if (!ent)
		goto out;

	memset(ent, 0, sizeof(struct proc_dir_entry));
	memcpy(((char *)ent) + sizeof(struct proc_dir_entry), fn, len + 1);
	ent->name = ((char *)ent) + sizeof(*ent);
	ent->namelen = len;
	ent->mode = mode;
	ent->nlink = nlink;
  out:
	return ent;
}


/** LINUX: CREATE PROC_FS SUBDIR
 *
 * This function is copied from fs/proc/generic.c
 */
struct proc_dir_entry *proc_mkdir(const char *name,
                                  struct proc_dir_entry *parent)
{
	struct proc_dir_entry *ent;

	ent = proc_create(&parent, name, (S_IFDIR | S_IRUGO | S_IXUGO), 2);
	if (ent) {
		ent->proc_fops = &proc_dir_operations;
		ent->proc_iops = &proc_dir_inode_operations;

		proc_register(parent, ent);
	}
	return ent;
}


/** LINUX: CREATE NEW PROC ENTRY
 *
 * This function is copied from fs/proc/generic.c
 */
struct proc_dir_entry *create_proc_entry(const char *name, mode_t mode,
                                         struct proc_dir_entry *parent)
{
	struct proc_dir_entry *ent;
	nlink_t nlink;

	if (S_ISDIR(mode)) {
		if ((mode & S_IALLUGO) == 0)
			mode |= S_IRUGO | S_IXUGO;
		nlink = 2;
	} else {
		if ((mode & S_IFMT) == 0)
			mode |= S_IFREG;
		if ((mode & S_IALLUGO) == 0)
			mode |= S_IRUGO;
		nlink = 1;
	}

	ent = proc_create(&parent, name, mode, nlink);
	if (ent) {
		if (S_ISDIR(mode)) {
			ent->proc_fops = &proc_dir_operations;
			ent->proc_iops = &proc_dir_inode_operations;
		}
		proc_register(parent, ent);
	}
	return ent;
}


/** LINUX: REMOVE A PROC ENTRY FROM PROC-FS
 *
 * At the moment we do not want to get rid of proc entries (in fact we are
 * happy to have some :-) So this is a dummy.
 */
void remove_proc_entry(const char *name, struct proc_dir_entry *parent)
{
}

/** LINUX: ...
 *
 */
struct proc_dir_entry *proc_symlink(const char *name,
                                    struct proc_dir_entry *parent,
                                    const char *dest)
{
	return NULL;
}

/** FLIPS: SHOW PROC DIRECTORY LIST
 *
 * Shows a directory listing of the specified proc path.
 */
void liblinux_proc_ls(char *path)
{
	struct proc_dir_entry *dp;
	LOG_printf("proc directory listing of %s:\n", path);

	dp = get_proc_dir_entry_by_name(path);


	if (!dp) {
		LOG_printf(" Error: %s does not exist.\n", path);
		return;
	}

	dp = dp->subdir;

	if (!dp) {
		LOG_printf(" %s is empty.\n", path);
		return;
	}

	while (dp) {
		if (S_ISDIR(dp->mode))
			LOG_printf(" (dir) ");
		else
			LOG_printf(" (file)");
		LOG_printf("%5d   %s\n", (int)dp->size, dp->name);
		dp = dp->next;
	}

	LOG_printf("directory listing done.\n\n");
}


/** FLIPS: READ FROM PROC ENTRY
 *
 * \param path      path of proc entry in proc fs
 * \param dst       destination buffer
 * \param dst_len   max number of bytes to read 
 *
 * \return number of bytes or a negative error code
 *
 * NOTE: can we just assume that page argument is not used by read_proc?
 */
int liblinux_proc_read(char *path, char *dst, int offset, int dst_len)
{
	struct proc_dir_entry *dp;
	char *start = NULL;
	int eof = 0;

	dp = get_proc_dir_entry_by_name(path);

	if (dp) {
		if (dp->get_info)
			/*
		 * Handle backwards compatibility with the old net
		 * routines.
		 */
			return dp->get_info(dst, &start, offset, dst_len);
		if (dp->read_proc) {
			int ret, i;
			ret = dp->read_proc(dst, &start, offset, dst_len,
			                       &eof, dp->data);
			LOG_printf("proc(read): %d bytes: ", ret);
			for (i = 0; i < ret; i++)
				LOG_printf("%c", dst[i]);
			LOG_printf("\n");
			return ret;
		}
		LOG_printf("liblinux_proc_read: read_proc/get_info undefined!\n");
	}

	return -1;
}


/** FLIPS: WRITE DATA TO PROC ENTRY
 *
 * \param path     path of proc entry in proc fs
 * \param src      source buffer
 * \param src_len  max number of bytes to write
 *
 * \return number of successfully written bytes or a negative error code
 *
 * NOTE: can we just assume that the file structure is not used by write_proc?
 */
int liblinux_proc_write(char *path, char *src, int src_len)
{
	struct proc_dir_entry *dp;
	struct file *file = NULL;

	dp = get_proc_dir_entry_by_name(path);

	if (dp && (dp->write_proc)) {
		return dp->write_proc(file, src, src_len, dp->data);
	}
	return -1;
}


/** FLIPS: INIT PROC EMULATION OF FLIPS
 *
 * The root of the proc-fs is defined here.
 */
int liblinux_proc_init(void)
{
	/* init root of procfs */
	proc_net = proc_mkdir("net", &proc_root);
	proc_net_stat = proc_mkdir("stat", &proc_net);
	proc_sys_root = proc_mkdir("sys", &proc_root);

	return 0;
}

#else /* CONFIG_PROC_FS */

void liblinux_proc_ls(char *path)
{
	LOG_printf("%s not supported", __func__);
}

int liblinux_proc_read(char *path, char *dst, int offset, int dst_len)
{
	LOG_printf("%s not supported", __func__);
	return -1;
}

int liblinux_proc_write(char *path, char *src, int src_len)
{
	LOG_printf("%s not supported", __func__);
	return -1;
}

int liblinux_proc_init(void)
{
	return 0;
}

#endif /* CONFIG_PROC_FS */
