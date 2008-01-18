/* Software-Based Trusted Platform Module (TPM) Emulator for Linux 
 * Copyright (C) 2004 Mario Strasser <mast@gmx.net>,
 *
 * This module is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published 
 * by the Free Software Foundation; either version 2 of the License, 
 * or (at your option) any later version.  
 *
 * This module is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details.
 *
 * $Id: linux_module.c 158 2006-12-03 10:44:02Z mast $
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>
#include "tpm_emulator_config.h"
#include "tpm/tpm_emulator.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mario Strasser <mast@gmx.net>");
MODULE_DESCRIPTION("Trusted Platform Module (TPM) Emulator");
MODULE_SUPPORTED_DEVICE(TPM_DEVICE_NAME);

/* module startup parameters */
char *startup = "save";
module_param(startup, charp, 0444);
MODULE_PARM_DESC(startup, " Sets the startup mode of the TPM. "
  "Possible values are 'clear', 'save' (default) and 'deactivated.");
char *storage_file = "/var/tpm/tpm_emulator-1.2."
  TPM_STR(VERSION_MAJOR) "." TPM_STR(VERSION_MINOR);
module_param(storage_file, charp, 0644);
MODULE_PARM_DESC(storage_file, " Sets the persistent-data storage file of the TPM.");

/* TPM lock */
static struct semaphore tpm_mutex;

/* TPM command response */
static struct {
  uint8_t *data;
  uint32_t size;
} tpm_response;

/* module state */
#define STATE_IS_OPEN 0
static uint32_t module_state;
static struct timespec old_time;

static int tpm_open(struct inode *inode, struct file *file)
{
  debug("%s()", __FUNCTION__);
  if (test_and_set_bit(STATE_IS_OPEN, (void*)&module_state)) return -EBUSY;
  return 0;
}

static int tpm_release(struct inode *inode, struct file *file)
{
  debug("%s()", __FUNCTION__);
  clear_bit(STATE_IS_OPEN, (void*)&module_state);
  down(&tpm_mutex);
  if (tpm_response.data != NULL) {
    kfree(tpm_response.data);
    tpm_response.data = NULL;
  }
  up(&tpm_mutex);
  return 0;
}

static ssize_t tpm_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
  debug("%s(%zd)", __FUNCTION__, count);
  down(&tpm_mutex);
  if (tpm_response.data != NULL) {
    count = min(count, (size_t)tpm_response.size - (size_t)*ppos);
    count -= copy_to_user(buf, &tpm_response.data[*ppos], count);
    *ppos += count;
    if ((size_t)tpm_response.size == (size_t)*ppos) {
      kfree(tpm_response.data);
      tpm_response.data = NULL;
    }
  } else {
    count = 0;
  }
  up(&tpm_mutex);
  return count;
}

static ssize_t tpm_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
  debug("%s(%zd)", __FUNCTION__, count);
  down(&tpm_mutex);
  *ppos = 0;
  if (tpm_response.data != NULL) {
    kfree(tpm_response.data);
    tpm_response.data = NULL;
  }
  if (tpm_handle_command(buf, count, &tpm_response.data, &tpm_response.size) != 0) { 
    count = -EILSEQ;
    tpm_response.data = NULL;
  }
  up(&tpm_mutex);
  return count;
}

#define TPMIOC_CANCEL   _IO('T', 0x00)
#define TPMIOC_TRANSMIT _IO('T', 0x01)

static int tpm_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
  debug("%s(%d, %p)", __FUNCTION__, cmd, (char*)arg);
  if (cmd == TPMIOC_TRANSMIT) {
    uint32_t count = ntohl(*(uint32_t*)(arg + 2));
    down(&tpm_mutex);
    if (tpm_response.data != NULL) {
      kfree(tpm_response.data);
      tpm_response.data = NULL;
    }
    if (tpm_handle_command((char*)arg, count, &tpm_response.data, &tpm_response.size) == 0) {
      tpm_response.size -= copy_to_user((char*)arg, tpm_response.data, tpm_response.size);
      kfree(tpm_response.data);
      tpm_response.data = NULL;
    } else {
      tpm_response.size = 0;
      tpm_response.data = NULL;
    }
    up(&tpm_mutex);
    return tpm_response.size;
  }
  return -1;
}

