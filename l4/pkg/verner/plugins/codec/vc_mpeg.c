/*
 * \brief   libmpeg2 codec plugin for VERNER's core
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

/* verner */
#include "arch_globals.h"

/* configuration */
#include "verner_config.h"

/* L4 includes */
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* OSKit includes */
#include <stdlib.h>

/* libmpeg2 stuff */
#include "vc_mpeg.h"
#include <mpeg2.h>

/* aclib */
#include "aclib.h"
void *(*fastmemcpy_func) (void *to, const void *from, size_t len);

/* internal data and handles */
struct mpeg_data
{
  mpeg2dec_t *decoder;
  const mpeg2_info_t *info;
  mpeg2_state_t state;
  unsigned char *fbuf[3];
  long frameID;
  int last_xdim, last_ydim;
};


/* framecode to framerate */
static int frameratecode2framerate[16] = {
  0,
  // Official mpeg1/2 framerates: (1-8)
  24000 / 1001, 24, 25,
  30000 / 1001, 30, 50,
  60000 / 1001, 60,
  // Xing's 15fps: (9)
  15,
  // libmpeg3's "Unofficial economy rates": (10-13)
  5, 10, 12, 15,
  // some invalid ones: (14-15)
  0, 0
};
static unsigned int frameperiod[16] = {
  0, 1126125, 1125000, 1080000, 900900, 900000, 540000, 450450, 450000,
  /* unofficial: xing 15 fps */
  1800000,
  /* unofficial: libmpeg3 "Unofficial economy rates" 5/10/12/15 fps */
  5400000, 2700000, 2250000, 1800000, 0, 0
};


/*
 * init codec 
 */
