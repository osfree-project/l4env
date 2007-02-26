/*
 * \brief   OGM container plugin for VERNER
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

#include "demux_ogm.h"

/* L4 includes */
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/lock/lock.h>

/* OSKit includes */
#include <stdlib.h>		/* malloc, free */

/* ogm */
#include "ogmlib.h"

/* verner */
#include "arch_globals.h"

/* configuration */
#include "verner_config.h"

/* internal data */
typedef struct
{
  /* ogg_t for usage in ogmlib */
  ogg_t *OGG;
  /* filename of opened file */
  char *filename;
  /* video */
  int vid_in_use;		/* already videopart in usage ? */
  int vid_serial;		/* serial no. of OGM */
  long vid_frameID;
  double vid_framerate;		/* framerate */
  frame_ctrl_t vid_frameattr;
  /* audio */
  int aud_in_use;		/* already videopart in usage ? */
  int aud_serial;
  long aud_chunkID;
  frame_ctrl_t aud_frameattr;
  double aud_last_end_pts;	/* last end-pts for audio stream */
  /* seek */
  int seek_type;		/* streamtype of last seeked stream */
  double seek_position;		/* position we seeked to */
}
ogm_ctrl_t;
/* 
 * control struct for one OGM file (used by audio and video)
 *
 * When opening the file we check if it's already in use for wanted streamtype (audio or video)
 * if yes: can't open
 * else: check if other streamtype is already used, if so, then compare filename and fail if it's
 * not the same.
 * Open OGM. on success -> set in_use to one and remember filename
 * Read data.
 * On close, if both streamtypes unused -> close OGM, else do nothing.
 *
 * !!! For more than one file opened at once you have to alloc a new ogm_ctrl_t
 * !!! and build a list or something else to use it in video and audio part.
 * !!! Connect it via plugin_in_ctrl_t->handle (attr->handle!).
 *
 * Seeking: first come first seeks :)
 * When seeking we lock a semaphore to ensure only one seek can be done at once.
 * We remember the timestamp we should seek to and disable ogmlib-caching for all streams (remember 
 * their serials). Perform seek and reenable caching. Unlock.
 * Expect the command to seek for the other streamtype too. As we have already 
 * seeked, we check if the value we should seek to is the same. If yes, don't do nothing except searching
 * the next keyframe. If not, perform complete seek as descripted above.
 *
 */
#define SEEK_POSITION_INVALID -1.00
static ogm_ctrl_t OGM_ctrl = {
  .OGG = NULL,
  .filename = NULL,
  .vid_in_use = 0,
  .aud_in_use = 0,
  .seek_type = STREAM_TYPE_INVALID,
  .seek_position = SEEK_POSITION_INVALID
};


/* 
 * lock to protect access of OGM_ctrl->OGG and it's usage
 * !!! ogmlib does not take care of access conflicts for one ogg_t
 * so it MUST be handle here !!!
 */
static l4lock_t OGM_lock = L4LOCK_UNLOCKED_INITIALIZER;

/*
 * internal helper function to avoid duplicate code 
 */
static inline int ogm_init (plugin_ctrl_t * attr, stream_info_t * info,
			    unsigned int type);
static inline int ogm_step (plugin_ctrl_t * attr, void *addr,
			    unsigned int type);
static inline int ogm_seek (plugin_ctrl_t * attr, void *addr, double position,
			    int whence, unsigned int type);
static inline int ogm_close (plugin_ctrl_t * attr, unsigned int type);

/*
 * init plugin 
 */

/*
 * video part
 */
int
vid_import_ogm_init (plugin_ctrl_t * attr, stream_info_t * info)
{
  return ogm_init (attr, info, STREAM_TYPE_VIDEO);
}

/*
 * audio part 
 */
int
aud_import_ogm_init (plugin_ctrl_t * attr, stream_info_t * info)
{
  return ogm_init (attr, info, STREAM_TYPE_AUDIO);
}

/*
 * helper function for open
 */
