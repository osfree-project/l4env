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

#include "vc_pass.h"

/* L4/OSKit includes */
#include <stdio.h>
#include <stdlib.h>
#include <l4/env/errno.h>



/*
 * initialize codec
 */
int
vid_codec_pass_init (plugin_ctrl_t * attr, stream_info_t * info)
{
  strncpy (attr->info, "Passthrough only for video", 32);

  if (attr->mode != PLUG_MODE_PASS)
  {
    LOG_Error ("codec_pass.c: MODE NOT SUPPORTED\n");
    return -L4_ENOTSUPP;
  }
  if (info->type != STREAM_TYPE_VIDEO)
  {
    LOG_Error ("MEDIA TYPE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }
  attr->minQLevel = attr->maxQLevel = 0;
  /* QAP not supported */
  attr->supportsQAP = 0;
  return 0;
}

/*
 * process next frame 
 */
int
vid_codec_pass_step (plugin_ctrl_t * attr, unsigned char *in_buffer,
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
    LOG_Error ("got wrong adresses");
    return CODING_ERROR;
  }
  frameattr = (frame_ctrl_t *) in_buffer;

  /* set wanted format (we don't change colorspaces) */
  frameattr->vi.format = attr->target_format;

  /* if it is raw we can do colorspace transformation */
  if (frameattr->vi.format == VID_FMT_RAW)
  {
    frameattr->framesize = vid_streaminfo2packetsize (frameattr);
    frameattr->framesize -= sizeof (frame_ctrl_t);

    /* set information in frameheader */
    attr->packetsize = frameattr->framesize + sizeof (frame_ctrl_t);
    /* colorspace? yv12<-->yuv420 is ok, but rgb (and others) stay as they are */
    if (((frameattr->vi.colorspace == VID_YV12)
	 && (attr->target_colorspace == VID_YUV420))
	|| ((frameattr->vi.colorspace == VID_YUV420)
	    && (attr->target_colorspace == VID_YV12)))
    {
      /* we have to exchange U and V */
      char *Ybuf, *Ubuf, *Vbuf;
      char *Ybuf_dest, *Ubuf_dest, *Vbuf_dest;
      unsigned int y_stride = frameattr->vi.xdim * frameattr->vi.ydim;
      unsigned int uv_stride = y_stride / 4;

      Ybuf = in_buffer + sizeof (frame_ctrl_t);
      Ubuf = Ybuf + y_stride;
      Vbuf = Ubuf + uv_stride;

      Ybuf_dest = out_buffer + sizeof (frame_ctrl_t);
      Vbuf_dest = Ybuf_dest + y_stride;
      Ubuf_dest = Vbuf_dest + uv_stride;

      /* copy Y, U and V */
      memcpy (Ybuf_dest, Ybuf, y_stride);
      memcpy (Ubuf_dest, Ubuf, uv_stride);
      memcpy (Vbuf_dest, Vbuf, uv_stride);
      /* now copy frame_ctrl_t */
      memcpy (out_buffer, in_buffer, sizeof (frame_ctrl_t));

      //set changed colorspace
      frameattr->vi.colorspace = attr->target_colorspace;
    }
    else
      memcpy (out_buffer, in_buffer, attr->packetsize);

  }
  else
    /* it is not RAW - just copy everthing */
  {
    /* packetsize */
    attr->packetsize = frameattr->framesize + sizeof (frame_ctrl_t);
    memcpy (out_buffer, in_buffer, attr->packetsize);
  }
  return CODING_OK;
}

/*
 * close codec 
 */
int
vid_codec_pass_close (plugin_ctrl_t * attr)
{
  strncpy (attr->info, "cu, your simple plugin :)", 32);
  return 0;
}
