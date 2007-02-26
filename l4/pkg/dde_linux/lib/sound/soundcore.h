/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux/lib/sound/soundcore.h
 * \brief  Linux DDE Soundcore
 *
 * \date   08/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __DDE_LINUX_LIB_SOUND_SOUNDCORE_H_
#define __DDE_LINUX_LIB_SOUND_SOUNDCORE_H_

#define TYPE_MAX        3 /**< maximum number of device types */
# define TYPE_DSP       0
# define TYPE_MIXER     1
# define TYPE_MIDI      2   /* unused */
#define NUM_MAX         4 /**< maximum number of devices per type */

struct file_operations* soundcore_req_fops(int type, int num);
void soundcore_rel_fops(int type, int num);

#endif
