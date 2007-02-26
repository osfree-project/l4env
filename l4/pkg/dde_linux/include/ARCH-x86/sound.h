/* $Id$ */
/*****************************************************************************/
/**
 * \file	dde_linux/include/l4/dde_linux/sound.h
 *
 * \brief	Linux DDE Sound Driver Environment
 *
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

#ifndef _L4DDE_SOUND_H
#define _L4DDE_SOUND_H

#include <l4/sys/l4int.h>

int l4dde_snd_init(void);
/* exitcalls are still an open issue; this does nothing */
int l4dde_snd_exit(void);

int l4dde_snd_open_dsp(int num);
int l4dde_snd_open_mixer(int num);
int l4dde_snd_close(int dev);
int l4dde_snd_write(int dev, const void *buf, int count);
int l4dde_snd_read(int dev, void *buf, int count);
int l4dde_snd_ioctl(int dev, int req, l4_addr_t arg);

#endif /* !_L4DDE_SOUND_H */
