/*
 * \brief   AVI container plugin for VERNER
 * \date    2004-09-09
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

#include "demux_avi.h"

/* L4 includes */
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* OSKit includes */
#include <stdlib.h>		/* malloc, free */

/* ogm */
#include "avilib.h"

/* verner */
#include "arch_globals.h"
/* configuration */
#include "verner_config.h"

/* internal data */
struct avi_data
{
  /* avi */
  avi_t *AVI;
  frame_ctrl_t frameattr;
  /* audio stuff */
  long audioBytesPerFrame;
  int vbr;
  /* video stuff */
  long frameID;
  long lastFrameSize;
  void *lastFrameBuffer;
};


/*
 * init plugin 
 */

/*
 * video part
 */
int
vid_import_avi_init (plugin_ctrl_t * attr, stream_info_t * info)
{
  struct avi_data *AVI_d;

  /* info string */
  strncpy (attr->info, "AVI-Import-Plugin", 32);

  /* check valid mode */
  if (attr->mode != PLUG_MODE_IMPORT)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }

  /* mem alloc */
  attr->handle = (struct avi_data *) malloc (sizeof (struct avi_data));
  if (!attr->handle)
  {
    LOG_Error ("malloc failed!");
    return -L4_ENOMEM;
  }
  memset (attr->handle, 0, sizeof (struct avi_data));
  AVI_d = (struct avi_data *) attr->handle;

  /* avi-file init */
  AVI_d->AVI = AVI_open_input_file (attr->filename, 1);
  if (AVI_d->AVI == NULL)
  {
    free (AVI_d);
    attr->handle = NULL;
    LOG_Error ("AVI_open_input_file() failed");
    return -L4_EOPEN;
  }

  /* get the size of the biggest chunk and alloc an buffer 
   * AVI_max_video_chunk() sometimes returns 0 - this doesn't harm */
  AVI_d->lastFrameBuffer = malloc (AVI_max_video_chunk (AVI_d->AVI));
  if (AVI_d->lastFrameBuffer == NULL)
  {
    free (AVI_d);
    attr->handle = NULL;
    LOG_Error ("malloc framebuffer failed");
    return -L4_ENOMEM;
  }

  /* Set stream info */
  /* type */
  info->type = STREAM_TYPE_VIDEO;
  /* Avi opened - probing */
  info->vi.fourCC = vid_fourcc2int (AVI_video_compressor (AVI_d->AVI));
  info->vi.format = vid_fourcc2fmt (AVI_video_compressor (AVI_d->AVI));
  info->vi.framerate = AVI_frame_rate (AVI_d->AVI);
  info->vi.xdim = AVI_video_width (AVI_d->AVI);
  info->vi.ydim = AVI_video_height (AVI_d->AVI);

  /* If we're not sure, we assume RGB32 for buffer sizes */
  if (!strncasecmp (AVI_video_compressor (AVI_d->AVI), "YV12", 4))
    info->vi.colorspace = VID_YV12;
  else if (!strncasecmp (AVI_video_compressor (AVI_d->AVI), "UYVY", 4))
  {
    LOGdL (DEBUG_IMPORT, "colorspace UYUY.");
    info->vi.colorspace = VID_UYVY;
  }
  else
  {
    LOGdL (DEBUG_IMPORT, "can't determine colorspace. expect RGB32.");
    info->vi.colorspace = VID_RGB32;
  }

  /* set defaults for furher "step"-routines ! */
  AVI_d->frameattr.type = info->type;
  AVI_d->frameattr.vi.format = info->vi.format;
  AVI_d->frameattr.vi.fourCC = info->vi.fourCC;
  AVI_d->frameattr.vi.framerate = info->vi.framerate;
  AVI_d->frameattr.vi.xdim = info->vi.xdim;
  AVI_d->frameattr.vi.ydim = info->vi.ydim;
  AVI_d->frameattr.vi.colorspace = info->vi.colorspace;

  /* there's no buffer */
  attr->bufferSize = attr->bufferElements = 0;

  /* done */
  return 0;
}

/*
 * audio part
 */
