/*
 * \brief   container specific for VERNER's demuxer
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

/* L4/DROPS includes */
#include <l4/env/errno.h>
#include <l4/sys/types.h>
#include <l4/util/macros.h>

/* verner */
#include "arch_types.h"
#include "arch_globals.h"
#include "arch_plugins.h"
#include "fileops.h"		/* open, close, fstat */

/* local */
#include "container.h"

/* configuration */
#include "verner_config.h"

/* container libs */
#if VDEMUXER_BUILD_AVILIB
#include "avilib.h"
#endif
#if VDEMUXER_BUILD_OGMLIB
#include "ogmlib.h"
#endif
/* not ready yet - #include "avformat.h" */

/* probe libs */
#include "probe_mpeg.h"
#include "probe_wav.h"
#include "probe_mp3.h"
#include "probe_avi.h"
#include "probe_ogm.h"

/*****************************************************************************/
/**
 * \brief Determines file type and sets up all necassary information/ptr.
 * 
 * \retval control structure containing all necassary informations to demux.
 *
 */
/*****************************************************************************/
int
determineAndSetupImport (control_struct_t * control)
{
  int plugin_id = control->plugin_id;

  switch (control->stream_type)
  {
  case STREAM_TYPE_VIDEO:
    {

      /*  video import disabled ? */
      if (control->plugin_ctrl.trackNo < 0)
      {
	LOG_Error ("demuxing video is disabled (trackNo -1).\n");
	return -L4_ENOTFOUND;
      }

      /* filename set ? */
      if (!control->plugin_ctrl.filename)
      {
	LOG_Error ("no filename set.\n");
	return -L4_ENOTFOUND;
      }

      /* autodetect or force ? */
      if (plugin_id < 0)
      {
	/* autodectect */
	LOGdL (DEBUG_IMPORT, "autodetecting plugin.");

	/* of course we want to import */
	control->plugin_ctrl.mode = PLUG_MODE_IMPORT;

	/*  AVI */
	if (probe_avi (control->plugin_ctrl.filename))
	{
	  plugin_id =
	    find_plugin_by_name (PLUG_NAME_AVI, PLUG_MODE_IMPORT,
				 STREAM_TYPE_VIDEO);
	}
	/* OGM */
	else if (probe_ogm (control->plugin_ctrl.filename))
	{
	  plugin_id =
	    find_plugin_by_name (PLUG_NAME_OGM, PLUG_MODE_IMPORT,
				 STREAM_TYPE_VIDEO);
	}
	/* MPEG(2) */
	else if (probe_mpeg (control->plugin_ctrl.filename))
	{
	  plugin_id =
	    find_plugin_by_name (PLUG_NAME_MPEG, PLUG_MODE_IMPORT,
				 STREAM_TYPE_VIDEO);
	  /* we set some defaults here, and later the plugin distinguishs between MPEG-1 or -2 */
	  control->streaminfo.vi.colorspace = VID_YV12;
	  control->streaminfo.vi.format = VID_FMT_MPEG1;
	  control->streaminfo.vi.fourCC = vid_fourcc2int ("MPG1");
	  control->streaminfo.vi.framerate = 25.00;
	}

      }				/* end autodetect plugin */
      else
      {
	/* force selected plugin */
	LOGdL (DEBUG_IMPORT, "selecting forced plugin.");
	if (plugin_id >= IMPORT_PLUGIN_ELEMENTS)
	  plugin_id = -1;
      }
      /* found a valid plugin ? */
      if (plugin_id < 0)
      {
	LOG_Error ("no valid plugin found.\n");
	return -L4_ENOTFOUND;
      }
      /* set function pointers and init plugin */
      control->import_init = import_plugin_info[plugin_id].import_init;
      control->import_close = import_plugin_info[plugin_id].import_close;
      control->import_step = import_plugin_info[plugin_id].import_step;
      control->import_commit = import_plugin_info[plugin_id].import_commit;
      control->import_seek = import_plugin_info[plugin_id].import_seek;
      if (control->import_init (&control->plugin_ctrl, &control->streaminfo))
      {
	LOG_Error ("video import init failed!\n");
	return -L4_ENOTFOUND;
      }
      break;
    }				/* end case STREAM_TYPE_VIDEO: */

  case STREAM_TYPE_AUDIO:
    {
      /*  audio import disabled ? */
      if (control->plugin_ctrl.trackNo < 0)
      {
	LOG_Error ("demuxing audio is disabled (trackNo -1).\n");
	return -L4_ENOTFOUND;
      }

      /* of course we want to import */
      control->plugin_ctrl.mode = PLUG_MODE_IMPORT;

      /* autodetect or force ? */
      if (plugin_id < 0)
      {
	/* autodectect */
	LOGdL (DEBUG_IMPORT, "autodetecting plugin.");

	/*  Import ist AVI */
	if (probe_avi (control->plugin_ctrl.filename))
	{
	  plugin_id =
	    find_plugin_by_name (PLUG_NAME_AVI, PLUG_MODE_IMPORT,
				 STREAM_TYPE_AUDIO);
	}
	else
	  /* Import ist OGM */
	if (probe_ogm (control->plugin_ctrl.filename))
	{
	  plugin_id =
	    find_plugin_by_name (PLUG_NAME_OGM, PLUG_MODE_IMPORT,
				 STREAM_TYPE_AUDIO);
	}
	else
	  /* Import is MP3 */
	if (probe_mp3 (control->plugin_ctrl.filename))
	{
	  plugin_id =
	    find_plugin_by_name (PLUG_NAME_MP3, PLUG_MODE_IMPORT,
				 STREAM_TYPE_AUDIO);
	}
	else
	  return -L4_ENOTFOUND;
      }				/* end autodetect plugin */
      /* force selected plugin */
      else
      {
	/* force selected plugin */
	LOGdL (DEBUG_IMPORT, "selecting forced plugin.");
	if (plugin_id >= IMPORT_PLUGIN_ELEMENTS)
	  plugin_id = -1;
      }
      /* found a valid plugin ? */
      if (plugin_id < 0)
      {
	LOG_Error ("not a valid plugin!\n");
	return -L4_ENOTFOUND;
      }
      /* set function pointers and init plugin */
      control->import_init = import_plugin_info[plugin_id].import_init;
      control->import_close = import_plugin_info[plugin_id].import_close;
      control->import_step = import_plugin_info[plugin_id].import_step;
      control->import_commit = import_plugin_info[plugin_id].import_commit;
      control->import_seek = import_plugin_info[plugin_id].import_seek;
      if (control->import_init (&control->plugin_ctrl, &control->streaminfo))
      {
	LOG_Error ("audio import init failed!\n");
	return -L4_ENOTFOUND;
      }
      break;
    }				/* end case STREAM_TYPE_AUDIO: */
    /* invalid stream type */
  default:
    LOG_Error ("requested unsupported media type !\n");
    return -L4_ENOTFOUND;

  }				/* end switch */


  /*  we're here ? so everything went fine */
  return 0;
}				/* end determineAndSetup() */




