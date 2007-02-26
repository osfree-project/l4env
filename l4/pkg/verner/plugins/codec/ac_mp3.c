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

#include "ac_mp3.h"


/* L4 includes */
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* OSKit includes */
#include <stdlib.h>		/* malloc, free */

/* verner */
#include "arch_globals.h"

/* configuration */
#include "verner_config.h"

/* libmpg123 */
#include "mpg123.h"
#include "mpglib.h"

/*
 * init plugin 
 */
int
aud_codec_mp3_init (plugin_ctrl_t * attr, stream_info_t * info)
{
  struct mpstr *mp;

  /* info string */
  strncpy (attr->info, "mpg123lib Decoder", 32);

  /* check valid mode */
  if (attr->mode != PLUG_MODE_DEC)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }

  /* alloc memory */
  attr->handle = (struct mpstr *) malloc (sizeof (struct mpstr));
  if (!attr->handle)
  {
    LOG_Error ("malloc failed!");
    return -L4_ENOMEM;
  }
  mp = (struct mpstr *) attr->handle;

  /* init codec */
  InitMP3 (mp);

  /* after decoding we get RAW PCM */
  info->ai.format = AUD_FMT_PCM;

  /*set supported qualities */
  attr->minQLevel = attr->maxQLevel = 0;
  /* QAP supported */
  attr->supportsQAP = 0;

  /* done */
  return 0;
}

/*
 * decode one frame 
 */
inline int
aud_codec_mp3_step (plugin_ctrl_t * attr, unsigned char *in_buffer,
		    unsigned char *out_buffer)
{
  int mp3ret, size;
  struct mpstr *mp;
  frame_ctrl_t *frameattr;
  int header_pos = 0;

  /* check for valid handle and mode */
  if (!attr->handle)
  {
    LOG_Error ("HANDLE FAILURE");
    return CODING_ERROR;
  }
  if (attr->mode != PLUG_MODE_DEC)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return CODING_ERROR;
  }

  /* get handle */
  mp = (struct mpstr *) attr->handle;

  /* but we need an output buffer ! */
  if (!out_buffer)
  {
    LOGdL (DEBUG_CODEC, "no valid output buffer.");
    return CODING_ERROR;
  }

  /* if we don't get any data, we can't use it :) */
  if (in_buffer)
    memcpy (out_buffer, in_buffer, sizeof (frame_ctrl_t));
  else
    LOGdL (DEBUG_CODEC, "no valid input buffer. try do work on.");


  frameattr = (frame_ctrl_t *) out_buffer;
  if (attr->target_format != AUD_FMT_PCM)
  {
    LOGdL (DEBUG_CODEC, "can only decode MP3 to PCM.");
    return CODING_ERROR;
  }

  /* reset sync? we have to flush buffers, but mpg123-api has no flush */
  if (((is_reconfigure_point (frameattr->keyframe))
       || (is_reset_sync_point (frameattr->keyframe))) && (in_buffer != NULL))
  {
    mp3_header_t mp3_header;
    unsigned long header;

    LOGdL (DEBUG_CODEC, "flushing buffers (reinit codec)");
    /* close codec */
    ExitMP3 (mp);
    /* reinit codec */
    InitMP3 (mp);
    /* now we have to search the mp3 header and setup our transmitted chunk correct */
    if (frameattr->framesize > 0)
      header_pos =
	find_mp3_header (in_buffer + sizeof (frame_ctrl_t),
			 frameattr->framesize, &header);
    else
      header_pos = -1;

    /* there's no header in the chunk - this should not happend! */
    if (header_pos < 0)
    {
      LOGdL (DEBUG_CODEC, "No MP3 header found - need more data.");
      return CODING_NEED_MORE;
    }
    /* now we've to adjust last_sync_position as we have changed offset */
    else if (header_pos > 0)
    {
      /* we look into the mp3 frame to know how many samples we dropped */
      decode_mp3_header (header, &mp3_header);
      frameattr->last_sync_pts +=
	(double) header_pos *1000.00 / (128.00 *
					(double) mp3_tabsel[mp3_header.
							    lsf][mp3_header.
								 bitrate_index]);

      LOGdL (DEBUG_CODEC,
	     "found MP3 header at byte %i, new last_sync_pts = %i\n",
	     (int) header_pos, (int) frameattr->last_sync_pts);
    }

  }				/* end reset_ */


  /* decode */
  /* we still get data */
  if (in_buffer)
  {
    mp3ret =
      decodeMP3 (mp, in_buffer + sizeof (frame_ctrl_t) + header_pos,
		 frameattr->framesize - header_pos,
		 out_buffer + sizeof (frame_ctrl_t), MAX_AUD_CHUNK_SIZE,
		 &size);
  }
  else
    /* we do not get data anymore, but we have to empty mp3's buffer. */
  {
    mp3ret =
      decodeMP3 (mp, NULL, 0, out_buffer + sizeof (frame_ctrl_t),
		 MAX_AUD_CHUNK_SIZE, &size);
  }

  /* set changed info in framectrl_t */
  frameattr->ai.format = AUD_FMT_PCM;
  frameattr->framesize = size;
  attr->packetsize = frameattr->framesize + sizeof (frame_ctrl_t);

  switch (mp3ret)
  {
  case MP3_OK:
    /* we should not consume and waste that much memory */
    //LOGdL (DEBUG_CODEC, "decode ok.");
    if (mp->bsize > 1024 * 128)	/* 128 KB ca. 0.2 min audio */
      return CODING_OK | CODING_HAS_STILL_DATA;
    else
      return CODING_OK;
    break;
  case MP3_NEED_MORE:
    LOGdL (DEBUG_CODEC, "need more data.");
    return CODING_NEED_MORE;
    break;
  case MP3_ERR:
  default:
    LOGdL (DEBUG_CODEC, "decode error.");
    frameattr->framesize = 0;
    return CODING_ERROR;
  }

  /* should never be  */
  return CODING_ERROR;
}

/*
 * close import plugin 
 */
int
aud_codec_mp3_close (plugin_ctrl_t * attr)
{
  struct mpstr *mp;
  mp = (struct mpstr *) attr->handle;

  /* check for valid handle and mode */
  if (!attr->handle)
  {
    LOG_Error ("HANDLE FAILURE");
    return CODING_ERROR;
  }
  if (attr->mode != PLUG_MODE_DEC)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return CODING_ERROR;
  }

  /* close codec */
  ExitMP3 (mp);
  /* free struct */
  free (mp);
  attr->handle = NULL;

  /* done */
  return 0;
}