int
aud_import_avi_init (plugin_ctrl_t * attr, stream_info_t * info)
{
  struct avi_data *AVI_d;

  /* info string */
  strncpy (attr->info, "AVI import plugin", 32);

  /* check valid mode */
  if (attr->mode != PLUG_MODE_IMPORT)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }

  /* alloc memory */
  attr->handle = (struct avi_data *) malloc (sizeof (struct avi_data));
  if (!attr->handle)
  {
    LOG_Error ("malloc failed!");
    return -L4_ENOMEM;
  }
  memset (attr->handle, 0, sizeof (struct avi_data));
  AVI_d = (struct avi_data *) attr->handle;

  /* avi-file init */
  AVI_d->AVI = AVI_open_input_file (attr->filename, 1);
  if (AVI_d->AVI == NULL)
  {
    free (AVI_d);
    attr->handle = NULL;
    LOG_Error ("AVI_open_input_file() failed");
    return -L4_EOPEN;
  }

  /* Set stream info */
  /* type */
  info->type = STREAM_TYPE_AUDIO;
  /* avi opened - probing params */
  AVI_set_audio_track (AVI_d->AVI, attr->trackNo);
  info->ai.samplerate = AVI_audio_rate (AVI_d->AVI);
  info->ai.channels = AVI_audio_channels (AVI_d->AVI);
  info->ai.format = aud_codec2codec (AVI_audio_format (AVI_d->AVI));

  /* Calculate how many audio bytes to be read per video frame */
  AVI_d->audioBytesPerFrame =
    2 * AVI_audio_bytes (AVI_d->AVI) / AVI_video_frames (AVI_d->AVI);
  if (AVI_d->audioBytesPerFrame > MAX_AUD_CHUNK_SIZE)
  {
    Panic ("maximum audio chunk size is too small - contact MAINTAINER.");
    AVI_d->audioBytesPerFrame = MAX_AUD_CHUNK_SIZE;
  }

  /* we can read bitrate (will fail for some codecs */
  AVI_d->frameattr.ai.bitrate = info->ai.bitrate =
    AVI_audio_mp3rate (AVI_d->AVI);

  /* is it VBR? seek might fail! */
  AVI_d->vbr = (int) AVI_get_audio_vbr (AVI_d->AVI);

  /* bits per sample = 0 is false, so it should be 16 */
  info->ai.bits_per_sample = AVI_audio_bits (AVI_d->AVI);
  if (info->ai.bits_per_sample <= 0)
    info->ai.bits_per_sample = 16;	/* not sure, but mp3 has no way to determine - always 16 bit ? */

  /* set defaults for furher "step"-routines ! */
  AVI_d->frameattr.type = info->type;
  AVI_d->frameattr.ai.format = info->ai.format;
  AVI_d->frameattr.ai.samplerate = info->ai.samplerate;
  AVI_d->frameattr.ai.channels = info->ai.channels;
  AVI_d->frameattr.ai.bits_per_sample = info->ai.bits_per_sample;

  /* there's no buffer */
  attr->bufferSize = attr->bufferElements = 0;

  /* done */
  return 0;
}



/*
 * import one frame/chunk
 */

/* 
 * video part
 */
inline int
vid_import_avi_step (plugin_ctrl_t * attr, void *addr)
{

  int keyframe;
  long framesize;
  struct avi_data *AVI_d;
  frame_ctrl_t *packet_frameattr;

  AVI_d = (struct avi_data *) attr->handle;

  /* check for valid handle and mode */
  if (!attr->handle)
  {
    LOG_Error ("HANDLE FAILURE");
    return -L4_ENOHANDLE;
  }
  if (attr->mode != PLUG_MODE_IMPORT)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }

  /* read one frame */
  if ((framesize =
       AVI_read_frame (AVI_d->AVI, addr + sizeof (frame_ctrl_t),
		       &keyframe)) == -1)
  {
    LOGdL (DEBUG_IMPORT, "AVI_read_frame=-1 (EOF ?), FrameID=%i",
	   (int) AVI_d->frameID);
    return -L4_ENODATA;
  }

  /* filling frame infos into packet */
  packet_frameattr = (frame_ctrl_t *) addr;

  /* first copy unchangeable */
  memcpy (packet_frameattr, &AVI_d->frameattr, sizeof (frame_ctrl_t));

  /* Frametype */
  packet_frameattr->keyframe = keyframe;
  if (AVI_d->frameID == 0)	/* sync reset ptr */
  {
    packet_frameattr->keyframe = RESET_SYNC_POINT;
    packet_frameattr->last_sync_pts = 0.00;
  }

  /* check if next frame is an ZERO size frame, if so store in internal buffer */
  if ((AVI_frame_size (AVI_d->AVI, AVI_d->frameID + 1) == 0)
      && (framesize != 0))
  {
    /* we ignore all errors here ! */
    /* realloc to fit */
    AVI_d->lastFrameBuffer = realloc (AVI_d->lastFrameBuffer, framesize);
    if (AVI_d->lastFrameBuffer == NULL)
    {
      LOG_Error ("realloc failed.");
      return -L4_ENOMEM;
    }
    /* memcpy into internal buffer */
    memcpy (AVI_d->lastFrameBuffer, addr + sizeof (frame_ctrl_t), framesize);
  }

  /* Size */
  if ((framesize == 0) && (AVI_d->lastFrameSize))	/* duplicated frame ! */
  {
    /* memcpy into external buffer */
    memcpy (addr + sizeof (frame_ctrl_t), AVI_d->lastFrameBuffer,
	    AVI_d->lastFrameSize);
    packet_frameattr->framesize = AVI_d->lastFrameSize;
  }
  else
  {
    AVI_d->lastFrameSize = packet_frameattr->framesize = framesize;
  }
  /* Packetsize for DSI submit */
  attr->packetsize = packet_frameattr->framesize + sizeof (frame_ctrl_t);

  /* Frame ID */
  packet_frameattr->frameID = AVI_d->frameID;
  AVI_d->frameID++;

  return 0;
}