static int
ogm_init (plugin_ctrl_t * attr, stream_info_t * info, unsigned int type)
{
  int err;
  char *compressor;

  /* info string */
  strncpy (attr->info, "OGM-Import-Plugin", 32);

  /* check valid mode */
  if (attr->mode != PLUG_MODE_IMPORT)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }

  /* lock */
  l4lock_lock (&OGM_lock);

  /* check if already used */
  if (((OGM_ctrl.aud_in_use) && (type == STREAM_TYPE_AUDIO))
      || ((OGM_ctrl.vid_in_use) && (type == STREAM_TYPE_VIDEO)))
  {
    LOG_Error
      ("Only one %s stream can be read at once! Already one opened.\n",
       type == STREAM_TYPE_AUDIO ? "audio" : "video");
    err = -L4_ENOTSUPP;
    goto open_fail;
  }

  /* check if video is in use and compare filename if neccassary */
  if (((OGM_ctrl.vid_in_use) && (type == STREAM_TYPE_AUDIO))
      || ((OGM_ctrl.aud_in_use) && (type == STREAM_TYPE_VIDEO)))
  {
    if (OGM_ctrl.filename)
      if (strcmp (OGM_ctrl.filename, attr->filename))
      {
	/* it's another one */
	LOG_Error ("Audio and video can only be read from the same file!\n");
	err = -L4_ENOTSUPP;
	goto open_fail;
      }
  }

  /* open OGM if not already done */
  if (OGM_ctrl.OGG == NULL)
  {
    OGM_ctrl.OGG = OGG_streaming_open_input_file (attr->filename);
    if (OGM_ctrl.OGG == NULL)
    {
      LOG_Error ("OGG_open_input_file() failed!");
      err = -L4_EOPEN;
      goto open_fail;
    }
  }

  switch (type)
  {

    /* audio */
  case STREAM_TYPE_AUDIO:
    {
      if (OGG_get_audio_track (OGM_ctrl.OGG) <= 0)
      {
	LOG_Error ("OGM has no audio tracks!");
	err = -L4_EOPEN;
	goto open_fail;
      }
      /* try to set trackno */
      if (OGG_set_audio_track (OGM_ctrl.OGG, attr->trackNo))
	LOG_Error ("failt to set video track correct - taking first one.");

      /* get serial for video stream */
      OGM_ctrl.aud_serial = OGG_get_audio_serial (OGM_ctrl.OGG);

      /* setup buffer for video stream */
      OGG_streaming_setup_buffer (OGM_ctrl.OGG, 1, OGM_ctrl.aud_serial);

      /* Set stream info */
      /* type */
      info->type = STREAM_TYPE_AUDIO;

      /* OGM opened - probing params */
      info->ai.format = aud_codec2codec (OGG_audio_format (OGM_ctrl.OGG));

      /* for mp3 we can read bitrate */
      if (info->ai.format == AUD_FMT_MP3)
	info->ai.bitrate = OGG_audio_mp3rate (OGM_ctrl.OGG);
      else
	info->ai.bitrate = 0;	/* unknown */
      info->ai.samplerate = OGG_audio_rate (OGM_ctrl.OGG);
      info->ai.channels = OGG_audio_channels (OGM_ctrl.OGG);
      info->ai.bits_per_sample = OGG_audio_bits (OGM_ctrl.OGG);

      /* set defaults for furher "step"-routines ! */
      OGM_ctrl.aud_frameattr.type = info->type;
      OGM_ctrl.aud_frameattr.ai.format = info->ai.format;
      OGM_ctrl.aud_frameattr.ai.samplerate = info->ai.samplerate;
      OGM_ctrl.aud_frameattr.ai.bitrate = info->ai.bitrate;
      OGM_ctrl.aud_frameattr.ai.channels = info->ai.channels;
      OGM_ctrl.aud_frameattr.ai.bits_per_sample = info->ai.bits_per_sample;
      OGM_ctrl.aud_frameattr.keyframe = 1;	/* audio is always a keyframe */

      /* set audio used */
      OGM_ctrl.aud_in_use = 1;
      /* chunks start at 0 */
      OGM_ctrl.aud_chunkID = 0;
    }
    break;
    /* video */

  case STREAM_TYPE_VIDEO:
    {
      if (OGG_get_video_track (OGM_ctrl.OGG) <= 0)
      {
	LOG_Error ("OGM has no video tracks!");
	err = -L4_EOPEN;
	goto open_fail;
      }
      /* try to set trackno */
      if (OGG_set_video_track (OGM_ctrl.OGG, attr->trackNo))
	LOG_Error ("failt to set video track correct - taking first one.");

      /* get serial for video stream */
      OGM_ctrl.vid_serial = OGG_get_video_serial (OGM_ctrl.OGG);

      /* setup buffer for video stream */
      OGG_streaming_setup_buffer (OGM_ctrl.OGG, 1, OGM_ctrl.vid_serial);

      /* Set stream info */
      /* type */
      info->type = STREAM_TYPE_VIDEO;
      /* OGM opened - probing */
      compressor = OGG_video_compressor (OGM_ctrl.OGG);
      info->vi.fourCC = vid_fourcc2int (compressor);
      info->vi.format = vid_fourcc2fmt (compressor);
      OGM_ctrl.vid_framerate = info->vi.framerate =
	OGG_frame_rate (OGM_ctrl.OGG);
      info->vi.xdim = OGG_video_width (OGM_ctrl.OGG);
      info->vi.ydim = OGG_video_height (OGM_ctrl.OGG);

      /* set video used */
      OGM_ctrl.vid_in_use = 1;

      /* If we're not sure, we assume RGB32 for buffer sizes */
      if (strlen (compressor) == 4)
      {
	if (!strncasecmp (compressor, "YV12", 4))
	  info->vi.colorspace = VID_YV12;
	else if (!strncasecmp (compressor, "UYVY", 4))
	  info->vi.colorspace = VID_UYVY;
	else
	  info->vi.colorspace = VID_RGB32;
      }
      else
	info->vi.colorspace = VID_RGB32;

      /* set defaults for furher "step"-routines ! */
      OGM_ctrl.vid_frameattr.type = info->type;
      OGM_ctrl.vid_frameattr.vi.format = info->vi.format;
      OGM_ctrl.vid_frameattr.vi.fourCC = info->vi.fourCC;
      OGM_ctrl.vid_frameattr.vi.framerate = info->vi.framerate;
      OGM_ctrl.vid_frameattr.vi.xdim = info->vi.xdim;
      OGM_ctrl.vid_frameattr.vi.ydim = info->vi.ydim;
      OGM_ctrl.vid_frameattr.vi.colorspace = info->vi.colorspace;

      /* set video used */
      OGM_ctrl.vid_in_use = 1;
      /* frames start at 0 */
      OGM_ctrl.vid_frameID = 0;

    }
    break;

    /* ??? either video nor audio */
  default:
    Panic ("unknown streamtype. BUG=YES.");

  }				/* end switch */

  /* there's no buffer */
  attr->bufferSize = attr->bufferElements = 0;

  /* copy filename */
  if (OGM_ctrl.filename == NULL)
  {
    OGM_ctrl.filename = malloc (strlen (attr->filename + 1));
    strcpy (OGM_ctrl.filename, attr->filename);
  }

  /* set handle to magic code */
  attr->handle = "OgM";

  /* unlock */
  l4lock_unlock (&OGM_lock);
  /* done */
  return 0;

