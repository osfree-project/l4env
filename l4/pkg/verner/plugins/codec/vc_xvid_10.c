/*
 * \brief   Xvid codec plugin for VERNER's core
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

/* configuration */
#include "verner_config.h"

/* OSKit includes */
#include <stdio.h>
#include <stdlib.h>
#include <l4/env/errno.h>

/* xvid stuff */
#include <xvid.h>		/* comes with XviD */

/* local */
#include "vc_xvid_10.h"

/*
 * helper functions 
 */

static int xvid_init_encoder (int *enc_handle, int bitrate, int framerate,
			      int xdim, int ydim, int cpucaps);
static int xvid_init_decoder (int *dec_handle, int xdim, int ydim,
			      int cpucaps);

/* 
 * encoding parameters 
 * FIXME: make them configureable 
 */
#define MAX_ZONES   1
static xvid_enc_zone_t ZONES[MAX_ZONES];
static int NUM_ZONES = 0;
#if 0				//disabled
static int ARG_STATS = 0;
static int ARG_DUMP = 0;
static int ARG_LUMIMASKING = 0;
static int ARG_BITRATE = 0;
static int ARG_SINGLE = 0;
static char *ARG_PASS1 = 0;
static char *ARG_PASS2 = 0;
static int ARG_MAXKEYINTERVAL = 0;
static int ARG_DEBUG = 0;
static int ARG_VOPDEBUG = 0;
#endif
static int ARG_BQRATIO = 150;
static int ARG_BQOFFSET = 100;
static int ARG_MAXBFRAMES = 0;
static int ARG_PACKED = 0;
static int ARG_GMC = 0;
static int ARG_QPEL = 0;
static int ARG_CLOSED_GOP = 0;


/*
 * quality levels for XVID
 * FIXME: NOT tested for QAP.
 */
/*****************************************************************************
 *                            Quality presets
 ****************************************************************************/

static const int motion_presets[] = {
  /* quality 0 */
  0,

  /* quality 1 */
  XVID_ME_ADVANCEDDIAMOND16,

  /* quality 2 */
  XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16,

  /* quality 3 */
  XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 |
    XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8,

  /* quality 4 */
  XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 |
    XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8 |
    XVID_ME_CHROMA_PVOP | XVID_ME_CHROMA_BVOP,

  /* quality 5 */
  XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 |
    XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8 |
    XVID_ME_CHROMA_PVOP | XVID_ME_CHROMA_BVOP,

  /* quality 6 */
  XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 | XVID_ME_EXTSEARCH16 |
    XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8 | XVID_ME_EXTSEARCH8 |
    XVID_ME_CHROMA_PVOP | XVID_ME_CHROMA_BVOP,

};

#define ME_ELEMENTS (sizeof(motion_presets)/sizeof(motion_presets[0]))

static const int vop_presets[] = {
  /* quality 0 */
  0,

  /* quality 1 */
  0,

  /* quality 2 */
  XVID_VOP_HALFPEL,

  /* quality 3 */
  XVID_VOP_HALFPEL | XVID_VOP_INTER4V,

  /* quality 4 */
  XVID_VOP_HALFPEL | XVID_VOP_INTER4V,

  /* quality 5 */
  XVID_VOP_HALFPEL | XVID_VOP_INTER4V | XVID_VOP_TRELLISQUANT,

  /* quality 6 */
  XVID_VOP_HALFPEL | XVID_VOP_INTER4V |
    XVID_VOP_TRELLISQUANT | XVID_VOP_HQACPRED,

};

#define VOP_ELEMENTS (sizeof(vop_presets)/sizeof(vop_presets[0]))

/* internal data */
struct internal_data
{
  void *handle;
  /* for decode */
  xvid_dec_frame_t xvid_dec_frame;
  xvid_dec_stats_t xvid_dec_stats;
  /* for encode */
  xvid_enc_frame_t xvid_enc_frame;
  xvid_enc_stats_t xvid_enc_stats;
  /* these values 'll be precalcalculated for faster decoding step routine */
  int packetsize, framesize;
};

/* 
 * initialize codec 
 */
