
#include <linux/config.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/swapctl.h>
#include <linux/proc_fs.h>
#include <linux/ctype.h>
#include <linux/utsname.h>
#include <linux/capability.h>
#include <linux/smp_lock.h>
#include <linux/init.h>
#include <linux/sysrq.h>
#include <linux/highuid.h>

#include <asm/uaccess.h>


/* External variables not in a header file. */
extern int panic_timeout;
extern int C_A_D;
extern int bdf_prm[], bdflush_min[], bdflush_max[];
extern int sysctl_overcommit_memory;
extern int max_threads;
extern atomic_t nr_queued_signals;
extern int max_queued_signals;
extern int sysrq_enabled;
extern int core_uses_pid;
extern int cad_pid;

///* this is needed for the proc_dointvec_minmax for [fs_]overflow UID and GID */
//static int maxolduid = 65535;
//static int minolduid;

#ifdef CONFIG_CHR_DEV_SG
extern int sg_big_buff;
#endif

extern int pgt_cache_water[];

static int parse_table(int *, int, void *, size_t *, void *, size_t,
                       ctl_table *, void **);

static ctl_table root_table[];
static struct ctl_table_header root_table_header =
	{ root_table, LIST_HEAD_INIT(root_table_header.ctl_entry) };

extern ctl_table net_table[];

/* /proc declarations: */

#ifdef CONFIG_PROC_FS

//static int proc_doutsstring(ctl_table *table, int write, struct file *filp,
                              //        void *buffer, size_t *lenp);

static ssize_t proc_readsys(struct file *, char *, size_t, loff_t *);
static ssize_t proc_writesys(struct file *, const char *, size_t,
                             loff_t *);
static int proc_sys_permission(struct inode *, int);

struct file_operations proc_sys_file_operations = {
	read:proc_readsys,
	write:proc_writesys,
};

static struct inode_operations proc_sys_inode_operations = {
	permission:proc_sys_permission,
};

extern struct proc_dir_entry *proc_sys_root;

static void register_proc_table(ctl_table *, struct proc_dir_entry *);
static void unregister_proc_table(ctl_table *, struct proc_dir_entry *);
#endif

/* The default sysctl tables: */


extern ctl_table ipv4_table[];

/* XXX krishna: commented this out; it's in sysctl_net.c */
#if 0
ctl_table net_table[] = {
//  {NET_CORE,   "core",      NULL, 0, 0555, core_table},      
//  {NET_802,    "802",       NULL, 0, 0555, e802_table},
//  {NET_ETHER,  "ethernet",  NULL, 0, 0555, ether_table},
	{NET_IPV4, "ipv4", NULL, 0, 0555, ipv4_table},
	{0}
};
#endif

static ctl_table root_table[] = {
	{CTL_NET, "net", NULL, 0, 0555, net_table},
	{0}
};


extern void init_irq_proc(void);

void liblinux_sysctl_init(void)
{
#ifdef CONFIG_PROC_FS
	register_proc_table(root_table, proc_sys_root);
	//init_irq_proc(); /* !!! */
#endif
}

int do_sysctl(int *name, int nlen, void *oldval, size_t * oldlenp,
              void *newval, size_t newlen)
{
	struct list_head *tmp;

	if (nlen <= 0 || nlen >= CTL_MAXNAME)
		return -ENOTDIR;
	if (oldval) {
		int old_len;
		if (!oldlenp || get_user(old_len, oldlenp))
			return -EFAULT;
	}
	tmp = &root_table_header.ctl_entry;
	do {
		struct ctl_table_header *head =
			list_entry(tmp, struct ctl_table_header, ctl_entry);
		void *context = NULL;
		int error = parse_table(name, nlen, oldval, oldlenp,
		                          newval, newlen, head->ctl_table,
		                          &context);
		if (context)
			kfree(context);
		if (error != -ENOTDIR)
			return error;
		tmp = tmp->next;
	}
	while (tmp != &root_table_header.ctl_entry);
	return -ENOTDIR;
}



