/*
 * \brief   ffmpeg codec plugin for VERNER's core
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
 * uses parts of transcodes import_ffmpeg.c (GPL)
 * Copyright notice is few lines down.
 */

#include "vc_ffmpeg.h"

/* verner */
#include "arch_globals.h"

/* configuration */
#include "verner_config.h"

/* L4 includes */
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* OSKit includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ffmpeg stuff */
#include <avcodec.h>

#if H264_SLICE_SCHEDULE
/* the H.264 slice scheduler */
#include "process.h"
#endif

/* aclib */
#include "aclib.h"
static void *(*fastmemcpy_func) (void *to, const void *from, size_t len);

/* avcodec - internal data */
struct ffmpeg_data
{
  AVCodec *lavc_dec_codec;
  AVCodecContext *lavc_dec_context;
  AVFrame *picture;
  unsigned int xdim, ydim;
  double framerate;
  /* buffer if we still have data after we got one frame */
  void *old_data;
  int old_data_alloc;
  int old_data_size;
};


/* transcode */
struct ffmpeg_codec
{
  int id;
  char *name;
  char fourCCs[10][5];
};

/* fourCC to ID mapping taken from MPlayer's codecs.conf */
static struct ffmpeg_codec ffmpeg_codecs[] = {
  {CODEC_ID_MSMPEG4V1, "msmpeg4v1",
   {"MP41", "DIV1", ""}},
  {CODEC_ID_MSMPEG4V2, "msmpeg4v2",
   {"MP42", "DIV2", ""}},
  {CODEC_ID_MSMPEG4V3, "msmpeg4",
   {"DIV3", "DIV5", "AP41", "MPG3", "MP43", ""}},
  {CODEC_ID_MPEG4, "mpeg4",
   {"DIVX", "XVID", "MP4S", "M4S2", "MP4V", "UMP4", "DX50", ""}},
  {CODEC_ID_MJPEG, "mjpeg",
   {"MJPG", "AVRN", "AVDJ", "JPEG", "MJPA", "JFIF", ""}},
  {CODEC_ID_MPEG2VIDEO, "mpeg2video",
   {"MPG1", "MPG2", ""}},
  {CODEC_ID_DVVIDEO, "dvvideo",
   {"DVSD", ""}},
  {CODEC_ID_WMV1, "wmv1",
   {"WMV1", ""}},
  {CODEC_ID_WMV2, "wmv2",
   {"WMV2", ""}},
  {CODEC_ID_H263I, "h263",
   {"I263", ""}},
  {CODEC_ID_H263P, "h263p",
   {"H263", "U263", "VIV1", ""}},
  {CODEC_ID_RV10, "rv10",
   {"RV10", "RV13", ""}},
  {CODEC_ID_H264, "h264",
   {"H264", "X264", ""}},
  {0, NULL, {""}}
};


static struct ffmpeg_codec *find_ffmpeg_codec (int fourCC);


