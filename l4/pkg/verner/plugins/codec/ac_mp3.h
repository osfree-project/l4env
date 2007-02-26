/*
 * \brief   mpg123lib plugin for VERNER's core
 * \date    2004-02-11
 * \author  Carsten Rietzschel <cr7@os.inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2003  Carsten Rietzschel  <cr7@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef _MP3_CODEC_AUDIO_H_
#define _MP3_CODEC_AUDIO_H_

#include "arch_globals.h"

int aud_codec_mp3_init (plugin_ctrl_t * attr, stream_info_t * info);
int aud_codec_mp3_step (plugin_ctrl_t * attr, unsigned char *in_buffer,
			unsigned char *out_buffer);
int aud_codec_mp3_close (plugin_ctrl_t * attr);

#endif
