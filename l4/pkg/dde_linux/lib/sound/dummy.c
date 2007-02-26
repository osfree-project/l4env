/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux/lib/sound/dummy.c
 * \brief  Dummies
 *
 * \date   08/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 */
#include <l4/dde_linux/dde.h>

/* Linux */
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/poll.h>

/* local */
#include "__config.h"

/** poll dummy
 *
 * As there is no poll implementation/emulation fops->poll will never be called.
 *
 * See also 'man 2 poll'.
 */
void __pollwait(struct file * filp, wait_queue_head_t * wait_address,
                poll_table *p)
{}

/** default llseek if it's not supported by driver/device
 * from fs/read_write.c
 */
loff_t no_llseek(struct file *file, loff_t offset, int origin)
{
  return -ESPIPE;
}
