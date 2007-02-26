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
#include "vc_pass.h"
#if BUILD_libavcodec
#include "vc_ffmpeg.h"
#endif
#if BUILD_xvid10
#include "vc_xvid_10.h"
#endif
#if BUILD_xvid09
#include "vc_xvid.h"
#endif
#if BUILD_libmpeg2
#include "vc_mpeg.h"
#endif

int
determineAndSetupCodec (control_struct_t * control)
{
#if PREDICT_DECODING_TIME
  predictor_t *(*predict_new)(const char *learn_file, const char *predict_file) = NULL;
  void (*predict_dispose)(predictor_t *predictor) = NULL;
#endif

  /* check if stream is valid */
  if (control->stream_type != STREAM_TYPE_VIDEO)
  {
    LOG_Error ("Only video codecs are supported.");
    return -L4_ENOTSUPP;
  }


  /* just pass through ? */
  if ((control->streaminfo.vi.format ==
	  control->plugin_ctrl.target_format))
  {
    LOGdL (DEBUG_CODEC, "deceided to pass through.");
    control->codec_init = vid_codec_pass_init;
    control->codec_close = vid_codec_pass_close;
    control->codec_step = vid_codec_pass_step;
  }

  /* decode or encode */
  switch (control->plugin_ctrl.target_format)
  {
  /* decode/copy to RAW */
  case VID_FMT_RAW:
    {
      /* MPEG-4 to RAW decode */
      if (0){;}
      else      
#if (BUILD_xvid10) || (BUILD_xvid09)
      if ((control->plugin_ctrl.mode == PLUG_MODE_DEC)
	  && (control->streaminfo.vi.format == VID_FMT_MPEG4))
      {
	LOGdL (DEBUG_CODEC, "deceided to decode MPEG-4 to RAW");
	/* XviD */
#if BUILD_xvid10
	control->codec_init = vid_codec_xvid_10_init;
	control->codec_close = vid_codec_xvid_10_close;
	control->codec_step = vid_codec_xvid_10_step;
#if PREDICT_DECODING_TIME
	predict_new                           = predict_xvid_new;
	control->predict->predict             = predict_xvid;
	control->predict->predict_learn       = predict_xvid_learn;
	control->predict->predict_eval        = predict_xvid_eval;
	control->predict->predict_discontinue = predict_xvid_discontinue;
	predict_dispose                       = predict_xvid_dispose;
#endif
#elif BUILD_xvid09
	control->codec_init = vid_codec_xvid_init;
	control->codec_close = vid_codec_xvid_close;
	control->codec_step = vid_codec_xvid_step;
#endif
      }
      else 
#endif
#if BUILD_libmpeg2
      if ((control->plugin_ctrl.mode == PLUG_MODE_DEC)
	       && ((control->streaminfo.vi.format == VID_FMT_MPEG2)
		   || (control->streaminfo.vi.format == VID_FMT_MPEG1)))
      {
	LOGdL (DEBUG_CODEC, "deceided to decode MPEG-1/-2 to RAW");
	/* libmpeg2 */
	control->codec_init = vid_codec_mpeg_init;
	control->codec_close = vid_codec_mpeg_close;
	control->codec_step = vid_codec_mpeg_step;
#if PREDICT_DECODING_TIME
	predict_new                           = predict_mpeg_new;
	control->predict->predict             = predict_mpeg;
	control->predict->predict_learn       = predict_mpeg_learn;
	control->predict->predict_eval        = predict_mpeg_eval;
	control->predict->predict_discontinue = predict_mpeg_discontinue;
	predict_dispose                       = predict_mpeg_dispose;
#endif
      }
      else
#endif
#if BUILD_libavcodec
      {
	/* FFmpeg */
	LOGdL (DEBUG_CODEC, "deceided to try libavcodec (read allrounder)");
	control->codec_init = vid_codec_ffmpeg_init;
	control->codec_close = vid_codec_ffmpeg_close;
	control->codec_step = vid_codec_ffmpeg_step;
      }
#else
      {
        LOG_Error ("no valid codec found to convert to RAW.");
        return -L4_ENOTSUPP;
      }
#endif
      break;
    }

  /* encode/copy to MPEG-4 */
  case VID_FMT_MPEG4:
    {
      /* MPEG-4 to RAW decode */
      if (0){;}
      else      
#if BUILD_xvid10
      if ((control->plugin_ctrl.mode == PLUG_MODE_ENC)
	  && (control->streaminfo.vi.format == VID_FMT_RAW))
      {
	LOGdL (DEBUG_CODEC, "deceided to encode MPEG-4 to RAW");
	/* XviD */
	control->codec_init = vid_codec_xvid_10_init;
	control->codec_close = vid_codec_xvid_10_close;
	control->codec_step = vid_codec_xvid_10_step;
      }
      else
#endif
      {
        LOG_Error ("no valid codec found to convert to MPEG-4.");
        return -L4_ENOTSUPP;
      }
      break;
    }
    
  default:
    {
      LOG_Error ("Currently only convert to RAW or MPEG-4 possible.");
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
    LOG_Error ("video codec init failed.");
    return -L4_EUNKNOWN;
  }
  else
    LOGdL (DEBUG_CODEC, "video codec \"%s\" registered.",
	   control->plugin_ctrl.info);

  /*
   * check if codec doesn't support it's own QLevels
   * if not, we add +3 (may be used for default filters)
   * (decode only)
   */
#if VCORE_VIDEO_ENABLE_QAP
  if ((!control->plugin_ctrl.supportsQAP)
      && (control->plugin_ctrl.mode == PLUG_MODE_DEC))
  {
    control->plugin_ctrl.maxQLevel = 3;
    control->plugin_ctrl.minQLevel = 0;
  }
#if VCORE_VIDEO_FORCE_LIBPOSTPROC
  if (control->plugin_ctrl.mode == PLUG_MODE_DEC)
  {
    LOGdL (DEBUG_CODEC, "forced to take libpostproc!");
    control->plugin_ctrl.supportsQAP = 0; /* declare codec as no QAP-supporter */
    control->plugin_ctrl.maxQLevel = 3;
    control->plugin_ctrl.minQLevel = 0;
  }
#endif
#endif

  /* done */
  return 0;
}