int
vid_codec_mpeg_init (plugin_ctrl_t * attr, stream_info_t * info)
{
  struct mpeg_data *internal_data;

  strncpy (attr->info, "libmpeg2-0.4.0b", 32);

  if (attr->mode != PLUG_MODE_DEC)
  {
    LOG_Error ("MODE NOT SUPPORTED. Only decoding is.");
    return -L4_ENOTSUPP;
  }
  if (info->type != STREAM_TYPE_VIDEO)
  {
    LOG_Error ("MEDIA TYPE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }
  /* Build internal data struct
     and alloc memory */
  attr->handle = (struct mpeg_data *) malloc (sizeof (struct mpeg_data));
  if (!attr->handle)
  {
    LOG_Error ("malloc failed!");
    return -L4_ENOHANDLE;
  }
  memset (attr->handle, 0, sizeof (struct mpeg_data));
  internal_data = (struct mpeg_data *) attr->handle;

  /* set supported qualities */
  attr->minQLevel = attr->maxQLevel = 0;
  /* QAP supported */
  attr->supportsQAP = 0;

  /* init decoder */
  internal_data->decoder = mpeg2_init ();
  if (internal_data->decoder == NULL)
  {
    LOG_Error ("Could not allocate a decoder object.");
    free (attr->handle);
    attr->handle = NULL;
    return -L4_EUNKNOWN;
  }
  internal_data->info = mpeg2_info (internal_data->decoder);


  LOGdL (DEBUG_CODEC, "accel cpu features=%i",
	 (int) mpeg2_accel (MPEG2_ACCEL_DETECT));

  /* after decoding we get raw data */
  info->vi.format = VID_FMT_RAW;

  /* start values */
  internal_data->last_xdim = internal_data->last_ydim = -1;

  /* start at frame null */
  info->frameID = 0;

  /* checking cpu caps for faster memcpy */
  fastmemcpy_func = determineFastMemcpy (attr->cpucaps);

  return 0;
}


/*
 * en-/decode next frame 
 */
inline int
vid_codec_mpeg_step (plugin_ctrl_t * attr, unsigned char *in_buffer,
		     unsigned char *out_buffer)
{
  int i;
  frame_ctrl_t *frameattr;
  int pixels;
  struct mpeg_data *internal_data;
  internal_data = (struct mpeg_data *) attr->handle;

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

  if (attr->target_colorspace != VID_YUV420)
  {
    LOG_Error ("colorspace not supported, only YV12 and YUV420 is.");
    return CODING_ERROR;
  }

  if (!out_buffer)
  {
    LOG_Error ("need output buffer");
    return CODING_ERROR;
  }
  frameattr = (frame_ctrl_t *) out_buffer;

  if (in_buffer)
  {
    /* get information from frame_ctrl_t */
    fastmemcpy_func (out_buffer, in_buffer, sizeof (frame_ctrl_t));
    if (is_reset_sync_point (frameattr->keyframe))
    {
      LOGdL (DEBUG_CODEC, "reset sync, flush buffers.\n");
      mpeg2_reset (internal_data->decoder, 1);
    }

    /* add data to buffer */
    mpeg2_buffer (internal_data->decoder,
		  (unsigned char *) (in_buffer + sizeof (frame_ctrl_t)),
		  (unsigned char *) (in_buffer + sizeof (frame_ctrl_t) +
				     frameattr->framesize));

  }				/* if in_buffer */


  do
  {
    internal_data->state = mpeg2_parse (internal_data->decoder);
    switch (internal_data->state)
    {
    case STATE_INVALID_END:
      LOG_Error ("invalid picture encountered - skipping.");
      mpeg2_skip (internal_data->decoder, 1);
      break;
    case STATE_SLICE:
    case STATE_END:
      if (internal_data->info->display_fbuf)
      {
	pixels =
	  internal_data->info->sequence->width *
	  internal_data->info->sequence->height;
	internal_data->fbuf[0] =
	  (unsigned char *) out_buffer + sizeof (frame_ctrl_t);
	internal_data->fbuf[1] =
	  (unsigned char *) internal_data->fbuf[0] + pixels;
	internal_data->fbuf[2] =
	  (unsigned char *) internal_data->fbuf[1] + (pixels / 4);

	/* now set information in frame_ctrl_t */

	/* 
	 * check if size hast changed, if so it's a RESET_ATTR_POINT 
	 * which should export-plugin in vsync to reorganize.
	 */
	if ((internal_data->last_xdim != internal_data->info->sequence->width)
	    || (internal_data->last_ydim !=
		internal_data->info->sequence->height))
	{
	  frameattr->keyframe = RECONFIGURE_POINT;
	  internal_data->last_xdim = internal_data->info->sequence->width;
	  internal_data->last_ydim = internal_data->info->sequence->height;
	}
	frameattr->vi.xdim = internal_data->info->sequence->width;
	frameattr->vi.ydim = internal_data->info->sequence->height;

	/* get framerate */
	for (i = 0; i < 16; i++)
	  if (internal_data->info->sequence->frame_period == frameperiod[i])
	  {
	    frameattr->vi.framerate = frameratecode2framerate[i];
	    break;
	  }

	/* other attributes */
	frameattr->vi.format = attr->target_format;
	frameattr->vi.colorspace = attr->target_colorspace;
	attr->packetsize = vid_streaminfo2packetsize (frameattr);
	frameattr->framesize = attr->packetsize - sizeof (frame_ctrl_t);
	frameattr->frameID = internal_data->frameID;
	internal_data->frameID++;

	/* Y */
	fastmemcpy_func (internal_data->fbuf[0],
			 (unsigned char *) internal_data->info->display_fbuf->
			 buf[0], pixels);
	/* U */
	fastmemcpy_func (internal_data->fbuf[1],
			 internal_data->info->display_fbuf->buf[1],
			 pixels / 4);
	/* V */
	fastmemcpy_func (internal_data->fbuf[2],
			 internal_data->info->display_fbuf->buf[2],
			 pixels / 4);

	return CODING_OK | CODING_HAS_STILL_DATA;
      }


      break;
    default:
      /*  unknown state. :-(  - loop */
      break;
    }
  }
  while (internal_data->state != STATE_BUFFER);

  return CODING_NEED_MORE;

}



/*
 * codec stop 
 */
int
vid_codec_mpeg_close (plugin_ctrl_t * attr)
{
  struct mpeg_data *internal_data;
  internal_data = (struct mpeg_data *) attr->handle;

  if (attr->mode != PLUG_MODE_DEC)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }
  if (!attr->handle)
  {
    LOG_Error ("HANDLE FAILURE");
    return -L4_ENOHANDLE;
  }

  /* free decoder */
  mpeg2_close (internal_data->decoder);
  /* free structs and mem */
  free (internal_data);
  attr->handle = NULL;
  strncpy (attr->info, "cu, libmpeg2 plugin :)", 32);
  return 0;
}