int
vid_codec_ffmpeg_init (plugin_ctrl_t * attr, stream_info_t * info)
{
  struct ffmpeg_data *internal_data;
  static struct ffmpeg_codec *codec;

  strncpy (attr->info, LIBAVCODEC_IDENT, 32);

  if (attr->mode != PLUG_MODE_DEC)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }
  if (info->type != STREAM_TYPE_VIDEO)
  {
    LOG_Error ("MEDIA TYPE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }
  /* build internal data structs and alloc mem */
  attr->handle = (struct ffmpeg_data *) malloc (sizeof (struct ffmpeg_data));
  if (!attr->handle)
  {
    LOG_Error ("malloc failed!");
    return -L4_ENOHANDLE;
  }
  memset (attr->handle, 0, sizeof (struct ffmpeg_data));
  internal_data = (struct ffmpeg_data *) attr->handle;

  /* set supported qualities */
  attr->minQLevel = attr->maxQLevel = 0;
  /* QAP supported */
  attr->supportsQAP = 0;

  /* initialization of ffmpeg stuff */
  avcodec_init ();
  avcodec_register_all ();

  codec = find_ffmpeg_codec (info->vi.fourCC);
  if (codec == NULL)
  {
    LOG_Error ("No codec is known the FOURCC");
    free (attr->handle);
    attr->handle = NULL;
    return -L4_EUNKNOWN;
  }


  internal_data->lavc_dec_codec = avcodec_find_decoder (codec->id);
  if (!internal_data->lavc_dec_codec)
  {
    LOG_Error ("No codec is known the FOURCC");
    free (attr->handle);
    attr->handle = NULL;
    return -L4_EUNKNOWN;
  }

  /* Set these to the expected values so that ffmpeg's decoder can
     properly detect interlaced input. */
  internal_data->lavc_dec_context = avcodec_alloc_context ();
  if (internal_data->lavc_dec_context == NULL)
  {
    LOG_Error ("Could not allocate enough memory.");
    free (attr->handle);
    attr->handle = NULL;
    return -L4_ENOMEM;
  }


  /* these are codecs which we import raw, and so we may be sending a incomplete frame. */
  if ((codec->id == CODEC_ID_MJPEG) || (codec->id == CODEC_ID_MPEG1VIDEO)
      || (codec->id == CODEC_ID_MP2) || (codec->id == CODEC_ID_DVVIDEO)
      || (codec->id == CODEC_ID_MPEG2VIDEO) || (codec->id == CODEC_ID_H264))
  {
    if (internal_data->lavc_dec_codec->capabilities & CODEC_CAP_TRUNCATED)
      internal_data->lavc_dec_context->flags |= CODEC_FLAG_TRUNCATED;
    LOGdL (DEBUG_CODEC, "Using truncated strategy");
  }
  else
    /* for others we have to know width and height */
  {
    /* this may change during playback */
    internal_data->lavc_dec_context->width = info->vi.xdim;
    internal_data->lavc_dec_context->height = info->vi.ydim;
    LOGdL (DEBUG_CODEC, "Using \"one frame in - on frame out\" strategy");
  }
  /* save old values to compare in step */
  internal_data->xdim = info->vi.xdim;
  internal_data->ydim = info->vi.ydim;
  internal_data->framerate = info->vi.framerate;

  /* set some necassary CAPS */
  internal_data->lavc_dec_context->error_resilience = 2;
  internal_data->lavc_dec_context->error_concealment = 3;
  internal_data->lavc_dec_context->workaround_bugs = FF_BUG_AUTODETECT;
  internal_data->lavc_dec_context->idct_algo = 0;	//autodetect

  /* open it */
  if (avcodec_open
      (internal_data->lavc_dec_context, internal_data->lavc_dec_codec) < 0)
  {
    LOG_Error ("could not open codec");
    free (internal_data->lavc_dec_context);
    internal_data->lavc_dec_context = NULL;
    free (attr->handle);
    attr->handle = NULL;
    return -L4_EUNKNOWN;
  }

  /* alloc internal frame buffer */
  internal_data->picture = avcodec_alloc_frame ();

  /* after decoding we get raw */
  info->vi.format = VID_FMT_RAW;

  /* checking cpu caps for faster memcpy */
  fastmemcpy_func = determineFastMemcpy (attr->cpucaps);

#if H264_SLICE_SCHEDULE
  if (internal_data->lavc_dec_context->codec_id == CODEC_ID_H264)
    process_init(internal_data->lavc_dec_context, NULL);
#endif

  /* done */
  return 0;
}

/* decode next frame */
inline int
vid_codec_ffmpeg_step (plugin_ctrl_t * attr, unsigned char *in_buffer,
		       unsigned char *out_buffer)
{
  int i, edge_width;
  int UVls, src, dst, row, col;
  char *Ybuf, *Ubuf, *Vbuf;
  int len, size;
  int got_picture;
  unsigned char *input_ptr;

  frame_ctrl_t *frameattr;
  struct ffmpeg_data *internal_data;
  internal_data = (struct ffmpeg_data *) attr->handle;

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

  if ((attr->target_colorspace != VID_YV12)
      && (attr->target_colorspace != VID_YUV420))
  {
    LOG_Error ("colorspace not supported, only YV12 and YUV420 is.");
    return CODING_ERROR;
  }

  if (!out_buffer)
  {
    LOGdL (DEBUG_CODEC, "need output buffer");
    return CODING_ERROR;
  }

  frameattr = (frame_ctrl_t *) out_buffer;

  /* get information from frame_ctrl_t */
  if (in_buffer)
  {
    LOGdL (DEBUG_CODEC, "using fresh data!");
    fastmemcpy_func (out_buffer, in_buffer, sizeof (frame_ctrl_t));
    if (is_reset_sync_point (frameattr->keyframe))
    {
      LOGdL (DEBUG_CODEC, "reset sync, flush buffers.");
      avcodec_flush_buffers (internal_data->lavc_dec_context);
    }
    input_ptr = in_buffer + sizeof (frame_ctrl_t);
    size = frameattr->framesize;
  }
  /* we should have still data here */
  else if ((internal_data->old_data != NULL)
	   && (internal_data->old_data_size > 0))
  {
    LOGdL (DEBUG_CODEC, "using old data!");
    input_ptr = internal_data->old_data;
    size = internal_data->old_data_size;
  }
  else
  {
    LOG_Error ("neither \"old\" data nor new data. :&");
    return CODING_ERROR;
  }

  /* while we have data */
  while (1)
  {
    if (size <= 0)
    {
      /* we need more input data */
      LOGdL (DEBUG_CODEC, "we need more data");
      return CODING_NEED_MORE;
    }

    len =
      avcodec_decode_video (internal_data->lavc_dec_context,
			    internal_data->picture, &got_picture, input_ptr,
			    size);

    if (len < 0)
    {
      LOG_Error ("frame decoding failed.");
      return CODING_ERROR;
    }

    size -= len;
    input_ptr += len;

    /* all input data used ?, if not copy into internal buffer,
     * take it in next step and signal we still have data */
    if (size > 0)
    {
      void *free_buffer = NULL;

      LOGdL (DEBUG_CODEC, "still have data - copy into internal buffer");
      /* we ignore all errors here ! */
      /* realloc to fit */
      if (internal_data->old_data_alloc < size) {
	free_buffer = internal_data->old_data;
	internal_data->old_data = malloc(size);
	if (internal_data->old_data == NULL)
	{
	  LOG_Error ("realloc failed.");
	  return -L4_ENOMEM;
	}
      }
      /* memcpy into internal buffer, have to use memmove due to potential overlap */
      memmove (internal_data->old_data, input_ptr, size);
      internal_data->old_data_size = size;

      free(free_buffer);
    }

    if (got_picture)
    {
      /* we have an complete frame */
      LOGdL (DEBUG_CODEC, "we got a frame");
      break;
    }
  }

  /* if we're here - we  have an complete frame */
  if ((!is_reset_sync_point (frameattr->keyframe))
      && (!is_reconfigure_point (frameattr->keyframe)))
    frameattr->keyframe =
      internal_data->picture->key_frame;

  /* check if we have to send the reconfigure signal */
  if ((internal_data->lavc_dec_context->width != internal_data->xdim)
      || (internal_data->lavc_dec_context->height != internal_data->ydim))
  {
    frameattr->keyframe = RECONFIGURE_POINT;
  }
  frameattr->vi.xdim = internal_data->xdim =
    internal_data->lavc_dec_context->width;
  frameattr->vi.ydim = internal_data->ydim =
    internal_data->lavc_dec_context->height;



  /* set data inf frame_ctrl_t */
  frameattr->vi.format = attr->target_format;
  frameattr->vi.colorspace = attr->target_colorspace;
  frameattr->vi.framerate = internal_data->framerate;
  attr->packetsize = vid_streaminfo2packetsize (frameattr);
  frameattr->framesize = attr->packetsize - sizeof (frame_ctrl_t);
  frameattr->frameID = internal_data->lavc_dec_context->frame_number;


  /* Now we have to convert the picture to YV12 or YUV420 (difference is U<-->V are changed */
  Ybuf = out_buffer + sizeof (frame_ctrl_t);
  switch (attr->target_colorspace)
  {
  case VID_YV12:
    Ubuf =
      Ybuf +
      internal_data->lavc_dec_context->width *
      internal_data->lavc_dec_context->height;
    Vbuf =
      Ubuf +
      internal_data->lavc_dec_context->width *
      internal_data->lavc_dec_context->height / 4;
    break;
  case VID_YUV420:
  default:
    Vbuf =
      Ybuf +
      internal_data->lavc_dec_context->width *
      internal_data->lavc_dec_context->height;
    Ubuf =
      Vbuf +
      internal_data->lavc_dec_context->width *
      internal_data->lavc_dec_context->height / 4;
  }
  UVls = internal_data->picture->linesize[1];


  switch (internal_data->lavc_dec_context->pix_fmt)
  {
  case PIX_FMT_YUV420P:
    /* Result is in YUV 4:2:0 (YV12) format, but each line ends with
       an edge which we must skip */
    edge_width =
      (internal_data->picture->linesize[0] -
       internal_data->lavc_dec_context->width) / 2;
    for (i = 0; i < internal_data->lavc_dec_context->height; i++)
    {
      fastmemcpy_func (Ybuf + i * internal_data->lavc_dec_context->width, internal_data->picture->data[0] + i * internal_data->picture->linesize[0],	//+ edge_width,
		       internal_data->lavc_dec_context->width);
    }
    for (i = 0; i < internal_data->lavc_dec_context->height / 2; i++)
    {
      fastmemcpy_func (Vbuf + i * internal_data->lavc_dec_context->width / 2, internal_data->picture->data[1] + i * internal_data->picture->linesize[1],	// + edge_width / 2,
		       internal_data->lavc_dec_context->width / 2);
      fastmemcpy_func (Ubuf + i * internal_data->lavc_dec_context->width / 2, internal_data->picture->data[2] + i * internal_data->picture->linesize[2],	// + edge_width / 2,
		       internal_data->lavc_dec_context->width / 2);
    }

    break;
  case PIX_FMT_YUV422P:
    /* Result is in YUV 4:2:2 format (subsample UV vertically for YV12): */
    fastmemcpy_func (Ybuf, internal_data->picture->data[0],
		     internal_data->picture->linesize[0] *
		     internal_data->lavc_dec_context->height);
    src = 0;
    dst = 0;
    for (row = 0; row < internal_data->lavc_dec_context->height; row += 2)
    {
      fastmemcpy_func (Ubuf + dst, internal_data->picture->data[1] + src,
		       UVls);
      fastmemcpy_func (Vbuf + dst, internal_data->picture->data[2] + src,
		       UVls);
      dst += UVls;
      src = dst << 1;
    }
    break;
  case PIX_FMT_YUV444P:
    /* Result is in YUV 4:4:4 format (subsample UV h/v for YV12): */
    fastmemcpy_func (Ybuf, internal_data->picture->data[0],
		     internal_data->picture->linesize[0] *
		     internal_data->lavc_dec_context->height);
    src = 0;
    dst = 0;
    for (row = 0; row < internal_data->lavc_dec_context->height; row += 2)
    {
      for (col = 0; col < internal_data->lavc_dec_context->width; col += 2)
      {
	Ubuf[dst] = internal_data->picture->data[1][src];
	Vbuf[dst] = internal_data->picture->data[2][src];
	dst++;
	src += 2;
      }
      src += UVls;
    }
    break;
  default:
    LOG_Error ("Unsupported decoded frame format colorspace");
    return CODING_ERROR;
  }

  /* here we'll have YV12 or YUV420 */


  /* still have data ? */
  if (size > 0)
    return CODING_OK | CODING_HAS_STILL_DATA;
  else
    return CODING_OK;
}

/* clode codec */
int
vid_codec_ffmpeg_close (plugin_ctrl_t * attr)
{
  struct ffmpeg_data *internal_data;
  internal_data = (struct ffmpeg_data *) attr->handle;

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

  if (internal_data->lavc_dec_context)
  {
#if H264_SLICE_SCHEDULE
    if (internal_data->lavc_dec_context->codec_id == CODEC_ID_H264)
      process_finish(internal_data->lavc_dec_context);
#endif

    avcodec_close (internal_data->lavc_dec_context);
    free (internal_data->lavc_dec_context);

    internal_data->lavc_dec_context = NULL;
  }

  /* free internal frame buffer */
  if (internal_data->old_data)
    free (internal_data->old_data);

  /* free struct */
  free (internal_data);
  attr->handle = NULL;
  strncpy (attr->info, "cu, FFmpeg :)", 32);
  return 0;
}


/* utilitie functions taken from transcode import_ffmpeg.c */

/*
 *  import_ffmpeg.c
 *
 *  Copyright (C) Moritz Bunkus - October 2002
 *
 *  This file is part of transcode, a linux video stream processing tool
 *      
 *  transcode is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  transcode is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */
static struct ffmpeg_codec *
find_ffmpeg_codec (int fourCC)
{
  int i;
  struct ffmpeg_codec *cdc;

  cdc = &ffmpeg_codecs[0];

  if (fourCC == 0)
    return NULL;

  while (cdc->name != NULL)
  {
    i = 0;
    while (cdc->fourCCs[i][0] != 0)
    {
      if (vid_fourcc2int (cdc->fourCCs[i]) == fourCC)
	return cdc;
      i++;
    }
    cdc++;
  }

  return NULL;
}
