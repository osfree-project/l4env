/*
 * \brief   LAME plugin for VERNER's core
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

/*
 * parts of LAME examples used (GPL)
 */

/* L4 includes */
#include <stdio.h>
#include <stdlib.h>
#include <l4/env/errno.h>
#include <l4/util/rand.h>


/* verner */
#include "arch_globals.h"

/* mp3 includes */
#include "ac_lame.h"
#include <lame.h>


/* 
 * initalize codec
 */
int
aud_codec_lame_init (plugin_ctrl_t * attr, stream_info_t * info)
{
  lame_global_flags *lame_gf;

  strncpy (attr->info, "LAME Encoder", 32);
  if (attr->mode != PLUG_MODE_ENC)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }

  attr->handle =
    (struct lame_global_flags *) malloc (sizeof (lame_global_flags));
  if (!attr->handle)
  {
    LOG_Error ("malloc failed!");
    return -L4_ENOMEM;
  }
  lame_gf = (lame_global_flags *) attr->handle;


  lame_init (lame_gf);		/* initialize libmp3lame */

  lame_version (lame_gf, attr->info);

  lame_gf->inPath = "";		/* name of input file */
  lame_gf->outPath = "";	/* name of output file. */
  lame_gf->input_format = sf_raw;
  lame_gf->gtkflag = 0;
  lame_gf->quality = 5;
/* FIXME - XXX -  TODO: set wanted samplerate and channels. */
  lame_gf->in_samplerate = 44100;
  lame_gf->out_samplerate = 44100;
  lame_gf->num_channels = 2;
  lame_gf->mode = 1;		//joint stereo
  //lame_gf->brate = 128;
  lame_gf->brate = attr->bitrate;
  lame_gf->bWriteVbrTag = 0;

  /* Now that all the options are set, lame needs to analyze them and
   * set some more options
   */
  lame_init_params (lame_gf);
  lame_print_config (lame_gf);	/* print usefull information about options being used */

  /*after encode: MP3 */
  info->ai.format = AUD_FMT_MP3;

  /* set supported qualities */
  attr->minQLevel = 0;
  attr->maxQLevel = 3;
  /* QAP supported */
  attr->supportsQAP = 1;

  return 0;
}



/*
 * encode one mp3-frame
 */
inline int
aud_codec_lame_step (plugin_ctrl_t * attr, unsigned char *in_buffer,
		     unsigned char *out_buffer)
{
  //static char mp3buffer[LAME_MAXMP3BUFFER];
  lame_global_flags *lame_gf;
  frame_ctrl_t *frameattr;
  unsigned char *input_ptr;
  unsigned char *output_ptr;

  lame_gf = (lame_global_flags *) attr->handle;

  if (attr->mode != PLUG_MODE_ENC)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return CODING_ERROR;
  }
  if (!attr->handle)
  {
    LOG_Error ("HANDLE FAILURE");
    return CODING_ERROR;
  }

  /*but we need an output buffer! */
  if (!out_buffer)
  {
    LOG_Error ("no valid output buffer.");
    return -L4_ENOMEM;
  }

  switch (attr->currentQLevel)
  {
  case 0:
    lame_gf->quality = 8;
    break;
  case 1:
    lame_gf->quality = 5;
    break;
  case 2:
    lame_gf->quality = 3;
    break;
  case 3:
    lame_gf->quality = 1;
    break;
  default:
    lame_gf->quality = 5;
  }
  lame_change_quality (lame_gf);

  /* copy frame header into output buffer */
  if (in_buffer)
  {
    memcpy (out_buffer, in_buffer, sizeof (frame_ctrl_t));
  }

  if (attr->target_format != AUD_FMT_MP3)	//better ?
  {
    LOG_Error ("can only encode PCM to MP3.");
    return CODING_ERROR;
  }

  frameattr = (frame_ctrl_t *) out_buffer;
  output_ptr = out_buffer + sizeof (frame_ctrl_t);
  if (!in_buffer)
  {
    return CODING_ERROR;
  }
  else
  {
    input_ptr = in_buffer + sizeof (frame_ctrl_t);
    frameattr->framesize =
      lame_encode_buffer_interleaved (lame_gf, (short *) input_ptr,
				      frameattr->framesize / 4, output_ptr,
				      0);
  }
  /* set format and framesize */
  frameattr->ai.format = AUD_FMT_MP3;
  attr->packetsize = frameattr->framesize + sizeof (frame_ctrl_t);

  /* was our output buffer big enough? */
  if (frameattr->framesize < 0)
  {
    LOG_Error ("may be: mp3 buffer is not big enough.");
    return CODING_ERROR;
  }
  if (frameattr->framesize == 0)
  {
    return CODING_NEED_MORE;
  }

  return CODING_OK;
}



/*
 * close and deinit codec
 */
int
aud_codec_lame_close (plugin_ctrl_t * attr)
{
  static char mp3buffer[LAME_MAXMP3BUFFER];
  lame_global_flags *lame_gf;
  lame_gf = (lame_global_flags *) attr->handle;


  if (attr->mode != PLUG_MODE_ENC)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }
  if (!attr->handle)
  {
    LOG_Error ("HANDLE FAILURE");
    return -L4_ENOHANDLE;
  }

  lame_encode_finish (lame_gf, mp3buffer, (int) sizeof (mp3buffer));

  // may return one more mp3 frame
  // ignored !!!!

  attr->handle = NULL;

  strncpy (attr->info, "LAME says bye bye :)", 32);
  return 0;
}
