/*
 * \brief   pass through plugin for VERNER's core
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

#include "ac_pass.h"

/* L4/OSKit includes */
#include <stdio.h>
#include <stdlib.h>
#include <l4/env/errno.h>



/*
 * initialize codec 
 */
int
aud_codec_pass_init (plugin_ctrl_t * attr, stream_info_t * info)
{
  strncpy (attr->info, "Passthrough only for audio", 32);

  if (attr->mode != PLUG_MODE_PASS)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }
  if (info->type != STREAM_TYPE_AUDIO)
  {
    LOG_Error ("MEDIA TYPE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }
  attr->minQLevel = attr->maxQLevel = 0;
  /* QAP supported */
  attr->supportsQAP = 0;
  return 0;
}


/*
 * processs next frame 
 */
inline int
aud_codec_pass_step (plugin_ctrl_t * attr, unsigned char *in_buffer,
		     unsigned char *out_buffer)
{
  frame_ctrl_t *frameattr;

  if (attr->mode != PLUG_MODE_PASS)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return CODING_ERROR;
  }
  if ((!in_buffer) || (!out_buffer))
  {
    LOG_Error ("got now data");
    return CODING_ERROR;
  }
  frameattr = (frame_ctrl_t *) in_buffer;
  /* set wanted format (we don't change anything) */
  frameattr->ai.format = attr->target_format;
  /* update packetsize */
  attr->packetsize = frameattr->framesize + sizeof (frame_ctrl_t);
  memcpy (out_buffer, in_buffer, attr->packetsize);


  return CODING_OK;
}

/*
 * close codec 
 */
int
aud_codec_pass_close (plugin_ctrl_t * attr)
{
  strncpy (attr->info, "cu, your simple plugin :)", 32);
  return 0;
}
