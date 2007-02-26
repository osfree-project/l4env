/*
 * \brief   Xvid codec plugin for VERNER's core
 * \date    2004-02-11
 * \author  Carsten Rietzschel <cr7@os.inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2004  Carsten Rietzschel  <cr7@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*
 * Parts used of GPL'ed XviD-examples
 */

/* L4 includes */
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* OSKit includes */
#include <stdlib.h>		/* malloc, free */

/* verner */
#include "arch_globals.h"
#include "debug_cfg.h"

/* OSKit includes */
#include <stdio.h>
#include <stdlib.h>
#include <l4/env/errno.h>

/* xvid stuff */
#include <xvid.h>		/* comes with XviD */

/* local */
#include "vc_xvid.h"

#undef XVID_CSP_EXTERN
/* ??? XVID_CSP_EXTERN should be faster, but it is NOT, isn't it. ??? */

/*
 * quality levels for XVID 0.9.x
 * Note: the first two Quality Settings are useless, because on my test machine Q2 is faster and has an better PSNR
 */
const static int motion_presets[5] = {
  /*0,                           */
  /*PMV_EARLYSTOP16,             */
  PMV_EARLYSTOP16,		// Q0
  PMV_EARLYSTOP16 | PMV_HALFPELREFINE16,	// Q1
  PMV_EARLYSTOP16 | PMV_HALFPELREFINE16,	// Q2
  PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 | PMV_EARLYSTOP8	// Q3
    | PMV_HALFPELREFINE8,
  PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 | PMV_EXTSEARCH16	// Q4
  | PMV_USESQUARES16 | PMV_EARLYSTOP8 | PMV_HALFPELREFINE8
};

const static int general_presets[5] = {
  /*XVID_H263QUANT,             */
  /*XVID_MPEGQUANT,             */
  XVID_H263QUANT,		// Q0
  XVID_H263QUANT | XVID_HALFPEL,	// Q1
  XVID_H263QUANT | XVID_HALFPEL | XVID_INTER4V,	// Q2
  XVID_H263QUANT | XVID_HALFPEL | XVID_INTER4V,	// Q3
  XVID_H263QUANT | XVID_HALFPEL | XVID_INTER4V
};				// Q4


/* internal data */
struct internal_data
{
  void *handle;
  XVID_DEC_FRAME dec_xframe;
  XVID_ENC_FRAME enc_xframe;
#ifdef XVID_CSP_EXTERN
  XVID_DEC_PICTURE pic;
#endif
  unsigned int xdim, ydim;

  /* these values 'll be precalcalculated for faster decoding step routine */
#ifdef XVID_CSP_EXTERN
  int offset_u, offset_v;
#endif
  int packetsize, framesize;
};




/*
 * internal helpers 
 */
static int xvid_init_encoder (int *enc_handle, int bitrate, int framerate,
			      int xdim, int ydim);
static int xvid_init_decoder (int *dec_handle, int xdim, int ydim);



/*
 * initialize codec 
 */
