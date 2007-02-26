/*
 * \brief   Types for all VERNER components
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

#ifndef ARCH_TYPES_H
#define ARCH_TYPES_H

/** type for Communication between Verner's components & plugins */
typedef struct
{
  // *.*
  /** capablities of CPU (0=autodetect) (format taken from XviD 1.0 */
  int cpucaps;

  /** description of plugin */
  char info[32];
  /** plugins internal handle */
  void *handle;
  /** streamtype: Audio, Video, Text, Raw ... */
  unsigned int type;
  /** pluginmode: Import, Export, Encode, Decode */
  unsigned int mode;

  //these are the target format description, which should be delivered.
  //mainly used in vcore_vid. 
  //video:
  //-vsync will set them to DOpE-accepted formats
  //-vmuxer will set it to user selection or VID_FMT_MPEG4 (Xvid) and VID_YV12
  //audio:
  //-vsync will set them to PCM
  //-vmuxer will set it to user selection or AUD_FMT_MP3

  /** format wanted by Sync- or Muxer-Component */
  unsigned int target_format;
  /** colorspace of pictures (video only) */
  unsigned int target_colorspace;
  /** value that was consumed in step in microsecs
   * (useful to synchronize with input/output)
   */
  unsigned long step_time_us;

  //Import, Export
  /** filename (for import and export plugins */
  char *filename;
  /** track number within file or stream */
  int trackNo;

  //makes it much easier to work with DSI:
  //stores framesize+sizeof(frame_ctrl_t) mostly within *_step
  //(no need to cast and lookup in frame_ctrl_type->framesize)
  /** size of one DSI-packet  */
  long packetsize;

  //Core
  /** Bitrate-Level (kbps) (both cores) */
  int bitrate;
  /** the codec has it's internal QLevels */
  int supportsQAP;
  /** current Quality-Level  */
  int currentQLevel;
  /** min. Quality-Level the codec supports */
  int minQLevel;
  /** max. Quality-Level the codec supports */
  int maxQLevel;

  /** noof packets fit into buffer (mostly dsi_buffer_size) */
  int bufferSize;
  /** elements currently in buffer */
  int bufferElements;

} plugin_ctrl_t;


/** type which holds all necessary about video(frames) */
struct videoinfo
{
  double framerate;
  double length;		//stream length in millisec
  unsigned int xdim;
  unsigned int ydim;
  unsigned int colorspace;
  unsigned int format;		//well known video format
  //Some Codecs can not determine only by format,
  //like many of ffmpeg's. They can only determine by FOURCC.
  //Note: not all FourCCs are listed as valid formats.
  //fourcc (LSB first, so "ABCD" -> ('D'<<24) + ('C'<<16) + ('B'<<8) + 'A')
  int fourCC;
};

/** type which holds all necessary about audio(chunks) */
struct audioinfo
{
  /* bitrate (kbps)only from NON-RAW-Formats, for instance often used by MP3 */
  int bitrate;

  long samplerate;
  double length;		//stream length in millisec
  int bits_per_sample;
  int channels;
  unsigned int format;		//well known audio format
};

/** type which holds all information for ONE frame or ONE audio chunk (while submitted or proccessed) */
typedef struct
{
  //Muxer, Sync
  //double start_pts;           //Gueltigkeit des Frames in MILLIseconds  (start/end)
  //double end_pts;
  double last_sync_pts;		//this is the offset from last SYNC_RESET_POINT

  //Demuxer, Controller, Sync
  int seekable;			//stream supports seeking ?

  //Demuxer, Core, Sync
  double pts;			//presentation time stamp in MILLIseconds 

  //VI / AI
  struct audioinfo ai;
  struct videoinfo vi;

  //mostly all
  unsigned int type;		//audio, video, text ...
  unsigned int keyframe;
  //0 = no keyframe ;  1 = keyframe ; 2 = keyframe und sync-reset !! 4 = reset-attr-point
  long frameID;
  long framesize;

  //info about stream (comment or ID3tag,...) for vcontrol and vsync
  char info[128];

  //filesize in bytes
  long filesize;

} frame_ctrl_t;

//currently all information about the stream could be saved within frame_ctrl_t
/** type which holds all about one stream */
#define stream_info_t frame_ctrl_t

#endif