open_fail:
  /* set failed unused */
  if (type == STREAM_TYPE_AUDIO)
    OGM_ctrl.aud_in_use = 0;
  else
    OGM_ctrl.vid_in_use = 0;

  /* the last one is closing the ogm and sets OGG and filename to NULL */
  if ((!OGM_ctrl.vid_in_use) && (!OGM_ctrl.aud_in_use))
  {
    /* free filename */
    if (OGM_ctrl.filename)
      free (OGM_ctrl.filename);
    OGM_ctrl.filename = NULL;
    /* close ogm */
    if (OGM_ctrl.OGG)
      OGG_close (OGM_ctrl.OGG);
    OGM_ctrl.OGG = NULL;
    /* set handle to invalid */
    attr->handle = 0;
  }
  /* unlock */
  l4lock_unlock (&OGM_lock);
  /* return error value */
  return err;
}



/*
 * import one frame/chunk 
 */

/* 
 * video part 
 */
inline int
vid_import_ogm_step (plugin_ctrl_t * attr, void *addr)
{
  return ogm_step (attr, addr, STREAM_TYPE_VIDEO);
}

/*
 * audio part
 */
inline int
aud_import_ogm_step (plugin_ctrl_t * attr, void *addr)
{
  return ogm_step (attr, addr, STREAM_TYPE_AUDIO);
}