static int parse_table(int *name, int nlen,
                       void *oldval, size_t * oldlenp,
                       void *newval, size_t newlen,
                       ctl_table * table, void **context)
{
	int n;
  repeat:
	if (!nlen)
		return -ENOTDIR;
	if (get_user(n, name))
		return -EFAULT;
	for (; table->ctl_name; table++) {
		if (n == table->ctl_name || table->ctl_name == CTL_ANY) {
			int error;
			if (table->child) {
				if (table->strategy) {
					error = table->strategy(table, name, nlen,
					                             oldval, oldlenp,
					                             newval, newlen, context);
					if (error)
						return error;
				}
				name++;
				nlen--;
				table = table->child;
				goto repeat;
			}
			error = do_sysctl_strategy(table, name, nlen,
			                              oldval, oldlenp,
			                              newval, newlen, context);
			return error;
		}
	}
	return -ENOTDIR;
}

/* Perform the actual read/write of a sysctl table entry. */
int do_sysctl_strategy(ctl_table * table,
                       int *name, int nlen,
                       void *oldval, size_t * oldlenp,
                       void *newval, size_t newlen, void **context)
{
	int op = 0, rc;
	size_t len;

	if (oldval)
		op |= 004;
	if (newval)
		op |= 002;

	if (table->strategy) {
		rc = table->strategy(table, name, nlen, oldval, oldlenp,
		                       newval, newlen, context);
		if (rc < 0)
			return rc;
		if (rc > 0)
			return 0;
	}

	/* If there is no strategy routine, or if the strategy returns
	 * zero, proceed with automatic r/w */
	if (table->data && table->maxlen) {
		if (oldval && oldlenp) {
			get_user(len, oldlenp);
			if (len) {
				if (len > table->maxlen)
					len = table->maxlen;
				if (copy_to_user(oldval, table->data, len))
					return -EFAULT;
				//                if(put_user(len, oldlenp))
				//                    return -EFAULT;
				if (oldlenp)
					*oldlenp = len;
				else
					return -EFAULT;
			}
		}
		if (newval && newlen) {
			len = newlen;
			if (len > table->maxlen)
				len = table->maxlen;
			if (copy_from_user(table->data, newval, len))
				return -EFAULT;
		}
	}
	return 0;
}

/**
 * register_sysctl_table - register a sysctl hierarchy
 * @table: the top-level table structure
 * @insert_at_head: whether the entry should be inserted in front or at the end
 *
 * Register a sysctl table hierarchy. @table should be a filled in ctl_table
 * array. An entry with a ctl_name of 0 terminates the table. 
 *
 * The members of the &ctl_table structure are used as follows:
 *
 * ctl_name - This is the numeric sysctl value used by sysctl(2). The number
 *            must be unique within that level of sysctl
 *
 * procname - the name of the sysctl file under /proc/sys. Set to %NULL to not
 *            enter a sysctl file
 *
 * data - a pointer to data for use by proc_handler
 *
 * maxlen - the maximum size in bytes of the data
 *
 * mode - the file permissions for the /proc/sys file, and for sysctl(2)
 *
 * child - a pointer to the child sysctl table if this entry is a directory, or
 *         %NULL.
 *
 * proc_handler - the text handler routine (described below)
 *
 * strategy - the strategy routine (described below)
 *
 * de - for internal use by the sysctl routines
 *
 * extra1, extra2 - extra pointers usable by the proc handler routines
 *
 * Leaf nodes in the sysctl tree will be represented by a single file
 * under /proc; non-leaf nodes will be represented by directories.
 *
 * sysctl(2) can automatically manage read and write requests through
 * the sysctl table.  The data and maxlen fields of the ctl_table
 * struct enable minimal validation of the values being written to be
 * performed, and the mode field allows minimal authentication.
 *
 * More sophisticated management can be enabled by the provision of a
 * strategy routine with the table entry.  This will be called before
 * any automatic read or write of the data is performed.
 *
 * The strategy routine may return
 *
 * < 0 - Error occurred (error is passed to user process)
 *
 * 0   - OK - proceed with automatic read or write.
 *
 * > 0 - OK - read or write has been done by the strategy routine, so
 *       return immediately.
 *
 * There must be a proc_handler routine for any terminal nodes
 * mirrored under /proc/sys (non-terminals are handled by a built-in
 * directory handler).  Several default handlers are available to
 * cover common cases -
 *
 * proc_dostring(), proc_dointvec(), proc_dointvec_jiffies(),
 * proc_dointvec_minmax(), proc_doulongvec_ms_jiffies_minmax(),
 * proc_doulongvec_minmax()
 *
 * It is the handler's job to read the input buffer from user memory
 * and process it. The handler should return 0 on success.
 *
 * This routine returns %NULL on a failure to register, and a pointer
 * to the table header on success.
 */
