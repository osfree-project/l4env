/*
 * \brief   Global structures and routines for all VERNER components
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

#ifndef ARCH_GLOBALS_H
#define ARCH_GLOBALS_H

/* env */
#include <l4/log/l4log.h>	/* for printf */
#include <l4/util/macros.h>	/* logl , panic , error */

/* configuration */
#include "verner_config.h"

/* don't include math.h here */
double ceil(double x);


/* CONFIG ATTR*/
#define MAX_AUD_CHUNK_SIZE (int)(32768-sizeof(frame_ctrl_t))

/* TYPE DEFS */
#define STREAM_TYPE_INVALID 0
#define STREAM_TYPE_VIDEO 1
#define STREAM_TYPE_AUDIO 2

/** supported PLUGIN FUNCTIONS */
#define PLUG_MODE_NONE 0
#define PLUG_MODE_DEC 1
#define PLUG_MODE_ENC 2
#define PLUG_MODE_EXPORT 3
#define PLUG_MODE_IMPORT 4
#define PLUG_MODE_PASS 5

/* 
  plugin without f.e. seek-support 
  before calling function to seek via import plugin _YOU MUST_ check for PLUG_NO_SEEK
*/
#define PLUG_FUNCTION_NOT_SUPPORTED NULL


/** type of video frames */
#define VID_CODEC_SELFFRAME -1
#define VID_CODEC_IFRAME 1
#define VID_CODEC_PFRAME 2
#define VID_CODEC_BFRAME 3

/** well known video formats */
#define VID_FMT_NONE 0
#define VID_FMT_RAW 0x0001
#define VID_FMT_MPEG1 0x0002
#define VID_FMT_MPEG2 0x0004
#define VID_FMT_MPEG4 0x0008
#define VID_FMT_DIV3 0x0010
//if this is the fourmat you need to check frame_attr->fourCC for a valig codec/info
#define VID_FMT_FOURCC 0xCCCC

/** availiable colorspaces */
#define VID_RGB565 1
#define VID_RGB24 2
#define VID_RGB32 3
#define VID_UYVY 6
// YV12 = YVU <--> YUV420
#define VID_YV12 4
#define VID_YUV420 5

//maybe these three audio formats are reserved and so misinterpreted
/** well known audio formats */
#define AUD_FMT_NONE 0
#define AUD_FMT_MP2 0x0003
#define AUD_FMT_WAV 0x0004
//these are ok.
#define AUD_FMT_PCM 0x0001
#define AUD_FMT_MP3 0x0055
#define AUD_FMT_AC3 0x2000
#define AUD_FMT_VORBIS  0xffff

/** player modes (stop,play,...) */
#define PLAYMODE_STOP    (1<<0)
#define PLAYMODE_PLAY    (1<<1)
#define PLAYMODE_PAUSE   (1<<2)
#define PLAYMODE_FORWARD (1<<3)
#define PLAYMODE_DROP    ((1<<4)|PLAYMODE_PLAY)

/** recorder modes (stop,record) */
#define RECMODE_STOP 1
#define RECMODE_REC 2

/** seek modes */
#define SEEK_RELATIVE 1
#define SEEK_ABSOLUTE 2

/** en-/decode answers for vcore and code plugins */
#define CODING_ERROR 0
#define CODING_OK 1
#define CODING_NEED_MORE 2
// THIS SHOULD NOT USED WHILE ENCODING !!
#define CODING_HAS_STILL_DATA 4
//if the codec has still some data, we should not import but giving instead in_buffer = NULL
#define is_CODING_OK(a) (CODING_OK & a)
#define is_CODING_ERROR(a) (CODING_ERROR == a)
#define is_CODING_MORE(a) (CODING_NEED_MORE == a)
#define has_CODING_DATA(a) (CODING_HAS_STILL_DATA & a)

#include "arch_types.h"


/** when getting this frame type, it's expected to reset sync mechanism */
#define RESET_SYNC_POINT 2
/** when getting this some video/audio attrs may have changed (xdim,ydim,...), 
 * so for instance vsync has to adjust window size and reconfigure
*/
#define RECONFIGURE_POINT 4
#define is_reset_sync_point(a) (RESET_SYNC_POINT & a)
#define is_reconfigure_point(a) (RECONFIGURE_POINT & a)


/* Helper functions */

#include <string.h>	/* strlen */
#include <strings.h>	/* strcasecmp */

/** converts audio format identifier to an valid VERNER-format identifier
 *
 * \ingroup mod_globals
 *
 * \param  format   identifier of format
 *
 * \return VERNER-format identifier
 * \sa arch_globals.h
*/
int aud_codec2codec (int format);

/** converts video format identifier (fourCC) to an valid VERNER-format identifier
 *
 * \ingroup mod_globals
 *
 * \param  codec_fourcc   string description of the codec (fourCC)
 *
 * \return VERNER-format identifier
 * \sa arch_globals.h
*/
int vid_fourcc2fmt (char *codec_fourcc);

