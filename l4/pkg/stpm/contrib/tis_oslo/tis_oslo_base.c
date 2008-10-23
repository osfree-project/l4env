/*
 * \brief   A TIS device driver for TPM v1.2 reusing the TIS driver of OSLO.
 * \date    2006-11-14
 * \author  Bernhard Kauer <kauer@tudos.org>
 */
/*
 * Copyright (C) 2006  Bernhard Kauer <kauer@tudos.org>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the TIS_OSLO package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */


#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include "tis.h"

#define TPM_MINOR_NR     224
#define MODULE_NAME      "tis_oslo"
#define MAX_BUFFER_SIZE  2048
#define error(X,...) printk(KERN_ERR MODULE_NAME " %s() " X "\n", __func__ ,##__VA_ARGS__)


struct tpm_data {
  void * base;
  unsigned long count;
  unsigned long used;
  u8 buf[MAX_BUFFER_SIZE];
};

static struct tpm_data tpm_data;


/**
 * helper functions
 */

void
out_string(const char *value)
{
  printk("%s",value);
}

void
out_description(char *prefix, unsigned int value)
{
  printk(MODULE_NAME ": %s %x\n", prefix, value);
}

void
out_info(char *msg)
{
  printk(MODULE_NAME ": %s\n", msg);
}

void
_exit(unsigned status)
{
  out_description("exit()", status);
  while (1)
    schedule();
}

void wait(int ms)
{
  msleep(ms);
}

static int
tpm_fops_open(struct inode *inode, struct file *file)
{
  if (tpm_data.used)
    return -EBUSY;
  tpm_data.used = 1;

  return 0;
}

static int
tpm_fops_release(struct inode *inode, struct file *file)
{
  tpm_data.used = 0;
  return 0;
}

static ssize_t
tpm_fops_read(struct file *file, char *buf, size_t count, loff_t *pos)
{
  if (count == 0)
    return 0;

  if (count > tpm_data.count)
    count = tpm_data.count;
  else if (count > tpm_data.count)
    error("read not enough bytes");

  tpm_data.count = 0;
  if (copy_to_user(buf, tpm_data.buf, count))
    return -EFAULT;
	
  return count;
}


static ssize_t
tpm_fops_write(struct file *file, const char *buf, size_t count, loff_t *pos)
{
  int res;
  
  if (count == 0)
    return 0;

  if (tpm_data.count != 0)
    {
      error("write with read data");
      tpm_data.count = 0;
    }

  if (count > sizeof(tpm_data.buf))
    count = sizeof(tpm_data.buf);

  if (copy_from_user(tpm_data.buf, buf, count))
    return -EFAULT;

  res = tis_transmit(tpm_data.buf, count, tpm_data.buf, MAX_BUFFER_SIZE);

  tpm_data.count = res;
  if (res < 0)
    {
      tpm_data.count = 0;
      return res;
    }
  return count;
}


static int
tpm_fops_ioctl(struct inode *inode, struct file *file,
		unsigned int cmd, unsigned long arg)
{
  return -ENODEV;
}

static 
struct file_operations tpm_fops = {
	owner:		THIS_MODULE,
	open:		tpm_fops_open,
	read:		tpm_fops_read,
	write:		tpm_fops_write,
	ioctl:		tpm_fops_ioctl,
	release:	tpm_fops_release
};


static struct miscdevice tpm_dev = {
	TPM_MINOR_NR,
	MODULE_NAME,
	&tpm_fops
};

static int __init
tpm_mod_init(void)
{  
  int res;

  if (!request_mem_region(TIS_BASE, 0x1000, "tpm"))
    return -EBUSY;
    
  tpm_data.base = ioremap(TIS_BASE, 0x1000);
  printk("tis_oslo: ptr %p\n",tpm_data.base);

  do {
    if (!tis_init((unsigned long) tpm_data.base))
      {
	error("no tpm found");
	res = -ENODEV;
	break;
      }    

    if (!tis_access(TIS_LOCALITY_0, 0))
      {
	error("tis_access failed");
	res = -EINVAL;
	break;
      }

    if ((res = misc_register(&tpm_dev))) {
      error("unable to misc_register minor %d error %d", TPM_MINOR_NR, res);
      res = -ENODEV;
      break;
    }    

    return 0;
  } while (0);

  iounmap(tpm_data.base);
  release_mem_region(TIS_BASE, 0x1000);
  return res;
}

static void __exit
tpm_mod_exit(void)
{
  misc_deregister(&tpm_dev);
  iounmap(tpm_data.base);
  release_mem_region(TIS_BASE, 0x1000);
}


module_init(tpm_mod_init);
module_exit(tpm_mod_exit);

MODULE_AUTHOR("Bernhard Kauer <kauer@tudos.org>");
MODULE_DESCRIPTION("Driver for v1.2 TPMs");
MODULE_LICENSE("GPL");