struct ctl_table_header *register_sysctl_table(ctl_table * table,
                                               int insert_at_head)
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
#ifdef CONFIG_PROC_FS
	register_proc_table(table, proc_sys_root);
#endif
	return tmp;
}

/**
 * unregister_sysctl_table - unregister a sysctl table hierarchy
 * @header: the header returned from register_sysctl_table
 *
 * Unregisters the sysctl table and all children. proc entries may not
 * actually be removed until they are no longer used by anyone.
 */
void unregister_sysctl_table(struct ctl_table_header *header)
{
	list_del(&header->ctl_entry);
#ifdef CONFIG_PROC_FS
	unregister_proc_table(header->ctl_table, proc_sys_root);
#endif
	kfree(header);
}

/*
 * /proc/sys support
 */

#ifdef CONFIG_PROC_FS


/** CALLBACK: DEFAULT FUNCTION TO READ FROM /PROC/SYS ENTRY
 *
 * This function is registered as read_proc be default for all sysctl
 * proc entries.
 *
 * \param dst   destination buffer to write the data to
 * \param start not supported yet
 * \param off   not supported yet
 * \param count max number of bytes to read
 * \param eof   not supported yet
 * \param data  data that is associated with the proc entry. In the case
 *              of the /proc/sys/.. entries it is the corresponding ctltable
 *
 * \return number of successfully read bytes or a negative error code
 */
static int proc_sys_default_read_proc(char *dst, char **start, off_t off,
                                      int max_len, int *eof, void *data)
{
	ctl_table *table;
	size_t cnt = max_len;
	struct file dummy_file;
	dummy_file.f_pos = 0;

	table = (ctl_table *) data;
	if (!table || !dst)
		return -1;

	if (table->proc_handler)
		table->proc_handler(table, 0, &dummy_file, dst, &cnt);
	else
		printf
			("Sysctl(proc_sys_default_read_proc): proc_handler undefined!\n");

	return cnt;
}



/* Scan the sysctl entries in table and add them all into /proc */
static void register_proc_table(ctl_table * table,
                                struct proc_dir_entry *root)
{
	struct proc_dir_entry *de;
	int len;
	mode_t mode;

	for (; table->ctl_name; table++) {
		/* Can't do anything without a proc name. */
		if (!table->procname)
			continue;
		/* Maybe we can't do anything with it... */
		if (!table->proc_handler && !table->child) {
			printk(KERN_WARNING "SYSCTL: Can't register %s\n",
			          table->procname);
			continue;
		}

		len = strlen(table->procname);
		mode = table->mode;

		de = NULL;
		if (table->proc_handler)
			mode |= S_IFREG;
		else {
			mode |= S_IFDIR;
			for (de = root->subdir; de; de = de->next) {
				if (proc_match(len, table->procname, de))
					break;
			}
			/* If the subdir exists already, de is non-NULL */
		}

		if (!de) {
			de = create_proc_entry(table->procname, mode, root);
			if (!de)
				continue;
			de->data = (void *)table;
			de->read_proc = &proc_sys_default_read_proc;

			if (table->proc_handler) {
				de->proc_fops = &proc_sys_file_operations;
				de->proc_iops = &proc_sys_inode_operations;
			}
		}
		table->de = de;
		if (de->mode & S_IFDIR)
			register_proc_table(table->child, de);
	}
}

