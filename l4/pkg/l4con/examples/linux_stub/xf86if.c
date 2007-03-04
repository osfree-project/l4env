/*!
 * \file	con/examples/linux_stub/xf86if.c
 * \brief	interface to XFree86 driver
 *
 * \date	01/2002
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include <l4/util/l4_macros.h>

#include <linux/version.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>

#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
#include <linux/module.h>
#endif

#include "dropscon.h"
#include "xf86if.h"

/* != 0 if redraw is done by X */
int xf86used = 0;
int xf86arg  = 0;

static struct fasync_struct *fasync = 0;
static struct task_struct *sig_proc = 0;

static int
xf86if_f_open(struct inode *inode, struct file *file)
{
  xf86used = 1;
  sig_proc = current;
  return 0;
}

/* X-server has closed connection to our device so from now on, we are
 * responsible again for screen output */
static int
xf86if_f_release(struct inode *inode, struct file *file)
{
  xf86used = 0;
  dropscon_redraw_all();
  return 0;
}

static int
xf86if_f_ioctl(struct inode *inode, struct file *filp, 
	       unsigned int cmd, unsigned long arg)
{
  switch (_IOC_NR(cmd))
    {
    case 1:
      if (copy_to_user((void*)arg, &dropsvc_l4id, sizeof(dropsvc_l4id)))
	return -EFAULT;
      break;
    case 2:
      if (copy_to_user((void*)arg, &con_l4id, sizeof(con_l4id)))
	return -EFAULT;
      break;
    case 3:
      xf86used--;
      dropscon_clear_gap();
      break;
    case 4:
      xf86used++;
      break;
    case 5:
      if (copy_to_user((void*)arg, &xf86arg, sizeof(xf86arg)))
	return -EFAULT;
      break;
    default:
      printk("unknown command %x\n", cmd);
      return -EINVAL;
    }

  return 0;
}

static ssize_t
xf86if_f_read(struct file *f, char *buf, size_t count, loff_t *ppos)
{
  int error;
  char *page;
  unsigned long length;

  if (!(page = (char*)__get_free_page(GFP_KERNEL)))
    return -ENOMEM;
  
  length = sprintf(page, "server: "l4util_idfmt"\n"
			 "vc: "l4util_idfmt"\n"
			 "flags: %08x\n"
			 "open: %d\n",
			 l4util_idstr(con_l4id),
			 l4util_idstr(dropsvc_l4id),
			 accel_flags,
			 xf86used-1 /* because open before read is possible */
		   );

  if (length <= 0)
    {
      error = length;
      goto done;
    }

  if (*ppos >= length)
    {
      error = 0;
      goto done;
    }

  if (count + *ppos > length)
    count = length - *ppos;
		
  copy_to_user(buf, page + *ppos, count);
  *ppos += count;

  error = count;

done:
  free_page((unsigned long) page);
  
  return error;
}

static int
xf86if_f_async(int fd, struct file *filp, int on)
{
  struct fasync_struct *fa, **fp;
  unsigned long flags;

  for (fp = &fasync; (fa = *fp) != NULL; fp = &fa->fa_next)
    {
      if (fa->fa_file == filp)
	break;
    }
  
  if (on) 
    {
      if (fa) 
	{
	  fa->fa_fd = fd;
	  return 0;
	}
      fa = kmalloc(sizeof(struct fasync_struct), GFP_KERNEL);
      if (!fa)
	return -ENOMEM;
      fa->magic = FASYNC_MAGIC;
      fa->fa_file = filp;
      fa->fa_fd = fd;
#ifdef L4L24
#warning Bah, this looks suspicious for 2.4 too (think UX) (+ same below)
#endif
      save_flags(flags);
      cli();
      fa->fa_next = fasync;
      fasync = fa;
      restore_flags(flags);
      return 1;
    }
  if (!fa)
    return 0;
  save_flags(flags);
  cli();
  *fp = fa->fa_next;
  restore_flags(flags);
  kfree(fa);
  return 1;
}

static struct file_operations xf86if_fops =
{
#if LINUX_VERSION_CODE >=  KERNEL_VERSION(2,4,0)
  owner:   THIS_MODULE,
#endif
  read:    xf86if_f_read,
  ioctl:   xf86if_f_ioctl,
  open:    xf86if_f_open,
  release: xf86if_f_release,
  fasync:  xf86if_f_async,
};

#ifdef L4L22
static struct inode_operations xf86if_iops =
{
	default_file_ops: &xf86if_fops,
};

static struct proc_dir_entry entry_dropscon =
{
	low_ino:	0,
	namelen:	8,
	name:		"dropscon",
	mode:		S_IRUGO | S_IFREG,
	nlink:		1,
	uid:		0,
	gid:		0,
	size:		0, 
	ops:		&xf86if_iops,
};
#endif

int
xf86if_handle_redraw_event(void)
{
  if (fasync && xf86used)
    {
      xf86arg = 1; /* map FB, redraw X screen */
#ifdef L4L22
      kill_fasync(fasync, SIGIO);
#else
      kill_fasync(&fasync, SIGIO, 0);
#endif
      send_sig(SIGUSR1, sig_proc, 1);
      
      return 1;
    }

  /* no -> let comh thread do it */
  return 0;
}

int
xf86if_handle_background_event(void)
{
  if (fasync && xf86used)
    {
      xf86arg = 2;
#ifdef L4L22
      kill_fasync(fasync, SIGIO);
#else
      kill_fasync(&fasync, SIGIO, 0);
#endif
    }

  return 0;
}

int
xf86if_init(void)
{
#ifdef L4L22
  proc_register(&proc_root, &entry_dropscon);
#else
  struct proc_dir_entry *d =
      create_proc_entry("dropscon", S_IRUGO | S_IFREG, &proc_root);
  if (d)
    d->proc_fops = &xf86if_fops;
#endif

  return 0;
}


void
xf86if_done(void)
{
#ifdef L4L22
  proc_unregister(&entry_dropscon, 1);
#else
  remove_proc_entry("dropscon", &proc_root);
#endif
}
