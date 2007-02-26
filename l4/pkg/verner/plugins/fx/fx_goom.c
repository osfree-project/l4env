/*
 * \brief   Effects using GOOM for VERNER's syns
 * \date    2004-05-19
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


/* l4 */
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* OSKit includes */
#include <stdlib.h>		/* malloc, free */

/* local */
#include "arch_globals.h"
#include "fx_goom.h"

/* configuration */
#include "verner_config.h"

/* goom */
#include <goom.h>

/* internal state */
typedef struct
{
  /* initialized */
  int initialized;
  /* store info tag */
#define TAG_LEN 128
  char infotext[TAG_LEN];
  /* chunk id (noof pcm samples passed filter */
  long chunks;
} goom_t;
static goom_t goom = { 0, };


/* init plugin */
int
fx_goom_init (plugin_ctrl_t * attr, stream_info_t * info, char *options)
{
  /* no options here */

  /* allow only one instance! */
  if (goom.initialized)
    return 0;

  /* verbose info */
  strncpy (attr->info, "goom fx", 32);

  /* check valid stream type */
  if (info->type != STREAM_TYPE_AUDIO)
  {
    LOG_Error ("only audio streams supported.");
    return -L4_ENOTSUPP;
  }

  /* init goom */
  jeko_init ();

  /* init private stuff */
  goom.initialized = 1;
  goom.infotext[0] = '\0';
  goom.chunks = 0;

  /* done */
  return 0;
}

/* send pcm data to goom engine */
int
fx_goom_step (plugin_ctrl_t * attr, unsigned char *buffer)
{
  frame_ctrl_t *frameattr = (frame_ctrl_t *) buffer;

  /* inited? */
  if (!goom.initialized)
    return 0;

  /* check for valid handle and mode */
  if (attr->mode != PLUG_MODE_EXPORT)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return CODING_ERROR;
  }

  /* set text of song title changed */
  if (strlen (attr->info))
  {
    /* display */
    jeko_set_text (attr->info);
    /* ack info */
    attr->info[0] = '\0';
    /* force redisplay frame info */
    goom.infotext[0] = '\0';
    goom.chunks = 0;
  }
  else
    /* check if the demuxer or core send an new string and more than 100 chunks are passed */
  {
    if ((strlen (frameattr->info))
	&& (strncmp (goom.infotext, frameattr->info, TAG_LEN))
	&& (goom.chunks > 100))
    {
      strncpy (goom.infotext, frameattr->info, TAG_LEN);
      jeko_set_text (goom.infotext);
    }
  }

  /* send pcm data to goom to adapt effects */
  if (frameattr->framesize > 1024)
  {
    jeko_render_pcm ((signed short *) (buffer + sizeof (frame_ctrl_t)));	/*signed short data[2][512] */
    goom.chunks++;
  }

  /* done */
  return 0;
}

/* close plugin */
int
fx_goom_close (plugin_ctrl_t * attr)
{

  /* inited? */
  if (!goom.initialized)
    return 0;
  goom.initialized = 0;

  /* close filter */
  LOGdL (DEBUG_EXPORT, "closing fx goom");
  jeko_cleanup ();
  return 0;
}