/*
 * Unregister a /proc sysctl table and any subdirectories.
 */
static void unregister_proc_table(ctl_table * table,
                                  struct proc_dir_entry *root)
{
	struct proc_dir_entry *de;
	for (; table->ctl_name; table++) {
		if (!(de = table->de))
			continue;
		if (de->mode & S_IFDIR) {
			if (!table->child) {
				printk(KERN_ALERT
				           "Help - malformed sysctl tree on free\n");
				continue;
			}
			unregister_proc_table(table->child, de);

			/* Don't unregister directories which still have entries.. */
			if (de->subdir)
				continue;
		}

		/* Don't unregister proc entries that are still being used.. */
		if (atomic_read(&de->count))
			continue;

		table->de = NULL;
		remove_proc_entry(table->procname, root);
	}
}

static ssize_t do_rw_proc(int write, struct file *file, char *buf,
                          size_t count, loff_t * ppos)
{
	int op;
	struct proc_dir_entry *de;
	struct ctl_table *table;
	size_t res;
	ssize_t error;

	de = (struct proc_dir_entry *)file->f_dentry->d_inode->u.generic_ip;
	if (!de || !de->data)
		return -ENOTDIR;
	table = (struct ctl_table *)de->data;
	if (!table || !table->proc_handler)
		return -ENOTDIR;
	op = (write ? 002 : 004);

	res = count;

	/*
	 * FIXME: we need to pass on ppos to the handler.
	 */

	error = (*table->proc_handler) (table, write, file, buf, &res);
	if (error)
		return error;
	return res;
}

static ssize_t proc_readsys(struct file *file, char *buf,
                            size_t count, loff_t * ppos)
{
	return do_rw_proc(0, file, buf, count, ppos);
}

static ssize_t proc_writesys(struct file *file, const char *buf,
                             size_t count, loff_t * ppos)
{
	return do_rw_proc(1, file, (char *)buf, count, ppos);
}

static int proc_sys_permission(struct inode *inode, int op)
{
	return 0;
}

/**
 * proc_dostring - read a string sysctl
 * @table: the sysctl table
 * @write: %TRUE if this is a write to the sysctl file
 * @filp: the file structure
 * @buffer: the user buffer
 * @lenp: the size of the user buffer
 *
 * Reads/writes a string from/to the user buffer. If the kernel
 * buffer provided is not large enough to hold the string, the
 * string is truncated. The copied string is %NULL-terminated.
 * If the string is being read by the user process, it is copied
 * and a newline '\n' is added. It is truncated if the buffer is
 * not large enough.
 *
 * Returns 0 on success.
 */
int proc_dostring(ctl_table * table, int write, struct file *filp,
                  void *buffer, size_t * lenp)
{
	size_t len;
	char *p, c;

	if (!table->data || !table->maxlen || !*lenp ||
	     (filp->f_pos && !write)) {
		*lenp = 0;
		return 0;
	}

	if (write) {
		len = 0;
		p = buffer;
		while (len < *lenp) {
			if (get_user(c, p++))
				return -EFAULT;
			if (c == 0 || c == '\n')
				break;
			len++;
		}
		if (len >= table->maxlen)
			len = table->maxlen - 1;
		if (copy_from_user(table->data, buffer, len))
			return -EFAULT;
		((char *)table->data)[len] = 0;
		filp->f_pos += *lenp;
	} else {
		len = strlen(table->data);
		if (len > table->maxlen)
			len = table->maxlen;
		if (len > *lenp)
			len = *lenp;
		if (len)
			if (copy_to_user(buffer, table->data, len))
				return -EFAULT;
		if (len < *lenp) {
			//if(put_user('\n', ((char *) buffer) + len))
			//  return -EFAULT;
			if (buffer)
				*(((char *)buffer) + len) = '\n';
			else
				return -EFAULT;
			len++;
		}
		*lenp = len;
		filp->f_pos += len;
	}
	return 0;
}