/** converts video format identifier (fourCC) to an integer
 *
 * \ingroup mod_globals
 *
 * \param  codec_fourcc   string description of the codec (fourCC)
 *
 * \return 0 if fails, else int representation of fourCC
*/
int vid_fourcc2int (const char *codec_fourcc);


/** calculates packetsize for on video frame for DSI submisson
 *
 * \ingroup mod_globals
 *
 * \param  streaminfo   streaminfo_info_t contains all necessary info about stream (xdim,ydim,colorspace)
 *
 * \return 0 if fails, else suggested packetsize for this stream
*/
int vid_streaminfo2packetsize (stream_info_t * stream_info);

/** calculates packetsize for on audio chunk for DSI submisson
 *
 * \ingroup mod_globals
 *
 * \param  streaminfo   streaminfo_info_t contains all necessary info about stream 
 *
 * \return 0 if fails, else suggested packetsize for this stream
 *
 * WARNING: Currently all submitted info are ignored! It just returns MAX_AUD_CHUNK_SIZE if ai.format is valid.
*/
int aud_streaminfo2packetsize (stream_info_t * stream_info);

#if BUILD_RT_SUPPORT
/** calculate how many loops per period the workers should do to process audio chunks or video frames
 *
 * \ingroup mod_globals
 *
 * \param  streaminfo   streaminfo_info_t contains all necessary info about stream 
 * \param  usec_period	period lenght in microsecs
 *
 * \return suggested loops per period. Panics if it fails!
 *
 * According to the streaminfo and the period lenght this function callculates
 * how many video frames or audio chunks (type of stream!) the workers have to 
 * process (demux, decode, play) in ONE period.
 */
int streaminfo2loops (stream_info_t * stream_info, unsigned long usec_period);
#endif

/* dump headers */
void dump_stream_info (const stream_info_t stream_info);

/* implementations follow */

/* taken from www.fourcc.org */
extern inline int
vid_fourcc2fmt (char *codec_fourcc)
{
  if (!strcasecmp (codec_fourcc, "DIV3") ||
      !strcasecmp (codec_fourcc, "DIV5") ||
      !strcasecmp (codec_fourcc, "MPG3") ||
      !strcasecmp (codec_fourcc, "MP43") ||
      !strcasecmp (codec_fourcc, "DIV4")
      || !strcasecmp (codec_fourcc, "AP41"))
  {
    return VID_FMT_DIV3;
  }
  else if ((!strcasecmp (codec_fourcc, "DIVX"))
	   || (!strcasecmp (codec_fourcc, "XVID"))
	   || (!strcasecmp (codec_fourcc, "MP4S"))
	   || (!strcasecmp (codec_fourcc, "M4S2"))
	   || (!strcasecmp (codec_fourcc, "MP4V"))
	   || (!strcasecmp (codec_fourcc, "UMP4"))
	   || (!strcasecmp (codec_fourcc, "DX50")))
  {
    return VID_FMT_MPEG4;
  }
  else if (!strcasecmp (codec_fourcc, "MPG1"))
  {
    return VID_FMT_MPEG1;
  }
  else if (!strcasecmp (codec_fourcc, "MPG2"))
  {
    return VID_FMT_MPEG2;
  }
  else if ((!strcasecmp (codec_fourcc, "YV12"))
	   || (!strcasecmp (codec_fourcc, "CC12"))
	   || (!strcasecmp (codec_fourcc, "UYUY"))
	   || (!strcasecmp (codec_fourcc, "RGBT"))
	   || (!strcasecmp (codec_fourcc, "Y41P"))
	   || (!strcasecmp (codec_fourcc, "YUY2")))
  {
    return VID_FMT_RAW;
  }
  else if (strlen (codec_fourcc) == 4)
    return VID_FMT_FOURCC;
  else
    return VID_FMT_NONE;	//???
}



extern inline int
aud_codec2codec (int format)
{
  switch (format)
  {
  case AUD_FMT_AC3:
    return AUD_FMT_AC3;
  case AUD_FMT_MP3:
  case 55:			//sometimes 55 not 0x55, not std !
    return AUD_FMT_MP3;
  case AUD_FMT_PCM:
    return AUD_FMT_PCM;
  case AUD_FMT_VORBIS:
    return AUD_FMT_VORBIS;
  default:
    return AUD_FMT_NONE;
  }
}

extern inline int
vid_fourcc2int (const char *codec_fourcc)
{
  if (strlen (codec_fourcc) != 4)
    return 0;
  return (int) ((codec_fourcc[0]) + ((codec_fourcc[1]) << 8) +
		((codec_fourcc[2]) << 16) + ((codec_fourcc[3]) << 24));
}