/*
 * helper function for step
 */
static int
ogm_step (plugin_ctrl_t * attr, void *addr, unsigned int type)
{
  /* framesize of returned data */
  long framesize;
  /* keyframe */
  int keyframe;
  /* for filling frame infos into packet */
  frame_ctrl_t *packet_frameattr;
  /* last position to detect wrap */
  double last_pts = SEEK_POSITION_INVALID;

  /* check for valid handle and mode */
  if (attr->handle != "OgM")
  {
    LOG_Error ("HANDLE FAILURE");
    return -L4_ENOHANDLE;
  }
  if (attr->mode != PLUG_MODE_IMPORT)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }

  /* lock */
  l4lock_lock (&OGM_lock);

  /* for filling frame infos into packet */
  packet_frameattr = (frame_ctrl_t *) addr;

  switch (type)
  {

    /* audio */
  case STREAM_TYPE_AUDIO:
    {
      /* first copy unchangeable */
      memcpy (packet_frameattr, &OGM_ctrl.aud_frameattr,
	      sizeof (frame_ctrl_t));
      /* read one frame */
      framesize =
	OGG_streaming_read_buffered_data (OGM_ctrl.OGG, OGM_ctrl.aud_serial,
					  addr + sizeof (frame_ctrl_t),
					  &keyframe);
      /* check return value of OGG's read */
      if (framesize < 0)
	break;

      /* 
       * check for gaps or packet loss:
       * the video's one is more precise then this one, but seems the only way
       * to do it for all audio codecs embedded in ogm (mp3, vorbis, wav, ...)
       * It works as follows:
       *   compare last end-pts with current start-pts
       *   if these aren't the same --> gap --> resync
       * (only compare when not seeking and if it's not the first chunk)
       */
      if ((OGM_ctrl.OGG->start_pts > OGM_ctrl.aud_last_end_pts)
	  && (OGM_ctrl.seek_position == SEEK_POSITION_INVALID)
	  && (OGM_ctrl.aud_chunkID != 0))
      {
	LOG
	  ("Gotcha! Gap detected at %ims in audio stream.",
	   (int) OGM_ctrl.OGG->start_pts);
	/* 
	 * check for granlepos wrap for more accurate timestamps 
	 * but audio only, cause in video streams a keyframe seems to have always
	 * a new pts, while a few audio chunks are grouped with the same pts
	 * only when video is also demuxed 
	 */
	if (OGM_ctrl.vid_in_use)
	{
	  /* wait for wrap */
	  while (((OGM_ctrl.OGG->start_pts == last_pts)
		  || (last_pts == SEEK_POSITION_INVALID)) && (framesize >= 0))
	  {
	    /* not wrapped - try next audio chunk */
	    last_pts = OGM_ctrl.OGG->start_pts;
	    /* read next frame */
	    framesize =
	      OGG_streaming_read_buffered_data (OGM_ctrl.OGG,
						OGM_ctrl.aud_serial,
						addr + sizeof (frame_ctrl_t),
						&keyframe);
	  }
	}

	packet_frameattr->keyframe = RESET_SYNC_POINT;
	packet_frameattr->last_sync_pts = (double) OGM_ctrl.OGG->start_pts;
      }

      /* sync reset ptr is always at first audio chunk */
      if (OGM_ctrl.aud_chunkID == 0)
      {
	packet_frameattr->keyframe = RESET_SYNC_POINT;
	packet_frameattr->last_sync_pts = (double) OGM_ctrl.OGG->start_pts;
      }
      /* chunk ID */
      packet_frameattr->frameID = OGM_ctrl.aud_chunkID;
      OGM_ctrl.aud_chunkID++;
      /* remember last end-pts for gap detection */
      OGM_ctrl.aud_last_end_pts = OGM_ctrl.OGG->end_pts;
    }
    break;

    /* video */
  case STREAM_TYPE_VIDEO:
    {
      /* first copy unchangeable */
      memcpy (packet_frameattr, &OGM_ctrl.vid_frameattr,
	      sizeof (frame_ctrl_t));
      /* read one frame */
      framesize =
	OGG_streaming_read_buffered_data (OGM_ctrl.OGG, OGM_ctrl.vid_serial,
					  addr + sizeof (frame_ctrl_t),
					  &packet_frameattr->keyframe);

      /* check return value of OGG's read */
      if (framesize < 0)
	break;

      /* 
       * check for gaps or packet loss:
       * calculate the pts from current frameID (that's were we should be)
       * and compare with OGG's current pts, if our is the smaller one
       * --> gotcha, gap found. Then set reset_sync_point and find a keyframe
       * don't check while seeking 
       */
      if ((((OGM_ctrl.vid_frameID * 1000.00) /
	    packet_frameattr->vi.framerate) < OGM_ctrl.OGG->start_pts)
	  && (OGM_ctrl.seek_position == SEEK_POSITION_INVALID))
      {
	LOG
	  ("Gotcha! Gap detected at %lums in video stream. Should be at %ims.",
	   (long) OGM_ctrl.OGG->start_pts,
	   (int) ((OGM_ctrl.vid_frameID * 1000.00) /
		  packet_frameattr->vi.framerate));
	/* fetch next keyframe */
	while ((!packet_frameattr->keyframe) && (framesize >= 0))
	{
	  framesize =
	    OGG_streaming_read_buffered_data (OGM_ctrl.OGG,
					      OGM_ctrl.vid_serial,
					      addr + sizeof (frame_ctrl_t),
					      &packet_frameattr->keyframe);
	}
	/* calculate frameID were we should be */
	OGM_ctrl.vid_frameID =
	  (long) (OGM_ctrl.OGG->start_pts * packet_frameattr->vi.framerate /
		  1000.00);
	packet_frameattr->keyframe = RESET_SYNC_POINT;
	packet_frameattr->last_sync_pts = (double) OGM_ctrl.OGG->start_pts;
      }

      /* first frame is always a sync reset ptr */
      if (OGM_ctrl.vid_frameID == 0)
      {
	packet_frameattr->keyframe = RESET_SYNC_POINT;
	packet_frameattr->last_sync_pts = (double) OGM_ctrl.OGG->start_pts;
      }

      /* set frame ID */
      packet_frameattr->frameID = OGM_ctrl.vid_frameID;
      OGM_ctrl.vid_frameID++;
    }
    break;

    /* ??? either video nor audio */
  default:
    Panic ("unknown streamtype. BUG=YES.");

  }				/* end switch */


  /* check return value of OGG's read */
  if (framesize < 0)
  {
    LOGdL (DEBUG_IMPORT, "Read frame returned -1 (EOF ?) at %s no %i.",
	   type == STREAM_TYPE_AUDIO ? "chunk" : "frame",
	   (int) OGM_ctrl.aud_chunkID);
    /* unlock */
    l4lock_unlock (&OGM_lock);
    return -L4_ENOTFOUND;
  }

  /* size */
  packet_frameattr->framesize = framesize;
  /* packetsize for DSI submit */
  attr->packetsize = packet_frameattr->framesize + sizeof (frame_ctrl_t);

  /* unlock */
  l4lock_unlock (&OGM_lock);

  /* done */
  return 0;
}