#define OP_SET  0
#define OP_AND  1
#define OP_OR   2
#define OP_MAX  3
#define OP_MIN  4

static int do_proc_dointvec(ctl_table * table, int write,
                            struct file *filp, void *buffer, size_t * lenp,
                            int conv, int op)
{
	int *i, vleft, first = 1, neg, val;
	size_t left, len;
	char *b = (char *)buffer;

#define TMPBUFLEN 20
	char buf[TMPBUFLEN], *p;

	if (!table->data || !table->maxlen || !*lenp ||
	     (filp->f_pos && !write)) {
		*lenp = 0;
		return 0;
	}

	i = (int *)table->data;
	vleft = table->maxlen / sizeof(int);
	left = *lenp;

	for (; left && vleft--; i++, first = 0) {
		if (write) {
			while (left) {
				char c;
				if (get_user(c, b))
					return -EFAULT;
				if (!isspace(c))
					break;
				left--;
				b++;
			}
			if (!left)
				break;
			neg = 0;
			len = left;
			if (len > TMPBUFLEN - 1)
				len = TMPBUFLEN - 1;
			if (copy_from_user(buf, b, len))
				return -EFAULT;
			buf[len] = 0;
			p = buf;
			if (*p == '-' && left > 1) {
				neg = 1;
				left--, p++;
			}
			if (*p < '0' || *p > '9')
				break;
			val = simple_strtoul(p, &p, 0) * conv;
			len = p - buf;
			if ((len < left) && *p && !isspace(*p))
				break;
			if (neg)
				val = -val;
			b += len;
			left -= len;
			switch (op) {
			case OP_SET:
				*i = val;
				break;
			case OP_AND:
				*i &= val;
				break;
			case OP_OR:
				*i |= val;
				break;
			case OP_MAX:
				if (*i < val)
					*i = val;
				break;
			case OP_MIN:
				if (*i > val)
					*i = val;
				break;
			}
		} else {
			p = buf;
			if (!first)
				*p++ = '\t';
			sprintf(p, "%d", (*i) / conv);
			len = strlen(buf);
			if (len > left)
				len = left;
			if (copy_to_user(b, buf, len))
				return -EFAULT;
			left -= len;
			b += len;
		}
	}

	if (!write && !first && left) {
		//if(put_user('\n', b))
		//  return -EFAULT;
		if (b)
			*b = '\n';
		else
			return -EFAULT;
		left--, b++;
	}
	if (write) {
		p = b;
		while (left) {
			char c;
			if (get_user(c, p++))
				return -EFAULT;
			if (!isspace(c))
				break;
			left--;
		}
	}
	if (write && first)
		return -EINVAL;
	*lenp -= left;
	filp->f_pos += *lenp;
	return 0;
}

/**
 * proc_dointvec - read a vector of integers
 * @table: the sysctl table
 * @write: %TRUE if this is a write to the sysctl file
 * @filp: the file structure
 * @buffer: the user buffer
 * @lenp: the size of the user buffer
 *
 * Reads/writes up to table->maxlen/sizeof(unsigned int) integer
 * values from/to the user buffer, treated as an ASCII string. 
 *
 * Returns 0 on success.
 */
int proc_dointvec(ctl_table * table, int write, struct file *filp,
                  void *buffer, size_t * lenp)
{
	return do_proc_dointvec(table, write, filp, buffer, lenp, 1, OP_SET);
}

/*
 *  init may raise the set.
 */

int proc_dointvec_bset(ctl_table * table, int write, struct file *filp,
                       void *buffer, size_t * lenp)
{
	if (!capable(CAP_SYS_MODULE)) {
		return -EPERM;
	}
	return do_proc_dointvec(table, write, filp, buffer, lenp, 1,
	                         (current->pid == 1) ? OP_SET : OP_AND);
}