extern inline int
vid_streaminfo2packetsize (stream_info_t * stream_info)
{
  int packet_size;
  if (stream_info->vi.format == VID_FMT_NONE)
  {
    LOG_Error
      ("Unknown video format: couldn't determine packetsize for video");
    return 0;			//error
  }
  switch (stream_info->vi.colorspace)
  {
  case VID_RGB565:
    packet_size =
      stream_info->vi.xdim * stream_info->vi.ydim * 2 + sizeof (frame_ctrl_t);
    break;
  case VID_RGB24:
  case VID_UYVY:
    packet_size =
      stream_info->vi.xdim * stream_info->vi.ydim * 3 + sizeof (frame_ctrl_t);
    break;
  case VID_RGB32:
    packet_size =
      stream_info->vi.xdim * stream_info->vi.ydim * 4 + sizeof (frame_ctrl_t);
    break;
  case VID_YV12:
  case VID_YUV420:
    packet_size =
      (stream_info->vi.xdim * stream_info->vi.ydim * 3) / 2 +
      sizeof (frame_ctrl_t);
    break;
  default:
    LOG_Error ("couldn't determine packetsize for video");
    return 0;			//error
  }
  return packet_size;
}


extern inline int
aud_streaminfo2packetsize (stream_info_t * stream_info)
{
  if (stream_info->ai.format == AUD_FMT_NONE)
  {
    LOG_Error
      ("Unknown audio format: couldn't determine packetsize for audio");
    return 0;			//error
  }
  return MAX_AUD_CHUNK_SIZE;
}



#if BUILD_RT_SUPPORT
extern inline int
streaminfo2loops (stream_info_t * stream_info, unsigned long usec_period)
{
  /*
   * calculate how many frames/chunks per loop have to be processed
   * for given stream and period length
   *
   * Video: period / (1/framerate)
   * Audio (Mp3): period / (1152/samplerate)
   * 
   * 1152 is samples per packet!
   * other audio streams are expected to be mp3 
   * XXX - add more here!
   *
   * Note: convert frame-/samplerate from millisec to microsec (/1000)
   */
  double suggest_frames = 0.00;


  switch (stream_info->type)
  {

  case STREAM_TYPE_VIDEO:
    /* valid? */
    if (stream_info->vi.framerate == 0)
      break;

    suggest_frames =
      (double) (((double) usec_period * stream_info->vi.framerate) /
		(1 * 1000.00));
    suggest_frames /= 1000.00;
    break;


  case STREAM_TYPE_AUDIO:
    /* print wanrning if it's not mp3 */
    if (stream_info->ai.format != AUD_FMT_MP3)
      LOG_Error
	("No MP3 stream = chunks per loop can't be calculated (yet). Report please!\nAssuming MP3.");

    /* valid? */
    if (stream_info->ai.samplerate == 0)
      break;

    LOG("Streaminfo: bitrate: %i, samplerate: %ld, length: %f, bps: %i, channels: %i, format: %u",
	stream_info->ai.bitrate, stream_info->ai.samplerate, stream_info->ai.length, 
	stream_info->ai.bits_per_sample, stream_info->ai.channels, stream_info->ai.format);

    suggest_frames =
      (double) (((double) usec_period * stream_info->ai.samplerate) /
		(1152 * 1000.00));
    suggest_frames /= 1000.00;
    LOG("%i = (period = %luusec * sample = %ldHz) / (1152 * 1000)",
	(int) suggest_frames, usec_period, stream_info->ai.samplerate);
    break;
  }

  /* warn if calculation failed ! */
  if (suggest_frames == 0.00)
  {
    LOG_Error
      ("Couldn't calculate how many frames to process! Taking useless default value!");
    Panic ("Should we continue?");
    return 2;
  }

  LOG ("I suggest %i %s per loop!", (int) ceil (suggest_frames),
       stream_info->type == STREAM_TYPE_VIDEO ? "(V)frames" : "(A)chunks");
  return (int) ceil (suggest_frames);

}
#endif


extern inline void
dump_stream_info (const stream_info_t stream_info)
{

  LOG_printf ("Stream_info dump:\n   seekable=%i type = %ui\n",
	  stream_info.seekable, stream_info.type);
  LOG_printf
    ("   videoinfo:\n      fps=%i length=%i size=%ix%i colorspace=%ui\n      fourCC=%i format=%ui\n",
     (int) stream_info.vi.framerate, (int) stream_info.vi.length,
     stream_info.vi.xdim, stream_info.vi.ydim, stream_info.vi.colorspace,
     stream_info.vi.fourCC, stream_info.vi.format);
  LOG_printf
    ("   audioinfo:\n      samplerate=%i lenght=%i bps=%i\n ch=%i format=%ui bitrate=%i\n",
     (int) stream_info.ai.samplerate, (int) stream_info.ai.length,
     stream_info.ai.bits_per_sample, stream_info.ai.channels,
     stream_info.ai.format, stream_info.ai.bitrate);

}

#endif