int
vid_codec_xvid_10_init (plugin_ctrl_t * attr, stream_info_t * info)
{
  int status;
  struct internal_data *data;

  /* verbose info */
  snprintf (attr->info, 32, "XviD Codec %i", XVID_VERSION);

  /* Is there a dumb XviD coder ? */
  if (ME_ELEMENTS != VOP_ELEMENTS)
  {
    fprintf (stderr,
	     "Presets' arrays should have the same number of elements -- Please fill a bug to xvid-devel@xvid.org\n");
    return (-1);
  }
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

  /* determine colorspace */
  switch (attr->target_colorspace)
  {
  case VID_YUV420:
    data->xvid_dec_frame.output.csp = data->xvid_enc_frame.input.csp =
      XVID_CSP_I420;
    data->xvid_enc_frame.input.stride[0] = info->vi.xdim;
    break;
  case VID_YV12:
    data->xvid_dec_frame.output.csp = data->xvid_enc_frame.input.csp =
      XVID_CSP_YV12;
    data->xvid_enc_frame.input.stride[0] = info->vi.xdim;
    break;
  case VID_RGB565:
    data->xvid_dec_frame.output.csp = data->xvid_enc_frame.input.csp =
      XVID_CSP_RGB565;
    data->xvid_enc_frame.input.stride[0] = info->vi.xdim * 3;
    break;
  case VID_RGB24:
    data->xvid_dec_frame.output.csp = data->xvid_enc_frame.input.csp =
      XVID_CSP_BGR;
    data->xvid_enc_frame.input.stride[0] = info->vi.xdim * 3;
    break;
  case VID_UYVY:
    data->xvid_dec_frame.output.csp = data->xvid_enc_frame.input.csp =
      XVID_CSP_UYVY;
    data->xvid_enc_frame.input.stride[0] = info->vi.xdim;
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

    /* precalculate packet size */
    info->vi.colorspace = attr->target_colorspace;
    data->packetsize = vid_streaminfo2packetsize (info);
    data->framesize = data->packetsize - sizeof (frame_ctrl_t);

    /* Set version */
    data->xvid_dec_frame.version = XVID_VERSION;
    data->xvid_dec_stats.version = XVID_VERSION;

    /* init xvid core */
    status =
      xvid_init_decoder ((int *) &data->handle, info->vi.xdim, info->vi.ydim,
			 attr->cpucaps);
    if (status)
    {
      LOG_Error ("Decore INIT problem, return value %d\n", status);
      free (data);
      attr->handle = NULL;
      return -L4_EUNKNOWN;
    }

    /* after decoding we get RAW */
    info->vi.format = VID_FMT_RAW;
    /* set supported qualities */
    attr->minQLevel = 0;
    attr->maxQLevel = 3;
    /* QAP supported - yes it does */
    attr->supportsQAP = 1;
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
			 info->vi.framerate, info->vi.xdim, info->vi.ydim,
			 attr->cpucaps);
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
    attr->minQLevel = 0;
    attr->maxQLevel = ME_ELEMENTS;
    /* QAP supported */
    attr->supportsQAP = 1;
  }

  /* done */
  return 0;
}


/* 
 * encode or decode next frame 
 */