/*
 *  commit - unused, unneeded
 */
inline int
vid_import_ogm_commit (plugin_ctrl_t * attr)
{
  return 0;
}
inline int
aud_import_ogm_commit (plugin_ctrl_t * attr)
{
  return 0;
}



/*
 *  seek to position (in millisec) 
 */


/*
 * video part 
 */
inline int
vid_import_ogm_seek (plugin_ctrl_t * attr, void *addr, double position,
		     int whence)
{
  return ogm_seek (attr, addr, position, whence, STREAM_TYPE_VIDEO);
}

/*
 * audio part 
 */
inline int
aud_import_ogm_seek (plugin_ctrl_t * attr, void *addr, double position,
		     int whence)
{
  /* 
   * small optimisation:
   * Let video seek first if video exists. 
   * Less async after seek cause video has very often much less keyframes.
   */

  /* lock */
  l4lock_lock (&OGM_lock);
  if (OGM_ctrl.vid_in_use)
  {
    while ((OGM_ctrl.seek_type != STREAM_TYPE_VIDEO) && (OGM_ctrl.vid_in_use))
    {
      /* unlock to let video in */
      l4lock_unlock (&OGM_lock);
      /* wait for video - yes it's an active wait, but who cares at seek ? */
      l4thread_sleep (40);	/* about one frame at 25fps */
      /* lock */
      l4lock_lock (&OGM_lock);
    }
  }
  /* unlock */
  l4lock_unlock (&OGM_lock);
  /* process seek */
  return ogm_seek (attr, addr, position, whence, STREAM_TYPE_AUDIO);
}

