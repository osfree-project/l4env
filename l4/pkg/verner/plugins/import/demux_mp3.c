/*
 * \brief   MP3 "container" plugin for VERNER
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

#include "demux_mp3.h"

/* L4 includes */
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* OSKit includes */
#include <stdlib.h>		/* malloc, free */

/* verner */
#include "arch_globals.h"
/* configuration */
#include "verner_config.h"

/* mp3 */
#include "mpg123.h"
#include "probe_mp3.h"

/* file I/O */
#include "fileops.h"


/* internal data */
struct mp3_data
{
  long frameID;
  /* File Descriptor */
  int fd;
  mp3_header_t mp3_header;
  unsigned long header;
  int header_pos;
  frame_ctrl_t frameattr;
};


/* init plugin */
int
aud_import_mp3_init (plugin_ctrl_t * attr, stream_info_t * info)
{
  struct mp3_data *MP3_d;
  unsigned char readbuf[4096];
  int readsize;

  /* info string */
  strncpy (attr->info, "MP3-Import-Plugin", 32);

  /* check valid mode */
  if (attr->mode != PLUG_MODE_IMPORT)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }

  /* alloc memory */
  attr->handle = (struct mp3_data *) malloc (sizeof (struct mp3_data));
  if (!attr->handle)
  {
    LOG_Error ("malloc failed!");
    return -L4_ENOMEM;
  }
  memset (attr->handle, 0, sizeof (struct mp3_data));
  MP3_d = (struct mp3_data *) attr->handle;

  /* open mp3 file */
  MP3_d->fd = fileops_open (attr->filename, 1);
  if (MP3_d->fd < 0)
  {
    free (MP3_d);
    attr->handle = NULL;
    LOG_Error ("fileops_open() failed");
    return -L4_EOPEN;
  }

  /* ok we should seek back to start, to decode all. */
  fileops_lseek (MP3_d->fd, 0, SEEK_SET);

  /* We have to find the mp3-header and decode it. If this fails say bye bye */
  readsize = fileops_read (MP3_d->fd, readbuf, 4096);

  if (readsize > 0)
    MP3_d->header_pos = find_mp3_header (readbuf, readsize, &MP3_d->header);
  else
    MP3_d->header_pos = -1;

  if (MP3_d->header_pos < 0)
  {
    LOG_Error ("no mp3 header found");
    fileops_close (MP3_d->fd);
    free (MP3_d);
    attr->handle = NULL;
    return -L4_ENOTSUPP;
  }

  /* ok, again we should seek back to first header, to decode all. */
  fileops_lseek (MP3_d->fd, MP3_d->header_pos, SEEK_SET);

  /* now decode header and fill stream info */
  decode_mp3_header (MP3_d->header, &MP3_d->mp3_header);

  /* Set stream info: 
     and set defaults for furher "step"-routines ! */
  /* type */
  MP3_d->frameattr.type = info->type = STREAM_TYPE_AUDIO;
  MP3_d->frameattr.ai.format = info->ai.format = AUD_FMT_MP3;
  MP3_d->frameattr.ai.samplerate = info->ai.samplerate =
    mp3_freqs[MP3_d->mp3_header.sampling_frequency];

  MP3_d->frameattr.ai.bitrate = info->ai.bitrate = mp3_tabsel[MP3_d->mp3_header.lsf][MP3_d->mp3_header.bitrate_index];	/* encoded */
  MP3_d->frameattr.ai.channels = info->ai.channels = MP3_d->mp3_header.stereo;

  /* we always expect 16 bits, don't know how to determine */
  MP3_d->frameattr.ai.bits_per_sample = info->ai.bits_per_sample = 16;

  /* now we trying to get mp3's idtag */
  if (get_mp3_taginfo (attr->filename, MP3_d->frameattr.info) != 0)
  {
    /* no tag found */
    strncpy (MP3_d->frameattr.info, attr->filename, 128);
  }

  /* there's no buffer */
  attr->bufferSize = attr->bufferElements = 0;

  /* done */
  return 0;
}


/* commit - unused */
inline int
aud_import_mp3_commit (plugin_ctrl_t * attr)
{
  /* no need for this */
  return 0;
}


