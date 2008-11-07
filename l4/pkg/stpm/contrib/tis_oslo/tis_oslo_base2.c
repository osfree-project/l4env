/*
 * \brief   A TIS device driver for TPM v1.2 reusing the TIS driver of OSLO.
 *          Adapted original tis_oslo by using (extended) standard Linux TPM
 *          interface (see tpm.c).
 * \date    2008-11-5
 * \author  Bernhard Kauer <kauer@tudos.org>
 * \author  Alexander Boettcher <boettcher@tudos.org>
 */
/*
 * Copyright (C) 2008  Bernhard Kauer <kauer@tudos.org> and Alexander Boettcher
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
#include "tpm.h"

#define TPM_MINOR_NR     224
#define MODULE_NAME      "tis_oslo"
#define TPM_DEVICE_NAME  "tpm"
#define MAX_BUFFER_SIZE  2048
#define error(X,...) printk(KERN_ERR MODULE_NAME " %s() " X "\n", __func__ ,##__VA_ARGS__)


struct tpm_data {
  void * base;
  unsigned long count;
  unsigned long used;
  u8 buf[MAX_BUFFER_SIZE];
};

static struct tpm_data tpm_data;
static struct platform_device *pdev;


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

static void tpm_inf_cancel(struct tpm_chip *chip)
{
	//TODO
}

static int tpm_inf_send(struct tpm_chip *chip, u8 * buf, size_t count)
{
  int res;

  error("sending count=%x", count);  
  if (count == 0)
    return 0;

  if (tpm_data.count != 0)
    {
      error("write with read data");
      tpm_data.count = 0;
    }
/*
  if (count > sizeof(tpm_data.buf))
    count = sizeof(tpm_data.buf);

  if (copy_from_user(tpm_data.buf, buf, count))
    return -EFAULT;
*/
  res = tis_transmit(buf, count, tpm_data.buf, MAX_BUFFER_SIZE);

  error("sending res=%x", res);  
  tpm_data.count = res;
  if (res < 0)
    {
      tpm_data.count = 0;
      return res;
    }

  return count;
}

static u8 tpm_inf_status(struct tpm_chip *chip)
{
  //TODO
	return 0;
}

static int tpm_inf_recv(struct tpm_chip *chip, u8 * buf, size_t count)
{
  error("receiving count=%x buf=%p", count, buf);  
  if (count == 0)
    return 0;

  if (count > tpm_data.count)
    count = tpm_data.count;
  else if (count > tpm_data.count)
    error("read not enough bytes");

  tpm_data.count = 0;
/*  if (copy_to_user(buf, tpm_data.buf, count))
    return -EFAULT;
*/
  memcpy(buf, tpm_data.buf, count);

  error("receiving count=%x finished", count);  
	
  return count;
}

static DEVICE_ATTR(pubek, S_IRUGO, tpm_show_pubek, NULL);
static DEVICE_ATTR(pcrs, S_IRUGO, tpm_show_pcrs, NULL);
static DEVICE_ATTR(caps, S_IRUGO, tpm_show_caps, NULL);
static DEVICE_ATTR(cancel, S_IWUSR | S_IWGRP, NULL, tpm_store_cancel);

static struct attribute *oslo_attrs[] = {
  &dev_attr_pubek.attr,
  &dev_attr_pcrs.attr,
  &dev_attr_caps.attr,
  &dev_attr_cancel.attr,
  NULL,
};

static struct attribute_group oslo_attr_grp = {.attrs = oslo_attrs };

static const struct file_operations tpm_fops = {
  owner   : THIS_MODULE,
  llseek  : no_llseek,
  open    : tpm_open,
  read    : tpm_read,
  write   : tpm_write,
  release : tpm_release,
};

static const struct tpm_vendor_specific tpm_oslo = {
  .recv = tpm_inf_recv,
  .send = tpm_inf_send,
  .cancel = tpm_inf_cancel,
  .status = tpm_inf_status,
  .req_complete_mask = 0,
  .req_complete_val = 0,
  .attr_group = &oslo_attr_grp,
  .miscdev = {.fops = &tpm_fops,},
};

/*
static struct miscdevice tpm_dev = {
	TPM_MINOR_NR,
	MODULE_NAME,
	&tpm_fops
};
*/
static struct device_driver tpm_dev = {
	.name = MODULE_NAME,
	.bus = &platform_bus_type,
	.owner = THIS_MODULE,
	.suspend = 0, //tpm_pm_suspend,
	.resume = 0, //tpm_pm_resume,
};

static int __init
tpm_mod_init(void)
{  
  int res;
  int rc;
	struct tpm_chip *chip;

  if (!request_mem_region(TIS_BASE, 0x1000, "tpm"))
    return -EBUSY;
    
  tpm_data.base = ioremap(TIS_BASE, 0x1000);
  printk("tis_oslo: ptr %p\n",tpm_data.base);
  if (tpm_data.base == 0)
  {
    release_mem_region(TIS_BASE, 0x1000);
    return -EBUSY;
  }

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

    rc = driver_register(&tpm_dev);
    if (rc < 0)
      return rc;

    if (IS_ERR(pdev = platform_device_register_simple(TPM_DEVICE_NAME,
                                                    -1, NULL, 0)))
      return PTR_ERR(pdev);

    if (!(chip = tpm_register_hardware(&pdev->dev, &tpm_oslo)))
      return -ENODEV;

    return 0;
  } while (0);

  iounmap(tpm_data.base);
  release_mem_region(TIS_BASE, 0x1000);
  return res;
}

static void __exit
tpm_mod_exit(void)
{
	platform_device_unregister(pdev);
	driver_unregister(&tpm_dev);

  iounmap(tpm_data.base);
  release_mem_region(TIS_BASE, 0x1000);
}


module_init(tpm_mod_init);
module_exit(tpm_mod_exit);

MODULE_AUTHOR("Bernhard Kauer <kauer@tudos.org> Alexander Boettcher <boettcher@tudos.org>");
MODULE_DESCRIPTION("Driver for v1.2 TPMs");
MODULE_LICENSE("GPL");