inline int
vid_codec_xvid_10_step (plugin_ctrl_t * attr, unsigned char *in_buffer,
			unsigned char *out_buffer)
{
  int status, used_bytes;
  frame_ctrl_t *frameattr;
  unsigned char *input_ptr;
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

  /* get date from frame_ctrl_t */
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

      /*
       * force at least low delay 
       * for higher quality levels we take internal postprocessing
       */
#if (VCORE_VIDEO_FORCE_LIBPOSTPROC)
      data->xvid_dec_frame.general = XVID_LOWDELAY;
#else
      switch (attr->currentQLevel)
      {
      case 3:
	data->xvid_dec_frame.general =
	  XVID_LOWDELAY | XVID_DEBLOCKY | XVID_DEBLOCKUV | XVID_FILMEFFECT;
	break;
      case 2:
	data->xvid_dec_frame.general =
	  XVID_LOWDELAY | XVID_DEBLOCKY | XVID_DEBLOCKUV;
	break;
      case 1:
	data->xvid_dec_frame.general = XVID_LOWDELAY | XVID_DEBLOCKY;
	break;
      default:
	data->xvid_dec_frame.general = XVID_LOWDELAY;
      }
#endif
      /* Output frame structure */
      data->xvid_dec_frame.output.plane[0] =
	out_buffer + sizeof (frame_ctrl_t);
      data->xvid_dec_frame.output.stride[0] = frameattr->vi.xdim;

      /* Input stream ptr */
      input_ptr = in_buffer + sizeof (frame_ctrl_t);

    repeat:
      /* Input stream */
      data->xvid_dec_frame.bitstream = input_ptr;
      data->xvid_dec_frame.length = frameattr->framesize;
      /* decode */

      used_bytes =
	xvid_decore (data->handle, XVID_DEC_DECODE, &data->xvid_dec_frame,
		     &data->xvid_dec_stats);

      /* check if we've used some bytes */
      if (used_bytes < 0)
      {
	LOG_Error ("Decode Frame problem, return value %d", used_bytes);
	return CODING_ERROR;
      }

      /* check if we got an display able frame */
      if (data->xvid_dec_stats.type == XVID_TYPE_VOL)
      {
	LOGdL (DEBUG_CODEC, "got VOL! must repeat xvid_decore()");
	input_ptr += used_bytes;
	frameattr->framesize -= used_bytes;
	goto repeat;
      }
#if DEBUG_CODEC
      switch (data->xvid_dec_stats.type)
      {
      case XVID_TYPE_NOTHING:
	LOGdL (DEBUG_CODEC, "got NOTHING!");
	break;
      case XVID_TYPE_IVOP:
	LOGdL (DEBUG_CODEC, "got IVOP!");
	break;
      case XVID_TYPE_PVOP:
	LOGdL (DEBUG_CODEC, "got PVOP!");
	break;
      case XVID_TYPE_BVOP:
	LOGdL (DEBUG_CODEC, "got BVOP!");
	break;
      case XVID_TYPE_SVOP:
	LOGdL (DEBUG_CODEC, "got SVOP!");
	break;
      default:
	LOGdL (DEBUG_CODEC, "got error or unknown!");
      }
#endif

      /* check if we've got a valid frame */
      if (data->xvid_dec_stats.type < 0)
	return CODING_ERROR;

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

      if (attr->currentQLevel > ME_ELEMENTS)
	attr->currentQLevel = ME_ELEMENTS;
      if (attr->currentQLevel < 0)
	attr->currentQLevel = 0;

      /* Version for the frame and the stats */
      memset (&data->xvid_enc_frame, 0, sizeof (data->xvid_enc_frame));
      data->xvid_enc_frame.version = XVID_VERSION;

      memset (&data->xvid_enc_stats, 0, sizeof (data->xvid_enc_stats));
      data->xvid_enc_stats.version = XVID_VERSION;

      /* Bind output buffer */
      data->xvid_enc_frame.bitstream = out_buffer + sizeof (frame_ctrl_t);
      data->xvid_enc_frame.length = -1;
      data->xvid_enc_frame.input.plane[0] = in_buffer + sizeof (frame_ctrl_t);

      /* Frame type -- let core decide for us */
      data->xvid_enc_frame.type = XVID_TYPE_AUTO;

      /* Set up core's general features */
      data->xvid_enc_frame.vol_flags = 0;

      if (ARG_QPEL)
	data->xvid_enc_frame.vol_flags |= XVID_VOL_QUARTERPEL;
      if (ARG_GMC)
	data->xvid_enc_frame.vol_flags |= XVID_VOL_GMC;

      /* Set up core's general features */
      data->xvid_enc_frame.vop_flags = vop_presets[attr->currentQLevel];
      if (ARG_GMC)
	data->xvid_enc_frame.vop_flags |= XVID_ME_GME_REFINE;

      /* Force the right quantizer -- It is internally managed by RC plugins */
      data->xvid_enc_frame.quant = 0;

      /* Set up motion estimation flags */
      data->xvid_enc_frame.motion = motion_presets[attr->currentQLevel];

      if (ARG_QPEL)
	data->xvid_enc_frame.motion |= XVID_ME_QUARTERPELREFINE16;
      if (ARG_QPEL && (data->xvid_enc_frame.vop_flags & XVID_VOP_INTER4V))
	data->xvid_enc_frame.motion |= XVID_ME_QUARTERPELREFINE8;

      /* We don't use special matrices */
      data->xvid_enc_frame.quant_intra_matrix = NULL;
      data->xvid_enc_frame.quant_inter_matrix = NULL;

      /* Encode the frame */
      status =
	xvid_encore (data->handle, XVID_ENC_ENCODE, &data->xvid_enc_frame,
		     &data->xvid_enc_stats);
      if (status)
      {
	LOG_Error ("Encode Frame problem, return value %d", status);
	return CODING_ERROR;
      }

      /* get frame type */
      if (data->xvid_enc_stats.type > 0)
      {				/* !XVID_TYPE_NOTHING */

	switch (data->xvid_enc_stats.type)
	{
	case XVID_TYPE_IVOP:
	  frameattr->keyframe = VID_CODEC_IFRAME;
	  break;
	case XVID_TYPE_PVOP:
	  frameattr->keyframe = VID_CODEC_IFRAME;
	  break;
	case XVID_TYPE_BVOP:
	  frameattr->keyframe = VID_CODEC_BFRAME;
	  break;
#if 0
	case XVID_TYPE_SVOP:
	  frameattr->keyframe = "S";
	  break;
	default:
	  frameattr->keyframe = "U";
	  break;
#endif
	}
      }
      /* set output format and size */
      frameattr->framesize = data->xvid_enc_stats.length;
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
vid_codec_xvid_10_close (plugin_ctrl_t * attr)
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
 * helper functions 
 */

#define FRAMERATE_INCR 1001
#define SMALL_EPS (1e-10)

/*
 * Initialize encoder for first use, pass all needed parameters to the codec 
 */
static int
xvid_init_encoder (int *enc_handle, int bitrate, int framerate, int xdim,
		   int ydim, int cpucaps)
{
  int xerr;
#if 0
  xvid_plugin_single_t single;
  xvid_plugin_2pass1_t rc2pass1;
  xvid_plugin_2pass2_t rc2pass2;
#endif
  xvid_enc_plugin_t plugins[7];
  xvid_gbl_init_t xvid_gbl_init;
  xvid_enc_create_t xvid_enc_create;

  /* XviD core initialization */


  /* Set version -- version checking will done by xvidcore */
  memset (&xvid_gbl_init, 0, sizeof (xvid_gbl_init));
  xvid_gbl_init.version = XVID_VERSION;
  xvid_gbl_init.debug = 0;	/*ARG_DEBUG; */


  /* cpu caps */
  xvid_gbl_init.cpu_flags = 0;	/* autodetect */
  //xvid_gbl_init.cpu_flags = cpucaps | XVID_CPU_FORCE; /* take already determined */

  /* Initialize XviD core -- Should be done once per __process__ */
  xvid_global (NULL, XVID_GBL_INIT, &xvid_gbl_init, NULL);

  /* XviD encoder initialization */

  /* Version again */
  memset (&xvid_enc_create, 0, sizeof (xvid_enc_create));
  xvid_enc_create.version = XVID_VERSION;

  /* Width and Height of input frames */
  xvid_enc_create.width = xdim;
  xvid_enc_create.height = ydim;

  /* init plugins  */
  xvid_enc_create.zones = ZONES;
  xvid_enc_create.num_zones = NUM_ZONES;

  xvid_enc_create.plugins = plugins;
  xvid_enc_create.num_plugins = 0;

#if 0
  if (ARG_SINGLE)
  {
    memset (&single, 0, sizeof (xvid_plugin_single_t));
    single.version = XVID_VERSION;
    single.bitrate = ARG_BITRATE;

    plugins[xvid_enc_create.num_plugins].func = xvid_plugin_single;
    plugins[xvid_enc_create.num_plugins].param = &single;
    xvid_enc_create.num_plugins++;
  }

  if (ARG_PASS2)
  {
    memset (&rc2pass2, 0, sizeof (xvid_plugin_2pass2_t));
    rc2pass2.version = XVID_VERSION;
    rc2pass2.filename = ARG_PASS2;
    rc2pass2.bitrate = ARG_BITRATE;

    plugins[xvid_enc_create.num_plugins].func = xvid_plugin_2pass2;
    plugins[xvid_enc_create.num_plugins].param = &rc2pass2;
    xvid_enc_create.num_plugins++;
  }

  if (ARG_PASS1)
  {
    memset (&rc2pass1, 0, sizeof (xvid_plugin_2pass1_t));
    rc2pass1.version = XVID_VERSION;
    rc2pass1.filename = ARG_PASS1;

    plugins[xvid_enc_create.num_plugins].func = xvid_plugin_2pass1;
    plugins[xvid_enc_create.num_plugins].param = &rc2pass1;
    xvid_enc_create.num_plugins++;
  }

  if (ARG_LUMIMASKING)
  {
    plugins[xvid_enc_create.num_plugins].func = xvid_plugin_lumimasking;
    plugins[xvid_enc_create.num_plugins].param = NULL;
    xvid_enc_create.num_plugins++;
  }

  if (ARG_DUMP)
  {
    plugins[xvid_enc_create.num_plugins].func = xvid_plugin_dump;
    plugins[xvid_enc_create.num_plugins].param = NULL;
    xvid_enc_create.num_plugins++;
  }

  if (ARG_DEBUG)
  {
    plugins[xvid_enc_create.num_plugins].func = rawenc_debug;
    plugins[xvid_enc_create.num_plugins].param = NULL;
    xvid_enc_create.num_plugins++;
  }
#endif

  /* No fancy thread tests */
  xvid_enc_create.num_threads = 0;

  /* Frame rate - Do some quick float fps = fincr/fbase hack */
  if ((framerate - (int) framerate) < SMALL_EPS)
  {
    xvid_enc_create.fincr = 1;
    xvid_enc_create.fbase = (int) framerate;
  }
  else
  {
    xvid_enc_create.fincr = FRAMERATE_INCR;
    xvid_enc_create.fbase = (int) (FRAMERATE_INCR * framerate);
  }

  /* Maximum key frame interval */
  xvid_enc_create.max_key_interval = (int) framerate *10;

  /* Bframes settings */
  xvid_enc_create.max_bframes = ARG_MAXBFRAMES;
  xvid_enc_create.bquant_ratio = ARG_BQRATIO;
  xvid_enc_create.bquant_offset = ARG_BQOFFSET;

  /* Dropping ratio frame -- we don't need that */
  xvid_enc_create.frame_drop_ratio = 0;

  /* Global encoder options */
  xvid_enc_create.global = 0;

  if (ARG_PACKED)
    xvid_enc_create.global |=XVID_GLOBAL_PACKED;

  if (ARG_CLOSED_GOP)
    xvid_enc_create.global |=XVID_GLOBAL_CLOSED_GOP;

  xerr = xvid_encore (NULL, XVID_ENC_CREATE, &xvid_enc_create, NULL);

  /* Retrieve the encoder instance from the structure */
  *enc_handle = (int) xvid_enc_create.handle;

  return (xerr);
}


/*
 * init decoder before first run 
 */
static int
xvid_init_decoder (int *dec_handle, int xdim, int ydim, int cpucaps)
{
  int ret;

  xvid_gbl_init_t xvid_gbl_init;
  xvid_dec_create_t xvid_dec_create;

  /* Version */
  xvid_gbl_init.version = XVID_VERSION;

  /* cpu caps */
  xvid_gbl_init.cpu_flags = 0;	/* autodetect */
  //xvid_gbl_init.cpu_flags = cpucaps | XVID_CPU_FORCE; /* take already determined */

  xvid_global (NULL, 0, &xvid_gbl_init, NULL);

  /* Version */
  xvid_dec_create.version = XVID_VERSION;

  /*
   * Image dimensions -- set to 0, xvidcore will resize when ever it is
   * needed
   */
  xvid_dec_create.width = 0;
  xvid_dec_create.height = 0;

  ret = xvid_decore (NULL, XVID_DEC_CREATE, &xvid_dec_create, NULL);

  *dec_handle = (int) xvid_dec_create.handle;

  return (ret);
}

/*
 *   dummy function to build with Xvidcore-cvs w/o any modifications in xvid's src.
 *   Fixme: returning SIG_ERR disabled SSE and SSE2 code in xvid!
 *          But we could overwrite with XVID_FORCE_CPU-flags.
 */
#include <signal.h>
void (*signal (int sig, void (*func) (int))) (int);
void (*signal (int sig, void (*func) (int))) (int)
{
  return SIG_ERR;
}

#ifdef USE_OSKIT

/* taken from dietlibc */
/* Knuth's TAOCP section 3.6 */
#define	M	((1U<<31) -1)
#define	A	48271
#define	Q	44488		// M/A
#define	R	3399		// M%A; R < Q !!!
// FIXME: ISO C/SuS want a longer period
int rand_r (unsigned int *seed);
int
rand_r (unsigned int *seed)
{
  int X;

  X = *seed;
  X = A * (X % Q) - R * (int) (X / Q);
  if (X < 0)
    X += M;

  *seed = X;
  return X;
}
static unsigned int seed = 1;
int
rand (void)
{
  return rand_r (&seed);
}

void
srand (unsigned int i)
{
  seed = i;
}
int random (void) __attribute__ ((alias ("rand")));
void srandom (unsigned int i) __attribute__ ((alias ("srand")));

#endif
