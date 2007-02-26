/*
 * \brief   Plugin configuration for VERNER
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

#ifndef PLUGINS_ARCH_H
#define PLUGINS_ARCH_H

/* verner */
#include "arch_globals.h"

/* verner configuration */
#include "verner_config.h"

#ifdef USE_VDEMUXER

/* import plugins includes */
#include "demux_ogm.h"
#include "demux_avi.h"
#include "demux_mpg.h"
#include "demux_mp3.h"

#endif /* end vdemuxer */

/* fx plugins' id */
#define FX_PLUG_ID_NONE		0
#define FX_PLUG_ID_GOOM		1


/** import plugin description type */
typedef struct
{
  /** see arch_globals.h - plugin_types */
  int mode;
  /** streamtype */
  int type;
  /** info string */
  char *name;

  /** import functions pointers */
  int (*import_init) (plugin_ctrl_t * attr, stream_info_t * info);
  int (*import_commit) (plugin_ctrl_t * attr);
  int (*import_step) (plugin_ctrl_t * attr, void *addr);
  int (*import_close) (plugin_ctrl_t * attr);
  int (*import_seek) (plugin_ctrl_t * attr, void *addr, double position,
		      int whence);
} import_plugin_info_t;


/** codec plugin description type */
typedef struct
{
  /** see arch_globals.h - plugin_types */
  int mode;
  /** streamtype */
  int type;
  /** info string */
  char *name;

  /** codec functions pointers */
  int (*codec_init) (plugin_ctrl_t * attr, stream_info_t * info);
  int (*codec_step) (plugin_ctrl_t * attr, unsigned char *in_buffer,
		     unsigned char *out_buffer);
  int (*codec_close) (plugin_ctrl_t * attr);

} codec_plugin_info_t;


/** availiable import plugins:
 * the define statements make sure not to compile all plugins into component
 * BUT: the id's are not the same in each component - so address them by NAME only!
 * import plugin names starting with ...
 *   FILE_: using Unix-Std-calls (read, write, open, close) directly or with a (avi-/ogm-)lib
 *   Autodetect: detect plugin by src probing
*/
#define PLUG_NAME_AUTODETECT	"Autodetect"
#define PLUG_NAME_AVI 		"File-AVI"
#define PLUG_NAME_MPEG 		"File-MPEG(1/2)"
#define PLUG_NAME_MP3 		"File-MP3"
#define PLUG_NAME_OGM 		"File-OGM"
#define PLUG_NAME_RAW 		"File-RAW"

#ifndef __import_plugin_info_t
#define __import_plugin_info_t

/** information about ALL availiable import plugins with function-pointer
*/
static import_plugin_info_t import_plugin_info[] = {
#ifdef USE_VDEMUXER
  /* Video import plugins */
#if VDEMUXER_BUILD_OGMLIB
  {
   PLUG_MODE_IMPORT, STREAM_TYPE_VIDEO, PLUG_NAME_OGM,
   /* import functions */
   vid_import_ogm_init, vid_import_ogm_commit,
   vid_import_ogm_step, vid_import_ogm_close,
   vid_import_ogm_seek,
   },
#endif
#if VDEMUXER_BUILD_AVILIB
  {
   PLUG_MODE_IMPORT, STREAM_TYPE_VIDEO, PLUG_NAME_AVI,
   /* import functions */
   vid_import_avi_init, vid_import_avi_commit,
   vid_import_avi_step, vid_import_avi_close,
   vid_import_avi_seek},
#endif
#if VDEMUXER_BUILD_MPEG
  {
   PLUG_MODE_IMPORT, STREAM_TYPE_VIDEO, PLUG_NAME_MPEG,
   /* import functions */
   vid_import_mpg_init, vid_import_mpg_commit,
   vid_import_mpg_step, vid_import_mpg_close,
   vid_import_mpg_seek},
  /* Audio import plugins */
#endif
#if VDEMUXER_BUILD_OGMLIB
  {
   PLUG_MODE_IMPORT, STREAM_TYPE_AUDIO, PLUG_NAME_OGM,
   /* import functions */
   aud_import_ogm_init, aud_import_ogm_commit,
   aud_import_ogm_step, aud_import_ogm_close,
   aud_import_ogm_seek},
#endif
#if VDEMUXER_BUILD_MP3
  {
   PLUG_MODE_IMPORT, STREAM_TYPE_AUDIO, PLUG_NAME_MP3,
   /* import functions */
   aud_import_mp3_init, aud_import_mp3_commit,
   aud_import_mp3_step, aud_import_mp3_close,
   aud_import_mp3_seek},
#endif
#if VDEMUXER_BUILD_AVILIB
  {
   PLUG_MODE_IMPORT, STREAM_TYPE_AUDIO, PLUG_NAME_AVI,
   /* import functions */
   aud_import_avi_init, aud_import_avi_commit,
   aud_import_avi_step, aud_import_avi_close,
   aud_import_avi_seek},
#endif // BUILD_AVILIB
#endif // USE_VDEMUXER
  /* end symbol */
  {
   PLUG_MODE_NONE, 0, "none", 0, 0, 0, 0},
};
#endif //#define __import_plugin_info_t


/* noof availiable import plugins */
#define IMPORT_PLUGIN_ELEMENTS ((sizeof(import_plugin_info)/sizeof(import_plugin_info[0]))-1)


/** Searching a plugin
 *
 * \ingroup mod_globals
 *
 * \param  name   plugin string description 
 * \param  mode   import,export,decode, encode, ... (see arch_globals.h)
 * \param  type   streamtype f.e. Audio, Video, Text, ... (see arch_globals.h)
 *
 * \return ID for verner_plugin_info[id]
*/
int find_plugin_by_name (const char *name, unsigned int mode,
			 unsigned int type);
/** Check whether a import plugin supports seeking or not
 *
 * \ingroup mod_globals
 *
 * \param  name   plugin string description 
 * \param  type   streamtype f.e. Audio, Video, Text, ... (see arch_globals.h)
 *
 * \return 1 for seek supported, else no seek support
*/
int import_plugin_supports_seek (const char *name, unsigned int type);
/* implementation follows */
extern inline int
find_plugin_by_name (const char *name, unsigned int mode, unsigned int type)
{
  int id = 0;
  /* search import plugin */
  if (mode == PLUG_MODE_IMPORT)
  {
    while (import_plugin_info[id].mode != PLUG_MODE_NONE)
    {
      if ((import_plugin_info[id].mode == mode) &&
	  (import_plugin_info[id].type == type) &&
	  (!strcasecmp (import_plugin_info[id].name, name)))
      {
	return id;
      }
      id++;
    }
  }
  LOG_Error ("Did not found plugin %s!", name);
  /* found nothing */
  return -1;
}

extern inline int
import_plugin_supports_seek (const char *name, unsigned int type)
{
  int plugin_id = find_plugin_by_name (name, PLUG_MODE_IMPORT, type);
  /* supports seeking */
  if ((plugin_id != -1)
      && (import_plugin_info[plugin_id].import_seek !=
	  PLUG_FUNCTION_NOT_SUPPORTED))
    return 1;
  /* no found or no seek support */
  return 0;
}


#endif