/*****************************************************************************/
/**
 * \brief Probe video file content
 * 
 * \param filename File to probe
 * \param videoTrackNo Noof video stream within file to probe
 * \param audioTrackNo Noof audio stream within file to probe
 * \retval videoTracks Noof video streams the file contains
 * \retval audioTracks Noof audio streams the file contains
 * \retval streaminfo  information about probed streams
 *
 */
/*****************************************************************************/
l4_int32_t
containerProbeVideoFile (const char *filename,
			 l4_int32_t vid_trackNo,
			 l4_int32_t aud_trackNo,
			 l4_int32_t * vid_tracks,
			 l4_int32_t * aud_tracks, frame_ctrl_t * streaminfo)
{
#if VDEMUXER_BUILD_OGMLIB
  ogg_t *OGG;
#endif
#if VDEMUXER_BUILD_AVILIB
  avi_t *AVI;
#endif
  /* to get filesize */
  struct stat statbuf;
  int fdes;

  streaminfo->type = 0;		/* set to an invalid streamtype because this avoids misuse when working directly w/ plugins */

  /* setting defaults, will be overwritten if we get new infos from container */
  streaminfo->vi.fourCC = 0;
  streaminfo->vi.format = VID_FMT_NONE;
  streaminfo->ai.format = AUD_FMT_NONE;
  streaminfo->vi.length = streaminfo->ai.length = 0.00;
  streaminfo->seekable = 0;
  streaminfo->filesize = 0;
  strncpy (streaminfo->info, filename, 128);

  /* trying to get filesize via fstat */
  fdes = fileops_open (filename, 1);
  if (fdes > 0)
  {
    if (fileops_fstat (fdes, &statbuf) == 0)
      streaminfo->filesize = statbuf.st_size;
    fileops_close (fdes);
  }

  /* it is an AVI */
  if (probe_avi (filename))
  {
    LOGdL (DEBUG_IMPORT, "probed file seems an AVI");
#if VDEMUXER_BUILD_AVILIB
    AVI = AVI_open_input_file ((char *) filename, 1);
    if (!AVI)
    {
      return 0;
    }
    /* video */
    *vid_tracks = 1;		/* AVI always has only one video track afaik */
    streaminfo->vi.fourCC = vid_fourcc2int (AVI_video_compressor (AVI));
    streaminfo->vi.format = vid_fourcc2fmt (AVI_video_compressor (AVI));
    streaminfo->vi.xdim = AVI_video_width (AVI);
    streaminfo->vi.ydim = AVI_video_height (AVI);
    streaminfo->vi.framerate = AVI_frame_rate (AVI);
    if (*vid_tracks > 0)
      streaminfo->vi.length =
	(double) AVI_video_frames (AVI) * 1000.00 / streaminfo->vi.framerate;
    /* ?? not shure - but buffer size depends on */
    if (strlen (AVI_video_compressor (AVI)) == 4)
    {
      if (!strncasecmp (AVI_video_compressor (AVI), "YV12", 4))
	streaminfo->vi.colorspace = VID_YV12;
      else if (!strncasecmp (AVI_video_compressor (AVI), "UYVY", 4))
	streaminfo->vi.colorspace = VID_UYVY;
      else
	streaminfo->vi.colorspace = VID_RGB24;
    }
    else
      streaminfo->vi.colorspace = VID_RGB24;
    /* audio */
    AVI_set_audio_track (AVI, aud_trackNo);
    *aud_tracks = AVI_audio_tracks (AVI);
    streaminfo->ai.format = AVI_audio_format (AVI);
    streaminfo->ai.bits_per_sample = AVI_audio_bits (AVI);
    streaminfo->ai.channels = AVI_audio_channels (AVI);
    streaminfo->ai.samplerate = AVI_audio_rate (AVI);

    /* check for seek support */
    if (*vid_tracks > 0)
      streaminfo->seekable =
	import_plugin_supports_seek (PLUG_NAME_AVI, STREAM_TYPE_VIDEO);
    if (*aud_tracks > 0)
      streaminfo->seekable =
	import_plugin_supports_seek (PLUG_NAME_AVI, STREAM_TYPE_AUDIO);
    /* Close */
    AVI_close (AVI);
#else /* build w/ avilib */
    LOG_Error ("probed file seems an AVI. but compiled without AVI support.");
    return 0;
#endif /* end build w/ avilib */
  }


  /* it is an OGM */
  else if (probe_ogm (filename))
  {
    LOGdL (DEBUG_IMPORT, "probed file seems an OGM");
#if VDEMUXER_BUILD_OGMLIB
    /* open OGM */
#if VDEMUXER_BUILD_RTNETFS
    /* open without fetching information about stream (length, bitrate,...) if it might be on RTNETFS */
    if ((!strncasecmp ("rtns://", filename, 7))
	|| (!strncasecmp (VDEMUXER_DEFAULT_URL, "rtns", 4)))
      OGG = OGG_open_input_file ((char *) filename, 0);
    else
      OGG = OGG_open_input_file ((char *) filename, 1);
#else
    /* open with fetching information about stream (length, bitrate,...) */
    OGG = OGG_open_input_file ((char *) filename, 1);
#endif
    if (!OGG)
    {
      return 0;
    }
    /* video */
    *vid_tracks = OGG_video_tracks (OGG);
    OGG_set_video_track (OGG, vid_trackNo);
    streaminfo->vi.format = vid_fourcc2fmt (OGG_video_compressor (OGG));
    streaminfo->vi.fourCC = vid_fourcc2int (OGG_video_compressor (OGG));
    streaminfo->vi.xdim = OGG_video_width (OGG);
    streaminfo->vi.ydim = OGG_video_height (OGG);
    streaminfo->vi.framerate = OGG_frame_rate (OGG);
    streaminfo->vi.length = OGG_video_length (OGG);
    /* ? not shure - but buffer size depends on */
    if (strlen (OGG_video_compressor (OGG)) == 4)
    {
      if (!strncasecmp (OGG_video_compressor (OGG), "YV12", 4))
	streaminfo->vi.colorspace = VID_YV12;
      else if (!strncasecmp (OGG_video_compressor (OGG), "UYVY", 4))
	streaminfo->vi.colorspace = VID_UYVY;
      else
	streaminfo->vi.colorspace = VID_RGB24;
    }
    else
      streaminfo->vi.colorspace = VID_RGB24;
    /* audio */
    OGG_set_audio_track (OGG, aud_trackNo);
    *aud_tracks = OGG_audio_tracks (OGG);
    streaminfo->ai.format = OGG_audio_format (OGG);
    streaminfo->ai.bits_per_sample = OGG_audio_bits (OGG);
    streaminfo->ai.channels = OGG_audio_channels (OGG);
    streaminfo->ai.samplerate = OGG_audio_rate (OGG);
    streaminfo->ai.length = OGG_audio_length (OGG);
    /* check for seek support */
    if (*vid_tracks > 0)
      streaminfo->seekable =
	import_plugin_supports_seek (PLUG_NAME_OGM, STREAM_TYPE_VIDEO);
    if (*aud_tracks > 0)
      streaminfo->seekable =
	import_plugin_supports_seek (PLUG_NAME_OGM, STREAM_TYPE_AUDIO);
    /* close */
    OGG_close (OGG);
#else /* build w/ ogmlib */
    LOG_Error ("probed file seems an OGM. but compiled without OGM support.");
    return 0;
#endif /* end build w/ ogmlib */
  }


  /* it is an MPEG */
  else if (probe_mpeg (filename))
  {
    LOGdL (DEBUG_IMPORT, "probed file seems an MPEG(-1/-2)\n");
#if VDEMUXER_BUILD_MPEG
    /* Maybe it's mpeg or mpeg2 ?? */
    *vid_tracks = 1;		/* ?? */
    *aud_tracks = 0;		/* ?? */
    /* we're setting some defaults - the plugin will care about MPEG-1/-2 */
    streaminfo->vi.colorspace = VID_YV12;
    streaminfo->ai.format = AUD_FMT_MP2;
    streaminfo->vi.format = VID_FMT_MPEG1;
    streaminfo->vi.fourCC = vid_fourcc2int ("MPG1");

    /* 
     * we don't now correct values until it's decoded, so we set some big ones
     * to reserve enough size for video packets */
#warning FIXME: autodetect values
    streaminfo->vi.xdim = 1280;
    streaminfo->vi.ydim = 1024;
    streaminfo->vi.framerate = 25.00;

    /* check for seek support */
    if (*vid_tracks > 0)
      streaminfo->seekable =
	import_plugin_supports_seek (PLUG_NAME_MPEG, STREAM_TYPE_VIDEO);
    if (*aud_tracks > 0)
      streaminfo->seekable =
	import_plugin_supports_seek (PLUG_NAME_MPEG, STREAM_TYPE_AUDIO);
#else /* build w/ mpeg */
    LOG_Error
      ("probed file seems an MPEG. but compiled without MPEG support.");
    return 0;
#endif /* end build w/ mpeg */
  }

  /* it is an WAVE */
  else if (probe_wave (filename))
  {
    LOGdL (DEBUG_IMPORT,
	   "probed file seems an WAV. No import_plugin availiable.");
    *vid_tracks = 0;
    *aud_tracks = 1;
    streaminfo->ai.format = AUD_FMT_WAV;
    /* check for seek support */
    streaminfo->seekable =
      import_plugin_supports_seek (PLUG_NAME_RAW, STREAM_TYPE_AUDIO);
  }

  /* it is an MP3 */
  else if (probe_mp3 (filename))
  {
    LOGdL (DEBUG_IMPORT, "probed file seems an MP3.");

#if VDEMUXER_BUILD_MP3
    *vid_tracks = 0;
    *aud_tracks = 1;
    streaminfo->ai.format = AUD_FMT_MP3;
    streaminfo->ai.bits_per_sample = 16;
    if (get_mp3_info
	(filename, &streaminfo->ai.length, &streaminfo->ai.channels,
	 &streaminfo->ai.samplerate, &streaminfo->ai.bitrate))
    {
      /* get info failed, take defaults */
      streaminfo->ai.channels = 2;
      streaminfo->ai.samplerate = 44100;
      streaminfo->ai.length = 0.00;
    }

    if (get_mp3_taginfo (filename, streaminfo->info))
      /* failed */
      LOGdL (DEBUG_IMPORT, "probed MP3 has no ID3v1 tag");

    /* check for seek support */
    streaminfo->seekable =
      import_plugin_supports_seek (PLUG_NAME_MP3, STREAM_TYPE_AUDIO);
#else /* build w/ mp3 */
    LOG_Error ("probed file seems an MP3. but compiled without MP3 support.");
    return 0;
#endif /* end build w/ mp3 */
  }
  /* it is unknown container */
  else
    LOG_Error ("probed file has UNKNOWN type!\n");
  return 0;

}