/*
 * helper function for seek
 */
static int
ogm_seek (plugin_ctrl_t * attr, void *addr, double position,
	  int whence, unsigned int type)
{
  /* frame attributes */
  frame_ctrl_t *frameattr;
  /* return value */
  int err = 0;
  /* last position to detect wrap */
  double last_pts = SEEK_POSITION_INVALID;
  /* flag is set if the other stream has already perfomed seek */
  int seeked = 0;
  /* type output for verbose */
  char *streamtype;
  if (type == STREAM_TYPE_AUDIO)
    streamtype = "audio";
  else
    streamtype = "video";

  /* check for valid handle and mode */
  if (attr->handle != "OgM")
  {
    LOG_Error ("HANDLE FAILURE");
    return -L4_ENOHANDLE;
  }
  if (attr->mode != PLUG_MODE_IMPORT)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }

  /* frame ctrl struct should be found at beginning of addr AFTER the first step call */
  frameattr = (frame_ctrl_t *) addr;

  /* lock */
  l4lock_lock (&OGM_lock);

  /* whence: relativ is wanted position + current position */
  if (whence == SEEK_RELATIVE)
    position += OGM_ctrl.OGG->start_pts;
  LOGdL (DEBUG_IMPORT, "%s seeking to position %i ms.", streamtype,
	 (int) position);

  /* check if we have already seeked for video to the same position */
  if ((OGM_ctrl.seek_position == position)
      && (OGM_ctrl.seek_type ==
	  (type ==
	   STREAM_TYPE_AUDIO ? STREAM_TYPE_VIDEO : STREAM_TYPE_AUDIO)))
  {
    LOGdL (DEBUG_IMPORT,
	   "%s already seeked to this position-fetching next keyframe.",
	   type == STREAM_TYPE_AUDIO ? "video" : "audio");
    /* setting position to an invalid value so seek is performed next time */
    OGM_ctrl.seek_position = SEEK_POSITION_INVALID;
    OGM_ctrl.seek_type = STREAM_TYPE_INVALID;
    /* set flag */
    seeked = 1;
  }

  /* perform seek */
  if (!seeked)
  {
    /* remember position for next seek */
    OGM_ctrl.seek_position = position;
    OGM_ctrl.seek_type = type;

    /* disble buffering */
    if (OGM_ctrl.vid_in_use == 1)
      OGG_streaming_remove_buffer (OGM_ctrl.OGG, 1, OGM_ctrl.vid_serial);
    if (OGM_ctrl.aud_in_use == 1)
      OGG_streaming_remove_buffer (OGM_ctrl.OGG, 1, OGM_ctrl.aud_serial);

    /* check current OGG->start_pos */
    if (OGM_ctrl.OGG->start_pts >= position)
    {
      LOGdL (DEBUG_IMPORT, "%s seeking to start first.", streamtype);
      OGG_seek_start (OGM_ctrl.OGG);
      if (type == STREAM_TYPE_AUDIO)
	OGM_ctrl.aud_chunkID = 0;
      else
	OGM_ctrl.vid_frameID = 0;
    }
  }

  /* loop until we found correct position */
  while (1)
  {
    if (type ==
	STREAM_TYPE_AUDIO ? aud_import_ogm_step (attr,
						 addr) :
	vid_import_ogm_step (attr, addr))
    {
      LOGdL (DEBUG_IMPORT, "%s seek failed.", streamtype);
      err = -L4_ENOTFOUND;
      goto seek_unlock;
    }

    /* is it in correct time ? and a sync point ? */
    if ((OGM_ctrl.OGG->start_pts >= position) && (frameattr->keyframe))
    {
      /* 
       * check for granlepos wrap for more accurate timestamps 
       * but audio only, cause in video streams a keyframe seems to have always
       * a new pts, while a few audio chunks are grouped with the same pts
       * only when video is also demuxed 
       */
      if ((type == STREAM_TYPE_AUDIO) && (OGM_ctrl.vid_in_use))
      {
	/* check for wrap */
	if ((OGM_ctrl.OGG->start_pts == last_pts)
	    || (last_pts == SEEK_POSITION_INVALID))
	{
	  /* not wrapped - try next audio chunk */
	  last_pts = OGM_ctrl.OGG->start_pts;
	  continue;
	}
      }
      /* is just the first frame after seek, so recalc it's frame position */
      else if (type == STREAM_TYPE_VIDEO)
      {
	/* calculate frameID were we should be */
	OGM_ctrl.vid_frameID =
	  (long) (OGM_ctrl.OGG->start_pts * frameattr->vi.framerate /
		  1000.00);
	/* manipulate frameID of current frame and set +1 for next one */
	frameattr->frameID = OGM_ctrl.vid_frameID;
	OGM_ctrl.vid_frameID++;
      }

      LOGdL (DEBUG_IMPORT, "%s seeked to position %i ms.", streamtype,
	     (int) OGM_ctrl.OGG->start_pts);
      frameattr->last_sync_pts = (double) OGM_ctrl.OGG->start_pts;
      frameattr->keyframe = RESET_SYNC_POINT;
      /* done */
      err = 0;
      goto seek_unlock;
    }
  }

