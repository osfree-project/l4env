/* $Id$ */
/*****************************************************************************/
/**
 * \file	dde_test/lib/src/sound/dummy.c
 *
 * \brief	dummies
 *
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4 */
#include <l4/dde_linux/dde.h>

/* Linux */
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/poll.h>

/* local includes */
#include "__config.h"
#include "__macros.h"

/*****************************************************************************/
/** poll dummy
 *
 * As there is no poll implementation/emulation fops->poll will never be called.
 *
 * See also 'man 2 poll'. */
/*****************************************************************************/
void __pollwait(struct file * filp, wait_queue_head_t * wait_address, poll_table *p)
{
}

/** default llseek if it's not supported by driver/device
 * from fs/read_write.c */
loff_t no_llseek(struct file *file, loff_t offset, int origin)
{
  return -ESPIPE;
}