/**
 * proc_dointvec_minmax - read a vector of integers with min/max values
 * @table: the sysctl table
 * @write: %TRUE if this is a write to the sysctl file
 * @filp: the file structure
 * @buffer: the user buffer
 * @lenp: the size of the user buffer
 *
 * Reads/writes up to table->maxlen/sizeof(unsigned int) integer
 * values from/to the user buffer, treated as an ASCII string.
 *
 * This routine will ensure the values are within the range specified by
 * table->extra1 (min) and table->extra2 (max).
 *
 * Returns 0 on success.
 */
int proc_dointvec_minmax(ctl_table * table, int write, struct file *filp,
                         void *buffer, size_t * lenp)
{
	int *i, *min, *max, vleft, first = 1, neg, val;
	size_t len, left;
	char *b = (char *)buffer;
#define TMPBUFLEN 20
	char buf[TMPBUFLEN], *p;

	if (!table->data || !table->maxlen || !*lenp ||
	     (filp->f_pos && !write)) {
		*lenp = 0;
		return 0;
	}

	i = (int *)table->data;
	min = (int *)table->extra1;
	max = (int *)table->extra2;
	vleft = table->maxlen / sizeof(int);
	left = *lenp;

	for (; left && vleft--; i++, min++, max++, first = 0) {
		if (write) {
			while (left) {
				char c;
				if (get_user(c, b))
					return -EFAULT;
				if (!isspace(c))
					break;
				left--;
				b++;
			}
			if (!left)
				break;
			neg = 0;
			len = left;
			if (len > TMPBUFLEN - 1)
				len = TMPBUFLEN - 1;
			if (copy_from_user(buf, b, len))
				return -EFAULT;
			buf[len] = 0;
			p = buf;
			if (*p == '-' && left > 1) {
				neg = 1;
				left--, p++;
			}
			if (*p < '0' || *p > '9')
				break;
			val = simple_strtoul(p, &p, 0);
			len = p - buf;
			if ((len < left) && *p && !isspace(*p))
				break;
			if (neg)
				val = -val;
			b += len;
			left -= len;

			if ((min && val < *min) || (max && val > *max))
				continue;
			*i = val;
		} else {
			p = buf;
			if (!first)
				*p++ = '\t';
			sprintf(p, "%d", *i);
			len = strlen(buf);
			if (len > left)
				len = left;
			if (copy_to_user(b, buf, len))
				return -EFAULT;
			left -= len;
			b += len;
		}
	}

	if (!write && !first && left) {
		//if(put_user('\n', b))
		//  return -EFAULT;
		if (b)
			*b = '\n';
		else
			return -EFAULT;
		left--, b++;
	}
	if (write) {
		p = b;
		while (left) {
			char c;
			if (get_user(c, p++))
				return -EFAULT;
			if (!isspace(c))
				break;
			left--;
		}
	}
	if (write && first)
		return -EINVAL;
	*lenp -= left;
	filp->f_pos += *lenp;
	return 0;
}



/**
 * proc_dointvec_jiffies - read a vector of integers as seconds
 * @table: the sysctl table
 * @write: %TRUE if this is a write to the sysctl file
 * @filp: the file structure
 * @buffer: the user buffer
 * @lenp: the size of the user buffer
 *
 * Reads/writes up to table->maxlen/sizeof(unsigned int) integer
 * values from/to the user buffer, treated as an ASCII string. 
 * The values read are assumed to be in seconds, and are converted into
 * jiffies.
 *
 * Returns 0 on success.
 */
int proc_dointvec_jiffies(ctl_table * table, int write, struct file *filp,
                          void *buffer, size_t * lenp)
{
	return do_proc_dointvec(table, write, filp, buffer, lenp, HZ, OP_SET);
}

#else                           /* CONFIG_PROC_FS */

int proc_dostring(ctl_table * table, int write, struct file *filp,
                  void *buffer, size_t * lenp)
{
	return -ENOSYS;
}

//static int proc_doutsstring(ctl_table *table, int write, struct file *filp,
                              //              void *buffer, size_t *lenp)