int
vid_codec_xvid_init (plugin_ctrl_t * attr, stream_info_t * info)
{
  int status;
  struct internal_data *data;

  /* verbose info */
  strncpy (attr->info, "XviD Codec 0.9.x", 32);

  /* check for valid input data */
  if ((attr->mode != PLUG_MODE_DEC) && (attr->mode != PLUG_MODE_ENC))
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }
  if (info->type != STREAM_TYPE_VIDEO)
  {
    LOG_Error ("MEDIA TYPE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }

  /* build internal data structs and alloc mem */
  attr->handle =
    (struct internal_data *) malloc (sizeof (struct internal_data));
  if (!attr->handle)
  {
    LOG_Error ("malloc failed!");
    return -L4_ENOMEM;
  }
  memset (attr->handle, 0, sizeof (struct internal_data));
  data = (struct internal_data *) attr->handle;

  /* set some default values if not done yet */
  if (info->vi.framerate == 0)
    info->vi.framerate = 25.00;
  if (attr->bitrate == 0)
    attr->bitrate = 600;

  /* remember values for step routines */
  data->xdim = info->vi.xdim;
  data->ydim = info->vi.ydim;
  data->dec_xframe.stride = info->vi.ydim;

  /* determine colorspace */
  switch (attr->target_colorspace)
  {
  case VID_YUV420:
    data->dec_xframe.colorspace = data->enc_xframe.colorspace = XVID_CSP_I420;
#ifdef XVID_CSP_EXTERN
    if (attr->mode == PLUG_MODE_DEC)
    {
      LOGdL (DEBUG_CODEC,
	     "has XVID_CSP_EXTERN. This should be fast for decoding :)");
      data->dec_xframe.colorspace = XVID_CSP_EXTERN;
      data->pic.stride_y = data->xdim;
      data->pic.stride_u = data->pic.stride_v = data->xdim / 2;
      data->dec_xframe.stride = 0;	/* stride is 0, because we use an external buffer */
      data->dec_xframe.image = &data->pic;
    }
#endif
    break;
  case VID_YV12:
    data->dec_xframe.colorspace = data->enc_xframe.colorspace = XVID_CSP_YV12;
#ifdef XVID_CSP_EXTERN
    if (attr->mode == PLUG_MODE_DEC)
    {
      LOGdL (DEBUG_CODEC,
	     "has XVID_CSP_EXTERN. This should be fast for decoding :)");
      data->dec_xframe.colorspace = data->enc_xframe.colorspace =
	XVID_CSP_EXTERN;
      data->pic.stride_y = data->xdim;
      data->pic.stride_u = data->pic.stride_v = data->xdim / 2;
      data->dec_xframe.stride = 0;	/* stride is 0, because we use an external buffer */
      data->dec_xframe.image = &data->pic;
    }
#endif
    break;
  case VID_RGB565:
    data->dec_xframe.colorspace = data->enc_xframe.colorspace =
      XVID_CSP_RGB565;
    break;
  case VID_RGB24:
    data->dec_xframe.colorspace = data->enc_xframe.colorspace =
      XVID_CSP_RGB24;
    break;
  case VID_RGB32:
    data->dec_xframe.colorspace = data->enc_xframe.colorspace =
      XVID_CSP_RGB32;
    break;
  case VID_UYVY:
    data->dec_xframe.colorspace = data->enc_xframe.colorspace = XVID_CSP_UYVY;
    break;
  default:
    LOG_Error ("unsupported colorspace.");
    attr->handle = NULL;
    free (data);
    return -L4_ENOTSUPP;
  }


  /* working as decoder */
  if (attr->mode == PLUG_MODE_DEC)
  {
    /* precalculate some values */
#ifdef XVID_CSP_EXTERN
    /* for YUV420 */
    if ((attr->target_colorspace == VID_YUV420)
	&& (data->dec_xframe.colorspace = XVID_CSP_EXTERN))
    {
      data->offset_u = (data->xdim * data->ydim);
      data->offset_v =
	(data->xdim * data->ydim) + (data->xdim * data->ydim / 4);
    }
    /* it's VID_YV12 */
    else if ((attr->target_colorspace == VID_YUV420)
	     && (data->dec_xframe.colorspace = XVID_CSP_EXTERN))
    {
      data->offset_v = (data->xdim * data->ydim);
      data->offset_u =
	(data->xdim * data->ydim) + (data->xdim * data->ydim / 4);
    }
#endif
    /* precalculate packet size */
    info->vi.colorspace = attr->target_colorspace;
    data->packetsize = vid_streaminfo2packetsize (info);
    data->framesize = data->packetsize - sizeof (frame_ctrl_t);

    /* init xvid core */
    status =
      xvid_init_decoder ((int *) &data->handle, info->vi.xdim, info->vi.ydim);
    if (status)
    {
      LOG_Error ("Decore INIT problem, return value %d\n", status);
      free (data);
      attr->handle = NULL;
      return -L4_EUNKNOWN;
    }

    /* after decoding we get RAW */
    info->vi.format = VID_FMT_RAW;

    /* QAP supported */
    attr->supportsQAP = 0;
    /* set supported qualities */
    attr->minQuality = attr->maxQuality = 0;
  }				/* end working as decoder */

  /* working as encoder */
  if (attr->mode == PLUG_MODE_ENC)
  {
    LOGdL (DEBUG_CODEC, "encoding w/ bitrate=%i fps=%i xdim=%i ydim=%i",
	   (int) attr->bitrate, (int) info->vi.framerate,
	   (int) info->vi.xdim, (int) info->vi.ydim);

    /* init encoder core */
    status =
      xvid_init_encoder ((int *) &data->handle, attr->bitrate,
			 info->vi.framerate, info->vi.xdim, info->vi.ydim);
    if (status)
    {
      LOG_Error ("Encore INIT problem, return value %d", status);
      free (data);
      attr->handle = NULL;
      return -L4_EUNKNOWN;
    }
    /* after encoding we get MPEG4 */
    info->vi.format = VID_FMT_MPEG4;
    /* set supported qualities */
    attr->minQuality = 0;
    attr->maxQuality = 4;
    /* QAP supported */
    attr->supportsQAP = 1;
  }

  /* done */
  return 0;
}



/*
 * process with next frame 
 */
inline int
vid_codec_xvid_step (plugin_ctrl_t * attr, unsigned char *in_buffer,
		     unsigned char *out_buffer)
{
  /* statistics for encoding */
  XVID_ENC_STATS xstats;

  int status;
  frame_ctrl_t *frameattr;
  struct internal_data *data = (struct internal_data *) attr->handle;

  /* check for valid input */
  if (!attr->handle)
  {
    LOG_Error ("HANDLE FAILURE");
    return CODING_ERROR;
  }
  if ((!out_buffer) || (!in_buffer))
  {
    LOGdL (DEBUG_CODEC,
	   "no input or no output buffer. XviD needs both at once.");
    return CODING_ERROR;
  }

  /* get information from frame_ctrl_t */
  memcpy (out_buffer, in_buffer, sizeof (frame_ctrl_t));
  frameattr = (frame_ctrl_t *) out_buffer;

  switch (attr->mode)
  {
  case PLUG_MODE_DEC:
    /* working as decoder */
    {
      /* set output format and framesize */
      frameattr->vi.format = attr->target_format;
      frameattr->vi.colorspace = attr->target_colorspace;

      /* set src buffers */
      data->dec_xframe.bitstream = in_buffer + sizeof (frame_ctrl_t);
      data->dec_xframe.length = frameattr->framesize;
      data->dec_xframe.general = 0;

      /* set destination buffer */
#ifdef XVID_CSP_EXTERN
      if (data->dec_xframe.colorspace == XVID_CSP_EXTERN)
      {
	data->pic.y = out_buffer + sizeof (frame_ctrl_t);
	data->pic.u = data->pic.y + data->offset_u;
	data->pic.v = data->pic.y + data->offset_v;
      }
      else
#endif
      {
	data->dec_xframe.image = out_buffer + sizeof (frame_ctrl_t);
      }

      /* decode */
      status =
	xvid_decore (data->handle, XVID_DEC_DECODE, &data->dec_xframe, NULL);;
      if (status)
      {
	LOG_Error ("Decode Frame problem, return value %d", status);
	return CODING_ERROR;
      }

      /* set output values */
      frameattr->framesize = data->framesize;
      attr->packetsize = data->packetsize;

      /* well done */
      return CODING_OK;
      break;
    }				/* end working as decoder */


    /* working as encoder */
  case PLUG_MODE_ENC:
    {
      frameattr->vi.format = attr->target_format;

      /* check for valid quality setting */
      if (attr->curQuality > 4)
	attr->curQuality = 4;
      if (attr->curQuality < 0)
	attr->curQuality = 0;

      /* set xvid flags */
      data->enc_xframe.bitstream = out_buffer + sizeof (frame_ctrl_t);
      data->enc_xframe.length = -1;	/* this is written by the routine */
      data->enc_xframe.image = in_buffer + sizeof (frame_ctrl_t);
      data->enc_xframe.intra = -1;	/* let the codec decide between I-frame (1) and P-frame (0) */
      data->enc_xframe.quant = 0;	/*  is quant != 0, use a fixed quant (and ignore bitrate) */
      data->enc_xframe.motion = motion_presets[attr->curQuality];
      data->enc_xframe.general = general_presets[attr->curQuality];
      data->enc_xframe.quant_intra_matrix = data->enc_xframe.quant_inter_matrix = NULL;	/* we use the internal matrix */

      /* encode frame */
      status =
	xvid_encore (data->handle, XVID_ENC_ENCODE, &data->enc_xframe,
		     &xstats);
      if (status)
      {
	LOG_Error ("Encode Frame problem, return value %d", status);
	return CODING_ERROR;
      }

      /* set outputted values */
      frameattr->keyframe = data->enc_xframe.intra;
      frameattr->framesize = data->enc_xframe.length;

      /* set format , framesize is set by enc_main() */
      frameattr->vi.format = VID_FMT_MPEG4;
      attr->packetsize = frameattr->framesize + sizeof (frame_ctrl_t);

      /* well done */
      return CODING_OK;
      break;
    }				/* end working as encoder */

  default:
    LOG_Error ("MODE NOT SUPPORTED");
    return CODING_ERROR;

  }				/* end switch */

  /* ?? should never be */
  LOG_Error ("unknown error");
  return CODING_ERROR;

}



/* 
 * close codec and free mem 
 */
int
vid_codec_xvid_close (plugin_ctrl_t * attr)
{
  int status = 0;
  struct internal_data *data = (struct internal_data *) attr->handle;

  /* check for valid input */
  if ((attr->mode != PLUG_MODE_DEC) && (attr->mode != PLUG_MODE_ENC))
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }
  if (!attr->handle)
  {
    LOG_Error ("HANDLE FAILURE");
    return -L4_ENOHANDLE;
  }


  /* working as decoder */
  if (attr->mode == PLUG_MODE_DEC)
  {
    status = xvid_decore (data->handle, XVID_DEC_DESTROY, NULL, NULL);
    if (status)
    {
      LOG_Error ("Decore RELEASE problem, return value %d", status);
      return -L4_EUNKNOWN;
    }
  }

  /* working as encoder */
  if (attr->mode == PLUG_MODE_ENC)
  {
    status = xvid_encore (data->handle, XVID_ENC_DESTROY, NULL, NULL);
    if (status)
    {
      LOG_Error ("Encore RELEASE problem, return value %d", status);
      return -L4_EUNKNOWN;
    }
  }

  /* free all */
  free (data);
  attr->handle = NULL;
  strncpy (attr->info, "XviD says bye bye :)", 32);
  return 0;
}