struct file_operations fops = {
  .owner   = THIS_MODULE,
  .open    = tpm_open,
  .release = tpm_release,
  .read    = tpm_read,
  .write   = tpm_write,
  .ioctl   = tpm_ioctl,
};

static struct miscdevice tpm_dev = {
  .minor      = TPM_DEVICE_MINOR, 
  .name       = TPM_DEVICE_NAME, 
  .fops       = &fops,
};

int __init init_tpm_module(void)
{
  int res = misc_register(&tpm_dev);
  if (res != 0) {
    error("misc_register() failed for minor %d\n", TPM_DEVICE_MINOR);
    return res;
  }
  /* initialize variables */
  sema_init(&tpm_mutex, 1);
  module_state = 0;
  tpm_response.data = NULL;
  old_time = current_kernel_time();
  /* initialize TPM emulator */
  if (!strcmp(startup, "clear")) {
    tpm_emulator_init(1);
  } else if (!strcmp(startup, "save")) {
    tpm_emulator_init(2);
  } else if (!strcmp(startup, "deactivated")) {
    tpm_emulator_init(3);
  } else {
    error("invalid startup mode '%s'; must be 'clear', "
      "'save' (default) or 'deactivated", startup);
    misc_deregister(&tpm_dev);
    return -EINVAL;
  }
  return 0;
}

void __exit cleanup_tpm_module(void)
{
  tpm_emulator_shutdown();
  misc_deregister(&tpm_dev);
  if (tpm_response.data != NULL) kfree(tpm_response.data);
}

module_init(init_tpm_module);
module_exit(cleanup_tpm_module);

uint64_t tpm_get_ticks(void)
{
  struct timespec new_time = current_kernel_time();
  uint64_t ticks = (uint64_t)(new_time.tv_sec - old_time.tv_sec) * 1000000
                   + (new_time.tv_nsec - old_time.tv_nsec) / 1000;
  old_time = new_time;
  return (ticks > 0) ? ticks : 1;
}

#define TPM_STORE_TO_FILE
#ifdef TPM_STORE_TO_FILE

#include <linux/fs.h>
#include <linux/unistd.h>
#include <asm/uaccess.h>

int tpm_write_to_file(uint8_t *data, size_t data_length)
{
  int res;
  struct file *fp;
  mm_segment_t old_fs = get_fs();
  fp = filp_open(storage_file, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
  if (IS_ERR(fp)) return -1;
  set_fs(get_ds());
  res = fp->f_op->write(fp, data, data_length, &fp->f_pos);
  set_fs(old_fs);
  filp_close(fp, NULL);
  return (res == data_length) ? 0 : -1;
}

int tpm_read_from_file(uint8_t **data, size_t *data_length)
{
  int res;
  struct file *fp;
  mm_segment_t old_fs = get_fs();
  fp = filp_open(storage_file, O_RDONLY, 0);
  if (IS_ERR(fp)) return -1;
  *data_length = (size_t)fp->f_dentry->d_inode->i_size;
  /* *data_length = i_size_read(fp->f_dentry->d_inode); */
  *data = tpm_malloc(*data_length);
  if (*data == NULL) {
    filp_close(fp, NULL);
    return -1;
  }
  set_fs(get_ds());
  res = fp->f_op->read(fp, *data, *data_length, &fp->f_pos);
  set_fs(old_fs);
  filp_close(fp, NULL);
  if (res != *data_length) {
    tpm_free(*data);
    return -1;
  }
  return 0;
}

#else

int tpm_write_to_file(uint8_t *data, size_t data_length)
{
  info("TPM_STORE_TO_FILE disabled, no data written");
  return -1;
}

int tpm_read_from_file(uint8_t **data, size_t *data_length)
{
  info("TPM_STORE_TO_FILE disabled, no data read");
  return -1;
}

#endif /* TPM_STORE_TO_FILE */
