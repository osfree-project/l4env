/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux/include/ARCH-x86/sound.h
 * \brief  Linux DDE Sound Driver Environment
 *
 * \date   08/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __DDE_LINUX_INCLUDE_ARCH_X86_SOUND_H_
#define __DDE_LINUX_INCLUDE_ARCH_X86_SOUND_H_

#include <l4/sys/l4int.h>

int l4dde_snd_init(void);
/* XXX exitcalls are still an open issue; this does nothing */
int l4dde_snd_exit(void);

int l4dde_snd_open_dsp(int num);
int l4dde_snd_open_mixer(int num);
int l4dde_snd_close(int dev);
int l4dde_snd_write(int dev, const void *buf, int count);
int l4dde_snd_read(int dev, void *buf, int count);
int l4dde_snd_ioctl(int dev, int req, l4_addr_t arg);

#endif