/*
 * audio part 
 */
inline int
aud_import_avi_step (plugin_ctrl_t * attr, void *addr)
{
  long framesize;
  struct avi_data *AVI_d;
  frame_ctrl_t *packet_frameattr;

  AVI_d = (struct avi_data *) attr->handle;

  /* check for valid handle and mode */
  if (!attr->handle)
  {
    LOG_Error ("HANDLE FAILURE");
    return -L4_ENOHANDLE;
  }
  if (attr->mode != PLUG_MODE_IMPORT)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }

  /* read one audio chunk */
  if ((framesize =
       AVI_read_audio (AVI_d->AVI, addr + sizeof (frame_ctrl_t),
		       AVI_d->audioBytesPerFrame)) <= 0)
  {
    LOGdL (DEBUG_IMPORT, "AVI_read_audio()<=0 (EOF ?), FrameID=%i",
	   (int) AVI_d->frameID);
    return -L4_ENOTFOUND;
  }

  /* filling frame infos into packet */
  packet_frameattr = (frame_ctrl_t *) addr;

  /* first copy unchangeable */
  memcpy (packet_frameattr, &AVI_d->frameattr, sizeof (frame_ctrl_t));

  /* frametype */
  packet_frameattr->keyframe = 1;

  if (AVI_d->frameID == 0)	/* sync reset ptr */
  {
    packet_frameattr->keyframe = RESET_SYNC_POINT;
    packet_frameattr->last_sync_pts = 0.00;
  }

  /* Size */
  packet_frameattr->framesize = framesize;
  /* Packetsize for DSI submit */
  attr->packetsize = packet_frameattr->framesize + sizeof (frame_ctrl_t);

  /* Frame ID */
  packet_frameattr->frameID = AVI_d->frameID;
  AVI_d->frameID++;

  /* done */
  return 0;
}



/* 
 * commit - unused 
 */
inline int
vid_import_avi_commit (plugin_ctrl_t * attr)
{
  /* no need for this */
  return 0;
}
inline int
aud_import_avi_commit (plugin_ctrl_t * attr)
{
  /* no need for this */
  return 0;
}



/*
 * seek to position (in millisec) 
 */


/*
 * video part
 */
inline int
vid_import_avi_seek (plugin_ctrl_t * attr, void *addr, double position,
		     int whence)
{
  long wish_frameID = 0;
  struct avi_data *AVI_d;
  frame_ctrl_t *frameattr;

  AVI_d = (struct avi_data *) attr->handle;

  /* check for valid handle and mode */
  if (!attr->handle)
  {
    LOG_Error ("HANDLE FAILURE");
    return -L4_ENOHANDLE;
  }
  if (attr->mode != PLUG_MODE_IMPORT)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }

  /* if current_pos>position - seek 2 start, then do it
     this is bad, but it works (I hope) */
  wish_frameID = (int) (position * AVI_frame_rate (AVI_d->AVI) / 1000);

  /* whence: current frameID + wish_frameID */
  if (whence == SEEK_RELATIVE)
    wish_frameID += AVI_d->frameID;

  LOGdL (DEBUG_IMPORT, "seeking to frameID=%i", (int) wish_frameID);

  if (AVI_d->frameID >= wish_frameID)
  {
    LOGdL (DEBUG_IMPORT, "seeking to start first.");
    AVI_seek_start (AVI_d->AVI);
    AVI_d->frameID = 0;
  }

  /* frame ctrl struct should be found at beginning of addr AFTER the first step call */
  frameattr = (frame_ctrl_t *) addr;

  /* import so long until granulepos is correct */
  while (1)
  {
    if (vid_import_avi_step (attr, addr))
    {
      LOGdL (DEBUG_IMPORT, "seek failed.");
      return -L4_ENOTFOUND;
    }
    /* if FrameID >= wish and Frame is a keyframe */
    if ((frameattr->frameID >= wish_frameID) && (frameattr->keyframe))
    {
      LOGdL (DEBUG_IMPORT, "seeked to frameID=%i", (int) frameattr->frameID);
      frameattr->keyframe = RESET_SYNC_POINT;
      frameattr->last_sync_pts =
	(double) ((frameattr->frameID) * 1000.00 /
		  AVI_frame_rate (AVI_d->AVI));
      /* done */
      return 0;
    }
  }

  /* failed */
  LOGdL (DEBUG_IMPORT, "seek failed.");
  return -L4_ENOTFOUND;
}