/* import one frame */
inline int
aud_import_mp3_step (plugin_ctrl_t * attr, void *addr)
{
  long framesize;
  struct mp3_data *MP3_d;
  frame_ctrl_t *packet_frameattr;


  MP3_d = (struct mp3_data *) attr->handle;

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

  /* we want to get only one mp3 frame ! -> but is seeking required for this
     first we read 16 bytes and search for the header. */
  if ((framesize =
       fileops_read (MP3_d->fd, (addr + sizeof (frame_ctrl_t)), 64)) <= 0)
  {
    LOGdL (DEBUG_IMPORT, "fileops_read()<=0 (EOF ?), FrameID=%i",
	   (int) MP3_d->frameID);
    return -L4_ENOTFOUND;
  }
  MP3_d->header_pos =
    find_mp3_header ((addr + sizeof (frame_ctrl_t)), framesize,
		     &MP3_d->header);
  if (MP3_d->header_pos < 0)
  {
    LOGdL (DEBUG_IMPORT,
	   "no mp3 header found. (EOF or stream error) (framesize=%ld)",
	   framesize);
    return -L4_ENOTFOUND;
  }

  /* now decode header and get size of this frame */
  decode_mp3_header (MP3_d->header, &MP3_d->mp3_header);

  /* now we read the rest of the frame */
  if ((framesize +=
       fileops_read (MP3_d->fd,
		     (addr + sizeof (frame_ctrl_t) + framesize),
		     MP3_d->mp3_header.framesize - framesize + 4)) <= 0)
  {
    LOGdL (DEBUG_IMPORT, "fileops_read()<=0 (EOF ?), FrameID=%ld",
	   MP3_d->frameID);
    return -L4_ENOTFOUND;
  }

  /*LOGdL (DEBUG_IMPORT, "got frame size=%ld header=%i pos=%i", framesize,
	 MP3_d->mp3_header.framesize, MP3_d->header_pos);*/

  /* filling frame infos into packet */
  packet_frameattr = (frame_ctrl_t *) addr;

  /* first copy unchangeable */
  memcpy (packet_frameattr, &MP3_d->frameattr, sizeof (frame_ctrl_t));

  /* Frametype */
  packet_frameattr->keyframe = 1;

  if (MP3_d->frameID == 0)	//sync reset ptr
  {
    packet_frameattr->keyframe = RESET_SYNC_POINT;
    packet_frameattr->last_sync_pts = 0.00;
  }
  /* Size */
  packet_frameattr->framesize = framesize;
  /* Packetsize for DSI submit */
  attr->packetsize = packet_frameattr->framesize + sizeof (frame_ctrl_t);

  /* Frame ID */
  packet_frameattr->frameID = MP3_d->frameID;
  MP3_d->frameID++;

  return 0;
}



/* close import plugin */
int
aud_import_mp3_close (plugin_ctrl_t * attr)
{
  int status = 0;
  struct mp3_data *MP3_d;
  MP3_d = (struct mp3_data *) attr->handle;

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

  /* close file */
  status = fileops_close (MP3_d->fd);
  /* struct free */
  free (MP3_d);
  attr->handle = NULL;

  return status;
}



/* seek to position (in millisec) */
inline int
aud_import_mp3_seek (plugin_ctrl_t * attr, void *addr, double position,
		     int whence)
{
  long wish_frameID = 0;
  struct mp3_data *MP3_d;
  frame_ctrl_t *packet_frameattr;
  double samples_per_msec, one_frame_msec;

  MP3_d = (struct mp3_data *) attr->handle;

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

  /* NOTE: one mp3-frame is 1152 samples ! */
  samples_per_msec = (double) MP3_d->frameattr.ai.samplerate / 1000.00;
  one_frame_msec = (double) 1152 / samples_per_msec;
  MP3_d->frameID++;

  /* this is bad, but it works (I hope) */
  wish_frameID = (int) (position / one_frame_msec);

  /* whence: current frameID + wish_frameID */
  if (whence == SEEK_RELATIVE)
    wish_frameID += MP3_d->frameID;
  LOGdL (DEBUG_IMPORT, "seeking to frameID=%ld", wish_frameID);

  /* if current_pos>position - seek 2 start, then do it */
  if (MP3_d->frameID >= wish_frameID)
  {
    LOGdL (DEBUG_IMPORT, "seeking to start first.");
    /* ok we should seek back to start, to decode all. */
    fileops_lseek (MP3_d->fd, 0, SEEK_SET);
    MP3_d->frameID = 0;
  }

  /* frame ctrl struct should be found at beginning of addr AFTER the first step call */
  packet_frameattr = (frame_ctrl_t *) addr;


  /* import so long until postion is correct */
  while (1)
  {
    if (aud_import_mp3_step (attr, addr))
    {
      LOGdL (DEBUG_IMPORT, "seek failed.");
      return -L4_ENOTFOUND;
    }
    /* if FrameID >= wish and Frame is a keyframe */
    if ((packet_frameattr->frameID >= wish_frameID)
	&& (packet_frameattr->keyframe))
    {
      LOGdL (DEBUG_IMPORT, "seeked to frameID=%ld",
	     packet_frameattr->frameID);
      packet_frameattr->keyframe = RESET_SYNC_POINT;
      packet_frameattr->last_sync_pts =
	(double) ((packet_frameattr->frameID) * one_frame_msec);
      /* done */
      return 0;
    }
  }

  LOGdL (DEBUG_IMPORT, "seek failed.");
  return 0;
}

/* video functions are useless for MP3 :) */
int
vid_import_mp3_init (plugin_ctrl_t * attr, stream_info_t * info)
{
  return -L4_ENOTSUPP;
}

int
vid_import_mp3_commit (plugin_ctrl_t * attr)
{
  return -L4_ENOTSUPP;
}

int
vid_import_mp3_step (plugin_ctrl_t * attr, void *addr)
{
  return -L4_ENOTSUPP;
}

int
vid_import_mp3_close (plugin_ctrl_t * attr)
{
  return -L4_ENOTSUPP;
}

int
vid_import_mp3_seek (plugin_ctrl_t * attr, void *addr, double position,
		     int whence)
{
  return -L4_ENOTSUPP;
}