/* 
 * internal helper functions 
 * cr7: code taken from xvid-examples.                               
 */
#define SMALL_EPS 1e-10
#define FRAMERATE_INCR 1001

/*
 * initialize encoder for first use, pass all needed parameters to the codec 
 */
static int
xvid_init_encoder (int *enc_handle, int bitrate, int framerate, int xdim,
		   int ydim)
{
  int xerr;

  XVID_INIT_PARAM xinit;
  XVID_ENC_PARAM xparam;
  /* auto detect cpu caps */
  xinit.cpu_flags = 0;

  xvid_init (NULL, 0, &xinit, NULL);

  xparam.width = xdim;
  xparam.height = ydim;
  if ((framerate - (int) framerate) < SMALL_EPS)
  {
    xparam.fincr = 1;
    xparam.fbase = (int) framerate;
  }
  else
  {
    xparam.fincr = FRAMERATE_INCR;
    xparam.fbase = (int) (FRAMERATE_INCR * framerate);
  }
  xparam.rc_reaction_delay_factor = 16;
  xparam.rc_averaging_period = 100;
  xparam.rc_buffer = 10;
  xparam.rc_bitrate = bitrate * 1000;
  xparam.min_quantizer = 1;
  xparam.max_quantizer = 31;
  xparam.max_key_interval = (int) framerate *10;

  xerr = xvid_encore (NULL, XVID_ENC_CREATE, &xparam, NULL);
  *enc_handle = (int) xparam.handle;

  return xerr;
}


/* 
 * init decoder before first run 
 */
static int
xvid_init_decoder (int *dec_handle, int xdim, int ydim)
{
  int xerr;
  XVID_INIT_PARAM xinit;
  XVID_DEC_PARAM xparam;

  xinit.cpu_flags = 0;		/* autodetect cpu caps */

  xvid_init (NULL, 0, &xinit, NULL);
  xparam.width = xdim;
  xparam.height = ydim;

  xerr = xvid_decore (NULL, XVID_DEC_CREATE, &xparam, NULL);
  *dec_handle = (int) xparam.handle;

  return xerr;
}
