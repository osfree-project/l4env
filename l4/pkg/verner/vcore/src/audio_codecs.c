/*
 * \brief   Codecs setup for VERNER's core
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

/* local */
#include "codecs.h"

/* configuration */
#include "verner_config.h"

/* codecs */
#include "ac_pass.h"
#if BUILD_lame
#include "ac_lame.h"
#endif
#if BUILD_mpg123
#include "ac_mp3.h"
#endif

int
determineAndSetupCodec (control_struct_t * control)
{
#if PREDICT_DECODING_TIME
  predictor_t *(*predict_new)(const char *learn_file, const char *predict_file) = NULL;
  void (*predict_dispose)(predictor_t *predictor) = NULL;
#endif

  /* check if stream is valid */
  if (control->stream_type != STREAM_TYPE_AUDIO)
  {
    LOG_Error ("Only audio codecs are supported.");
    return -L4_ENOTSUPP;
  }

  /* just pass through ? */
  if ((control->streaminfo.ai.format ==
	  control->plugin_ctrl.target_format))
  {
    LOGdL (DEBUG_CODEC, "deceided to pass through.");
    control->codec_init = aud_codec_pass_init;
    control->codec_close = aud_codec_pass_close;
    control->codec_step = aud_codec_pass_step;
  }

  /* decode or encode */
  switch (control->plugin_ctrl.target_format)
  {
  case AUD_FMT_PCM:
    {
      /* MP3 to PCM decode */
#if BUILD_mpg123
      if ((control->plugin_ctrl.mode == PLUG_MODE_DEC)
	  && (control->streaminfo.ai.format == AUD_FMT_MP3))
      {
	LOGdL (DEBUG_CODEC, "deceided to decode MP3 to PCM");
	control->codec_init = aud_codec_mp3_init;
	control->codec_close = aud_codec_mp3_close;
	control->codec_step = aud_codec_mp3_step;
      }
      else
#endif
      {
	LOG_Error
	  ("Not valid codec found to get PCM-Output (mode=%i, format=%i).",
	   control->plugin_ctrl.mode, control->streaminfo.ai.format);
	return -L4_ENOTSUPP;
      }
      break;
    }
  case AUD_FMT_MP3:
    {
      /* PCM to MP3 encode */
#if BUILD_lame
      if ((control->plugin_ctrl.mode == PLUG_MODE_ENC)
	  && (control->streaminfo.ai.format == AUD_FMT_PCM))
      {
	LOGdL (DEBUG_CODEC, "deceided to encode PCM to MP3");
	control->codec_init = aud_codec_lame_init;
	control->codec_close = aud_codec_lame_close;
	control->codec_step = aud_codec_lame_step;
      }
      else
#endif      
      {
	LOG_Error ("Not valid codec found to get MP3-Output.");
	return -L4_ENOTSUPP;
      }
      break;
    }
  default:
    {
      LOG_Error ("Only PCM and MP3 are currently supported (if configured).");
      return -L4_ENOTSUPP;
    }
  }				/* end switch */

#if PREDICT_DECODING_TIME
  if (control->predict->predictor) {
    if (predict_new != control->predict->predict_new) {
      /* the predictor format is changing, so delete the old one */
      control->predict->predict_dispose(control->predict->predictor);
      control->predict->predictor = NULL;
    } else {
      control->predict->predict_discontinue(control->predict->predictor);
    }
  }
  if (!control->predict->predictor && predict_new) {
    /* initialize the predictor */
    control->predict->predictor = predict_new(control->predict->learn_file, control->predict->predict_file);
    if (!control->predict->predictor) LOG_Error("predictor could not be initialized");
    control->predict->predict_new     = predict_new;
    control->predict->predict_dispose = predict_dispose;
  }
  control->predict->decoding_time       = 0;
  control->predict->prediction_overhead = 0;
#endif

  /* now initialize the codec */
  if (control->codec_init (&control->plugin_ctrl, &control->streaminfo) != 0)
  {
    LOG_Error ("audio codec init failed.");
    return -L4_EUNKNOWN;
  }
  else
    LOGdL (DEBUG_CODEC, "audio codec \"%s\" registered.",
	   control->plugin_ctrl.info);

  /* done */
  return 0;
}