//{
//  return -ENOSYS;
//}

int proc_dointvec(ctl_table * table, int write, struct file *filp,
                  void *buffer, size_t * lenp)
{
	return -ENOSYS;
}

int proc_dointvec_bset(ctl_table * table, int write, struct file *filp,
                       void *buffer, size_t * lenp)
{
	return -ENOSYS;
}

int proc_dointvec_minmax(ctl_table * table, int write, struct file *filp,
                         void *buffer, size_t * lenp)
{
	return -ENOSYS;
}

int proc_dointvec_jiffies(ctl_table * table, int write, struct file *filp,
                          void *buffer, size_t * lenp)
{
	return -ENOSYS;
}




#endif                          /* CONFIG_PROC_FS */


/*
 * General sysctl support routines 
 */

/* The generic string strategy routine: */
int sysctl_string(ctl_table * table, int *name, int nlen,
                  void *oldval, size_t * oldlenp,
                  void *newval, size_t newlen, void **context)
{
	size_t l, len;

	if (!table->data || !table->maxlen)
		return -ENOTDIR;

	if (oldval && oldlenp) {
		if (get_user(len, oldlenp))
			return -EFAULT;
		if (len) {
			l = strlen(table->data);
			if (len > l)
				len = l;
			if (len >= table->maxlen)
				len = table->maxlen;
			if (copy_to_user(oldval, table->data, len))
				return -EFAULT;
			//if(put_user(0, ((char *) oldval) + len))
			//  return -EFAULT;
			if (oldval)
				*(((char *)oldval) + len) = 0;
			else
				return -EFAULT;
			//            if(put_user(len, oldlenp))
			//                return -EFAULT;
			if (oldlenp)
				*oldlenp = len;
			else
				return -EFAULT;
		}
	}
	if (newval && newlen) {
		len = newlen;
		if (len > table->maxlen)
			len = table->maxlen;
		if (copy_from_user(table->data, newval, len))
			return -EFAULT;
		if (len == table->maxlen)
			len--;
		((char *)table->data)[len] = 0;
	}
	return 0;
}

/*
 * This function makes sure that all of the integers in the vector
 * are between the minimum and maximum values given in the arrays
 * table->extra1 and table->extra2, respectively.
 */
int sysctl_intvec(ctl_table * table, int *name, int nlen,
                  void *oldval, size_t * oldlenp,
                  void *newval, size_t newlen, void **context)
{
	int i, *vec, *min, *max;
	size_t length;

	if (newval && newlen) {
		if (newlen % sizeof(int) != 0)
			return -EINVAL;

		if (!table->extra1 && !table->extra2)
			return 0;

		if (newlen > table->maxlen)
			newlen = table->maxlen;
		length = newlen / sizeof(int);

		vec = (int *)newval;
		min = (int *)table->extra1;
		max = (int *)table->extra2;

		for (i = 0; i < length; i++) {
			int value;
			get_user(value, vec + i);
			if (min && value < min[i])
				return -EINVAL;
			if (max && value > max[i])
				return -EINVAL;
		}
	}
	return 0;
}

/* Strategy function to convert jiffies to seconds */
int sysctl_jiffies(ctl_table * table, int *name, int nlen,
                   void *oldval, size_t * oldlenp,
                   void *newval, size_t newlen, void **context)
{
	if (oldval) {
		size_t olen;
		if (oldlenp) {
			if (get_user(olen, oldlenp))
				return -EFAULT;
			if (olen != sizeof(int))
				return -EINVAL;
		}
		if (put_user(*(int *)(table->data) / HZ, (int *)oldval) ||
		      (oldlenp && put_user(sizeof(int), oldlenp)))
			return -EFAULT;
	}
	if (newval && newlen) {
		int new;
		if (newlen != sizeof(int))
			return -EINVAL;
		if (get_user(new, (int *)newval))
			return -EFAULT;
		*(int *)(table->data) = new * HZ;
	}
	return 1;
}