seek_unlock:
  if (!seeked)
  {
    /* reenable buffering only when disabled */
    if (OGM_ctrl.vid_in_use == 1)
      OGG_streaming_setup_buffer (OGM_ctrl.OGG, 1, OGM_ctrl.vid_serial);
    if (OGM_ctrl.aud_in_use == 1)
      OGG_streaming_setup_buffer (OGM_ctrl.OGG, 1, OGM_ctrl.aud_serial);
  }
  /* unlock */
  l4lock_unlock (&OGM_lock);
  /* done */
  return err;
}



/*
 * close import plugin 
 */

/*
 * video part 
 */
int
vid_import_ogm_close (plugin_ctrl_t * attr)
{
  return ogm_close (attr, STREAM_TYPE_VIDEO);
}

/*
 * audio part 
 */
int
aud_import_ogm_close (plugin_ctrl_t * attr)
{
  return ogm_close (attr, STREAM_TYPE_AUDIO);
}

/*
 * helper function for close 
 */
static int
ogm_close (plugin_ctrl_t * attr, unsigned int type)
{

  /* check for valid handle and mode */
  if (attr->handle != "OgM")
  {
    LOG_Error ("HANDLE FAILURE");
    return -L4_ENOHANDLE;
  }
  if (attr->mode != PLUG_MODE_IMPORT)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }

  /* lock */
  l4lock_lock (&OGM_lock);

  /* remove and flush buffer */
  OGG_streaming_remove_buffer (OGM_ctrl.OGG, 1,
			       type ==
			       STREAM_TYPE_AUDIO ? OGM_ctrl.
			       aud_serial : OGM_ctrl.vid_serial);

  /* the last one is closing the ogm and sets OGG and filename to NULL */
  if (((!OGM_ctrl.vid_in_use) && (type == STREAM_TYPE_AUDIO))
      || ((!OGM_ctrl.aud_in_use) && (type == STREAM_TYPE_VIDEO)))
  {
    /* free filename */
    if (OGM_ctrl.filename)
      free (OGM_ctrl.filename);
    OGM_ctrl.filename = NULL;
    /* print stats */
    OGG_streaming_stats (OGM_ctrl.OGG);
    /* close ogm */
    if (OGM_ctrl.OGG)
      OGG_close (OGM_ctrl.OGG);
    OGM_ctrl.OGG = NULL;
    /* set handle to invalid */
    attr->handle = 0;

  }
  /* set video unused */
  if (type == STREAM_TYPE_AUDIO)
    OGM_ctrl.aud_in_use = 0;
  else
    OGM_ctrl.vid_in_use = 0;

  /* unlock */
  l4lock_unlock (&OGM_lock);

  /* bye */
  return 0;
}
