/*
 * \brief   Codecs setup for VERNER's core
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

#ifndef _CODEC_SETUP_H_
#define _CODEC_SETUP_H_

/* configuration */
#include "verner_config.h"

/* local */
#include "types.h"

int determineAndSetupCodec (control_struct_t * control);

/* the implementation is in audio_codecs.c (for VCore-Audio) or video_codecs.c (for VCore-Video) */

#endif