/*
 * audio part
 */
inline int
aud_import_avi_seek (plugin_ctrl_t * attr, void *addr, double position,
		     int whence)
{
  long framesize;
  struct avi_data *AVI_d;
  frame_ctrl_t *packet_frameattr;
  float audio_bitrate;		/* bitrate */
  long abyte;			/* position in bytes to jump to */

  AVI_d = (struct avi_data *) attr->handle;

  /* check for valid handle and mode */
  if (!attr->handle)
  {
    LOG_Error ("HANDLE FAILURE");
    return -L4_ENOHANDLE;
  }
  if (attr->mode != PLUG_MODE_IMPORT)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }

  /* if it's VBR - we have to warn ! */
  if (AVI_d->vbr)
    LOG_Error ("Seeking in a VBR audio stream will fail often!\n");

  /* calculate postion to sec to */
  /* kBit/s * 1024/8 = Byte/s , msec/1000 = sec */
  audio_bitrate = (double) AVI_audio_mp3rate (AVI_d->AVI) * 128 / 1000;	/*(1024/8) / 1000 */
  abyte = (long) ((double) position * audio_bitrate);

  /* seeking to calculate byte pos */
  AVI_set_audio_position (AVI_d->AVI, abyte);

  /* now we read a chunk */
  if ((framesize =
       AVI_read_audio (AVI_d->AVI, addr + sizeof (frame_ctrl_t),
		       AVI_d->audioBytesPerFrame)) <= 0)
  {
    Panic ("AVI_read_audio()<=0 (EOF ?)");
    return -L4_ENOTFOUND;
  }

  /* filling frame infos into packet */
  packet_frameattr = (frame_ctrl_t *) addr;

  /* first copy unchangeable */
  memcpy (packet_frameattr, &AVI_d->frameattr, sizeof (frame_ctrl_t));

  /* frametype */
  packet_frameattr->keyframe = RESET_SYNC_POINT;

  /* as we does not hit the exact position, we have to recalculate it */
  packet_frameattr->last_sync_pts = (double) abyte / audio_bitrate;
  LOGdL (DEBUG_IMPORT, "new position is %i",
	 (int) packet_frameattr->last_sync_pts);

  /* Size */
  packet_frameattr->framesize = framesize;
  /* Packetsize for DSI submit */
  attr->packetsize = packet_frameattr->framesize + sizeof (frame_ctrl_t);

  /* Frame ID */
  AVI_d->frameID = abyte / AVI_d->audioBytesPerFrame;
  packet_frameattr->frameID = AVI_d->frameID;

  /* done */
  return 0;
}



/*
 * close import plugin 
 */

/*
 * video part
 */
int
vid_import_avi_close (plugin_ctrl_t * attr)
{
  int status = 0;
  struct avi_data *AVI_d;
  AVI_d = (struct avi_data *) attr->handle;

  /* check for valid handle and mode */
  if (!attr->handle)
  {
    LOG_Error ("HANDLE FAILURE");
    return -L4_ENOHANDLE;
  }
  if (attr->mode != PLUG_MODE_IMPORT)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }

  /* close avi file */
  status = AVI_close (AVI_d->AVI);

  /* free buffer */
  if (AVI_d->lastFrameBuffer)
    free (AVI_d->lastFrameBuffer);

  /* struct free */
  free (AVI_d);
  attr->handle = NULL;

  return status;
}


/*
 * audio part
 */
int
aud_import_avi_close (plugin_ctrl_t * attr)
{
  int status = 0;
  struct avi_data *AVI_d;
  AVI_d = (struct avi_data *) attr->handle;

  /* check for valid handle and mode */
  if (!attr->handle)
  {
    LOG_Error ("HANDLE FAILURE");
    return -L4_ENOHANDLE;
  }
  if (attr->mode != PLUG_MODE_IMPORT)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }

  /* close avi */
  status = AVI_close (AVI_d->AVI);
  /* struct free */
  free (AVI_d);
  attr->handle = NULL;

  return status;
}
