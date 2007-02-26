/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux/lib/sound/helpers.c
 * \brief  Linux DDE Soundcore Helpers
 *
 * \date   08/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/** \ingroup mod_sound
 * \defgroup mod_sound_helpers Linux DDE Soundcore Helpers
 *
 * Linux soundcore emulation helpers.
 *
 * This is a highly sophisticated VFS layer emulation and initialization
 * implementation for <em>Linux DDE Soundcore</em>. *haha*
 */

/* L4 */
#include <l4/env/errno.h>

#include <l4/dde_linux/dde.h>
#include <l4/dde_linux/sound.h>

/* Linux */
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/soundcard.h>

/* local */
#include "__config.h"
#include "internal.h"

#include "soundcore.h"

/** initialization flag */
static int _initialized = 0;

/** all sound devices */
static struct devs
{
  struct file file;
  struct inode inode;
  struct file_operations *fops;
} devs[TYPE_MAX][NUM_MAX];

/** show file operation pointers */
static inline void show_fops(struct file_operations* f)
{
  DMSG("  open @ %p\n", f->open);
  DMSG(" close @ %p\n", f->release);
  DMSG("  read @ %p\n", f->read);
  DMSG(" write @ %p\n", f->write);
  DMSG(" ioctl @ %p\n", f->ioctl);
}

/**************************************************/

/** internal open */
static int snd_open_dev(int type, int num)
{
  int ret;
  struct file_operations* fops;
  struct file *file;
  struct inode *inode;

  fops = soundcore_req_fops(type, num);
  if (!fops)
    return -L4_ENOTFOUND; /* ENODEV */

  devs[type][num].fops = fops;
  file = &devs[type][num].file;
  inode = &devs[type][num].inode;

  file->f_mode = FMODE_READ | FMODE_WRITE;
  file->f_flags = O_RDWR;
  file->f_pos = 0;

  if ((ret=fops->open(inode, file)))
    {
      ERROR("fops->open returned %d", ret);
      return -L4_EOPEN;
    }

#if DEBUG_SOUND
  DMSG("%s%d opened\n",
      type ? "mixer" : "dsp",
      num);
  show_fops(fops);
#endif

  return (type*NUM_MAX + num);
}

/** Open dsp
 * \ingroup mod_sound_helpers
 */
int l4dde_snd_open_dsp(int num)
{
  return snd_open_dev(TYPE_DSP, num);
}

/** Open mixer
 * \ingroup mod_sound_helpers
 */
int l4dde_snd_open_mixer(int num)
{
  return snd_open_dev(TYPE_MIXER, num);
}

/** Close DSP unit
 * \ingroup mod_sound_helpers
 */
int l4dde_snd_close(int dev)
{
  int ret=0;
  struct devs *d = (struct devs *) devs;
  struct file_operations* fops = d[dev].fops;
  struct file *file;
  struct inode *inode;

  ASSERT(fops);
  if (!fops)
    return -L4_ESKIPPED;

  file = &d[dev].file;
  inode = &d[dev].inode;

  /* should always return 0 */
  if (fops->release)
    ret = fops->release(inode, file);

#if DEBUG_SOUND
  DMSG("device closed (%d)\n", ret);
#endif

  return 0;
}

/** Read from sound unit
 * \ingroup mod_sound_helpers
 */
int l4dde_snd_read(int dev, void *buf, int count)
{
  int ret;
  struct devs *d = (struct devs *) devs;
  struct file_operations* fops = d[dev].fops;
  struct file *file;

  ASSERT(fops);
  if (!fops)
    return -L4_ESKIPPED;

  file = &d[dev].file;

  if (fops->read)
    ret = fops->read(file, buf, count, &file->f_pos);
  else
    ret = -L4_EINVAL;

#if DEBUG_SOUND_READ
  DMSG("read from device (%d)\n", ret);
#endif

  return ret;
}

/** Write to sound unit
 * \ingroup mod_sound_helpers
 */
int l4dde_snd_write(int dev, const void *buf, int count)
{
  int ret;
  struct devs *d = (struct devs *) devs;
  struct file_operations* fops = d[dev].fops;
  struct file *file;

  ASSERT(fops);
  if (!fops)
    return -L4_ESKIPPED;

  file = &d[dev].file;

  if (fops->write)
    ret = fops->write(file, buf, count, &file->f_pos);
  else
    ret = -L4_EINVAL;

#if DEBUG_SOUND_WRITE
  DMSG("write on device (%d)\n", ret);
#endif

  return ret;
}

/** Execute ioctl() for unit
 * \ingroup mod_sound_helpers
 */
int l4dde_snd_ioctl(int dev, int req, l4_addr_t arg)
{
  int ret;
  struct devs *d = (struct devs *) devs;
  struct file_operations* fops = d[dev].fops;
  struct file *file;
  struct inode *inode;

  ASSERT(fops);
  if (!fops)
    return -L4_ESKIPPED;

  file = &d[dev].file;
  inode = &d[dev].inode;

  if (fops->ioctl)
    ret = fops->ioctl(inode, file, req, arg);
  else
    ret = -L4_EINVAL;

#if DEBUG_SOUND
  DMSG("ioctl on device (%d)\n", ret);
#endif

  return ret;
}

/** Initialize soundcore emulation
 * \ingroup mod_sound_helpers
 */
int l4dde_snd_init()
{
  if (_initialized)
    return -L4_ESKIPPED;

  ++_initialized;
  return 0;
}

/** Finalize soundcore emulation
 * \ingroup mod_sound_helpers
 */
int l4dde_snd_exit()
{
  if (!_initialized)
    return -L4_ESKIPPED;

  --_initialized;
  return 0;
}
