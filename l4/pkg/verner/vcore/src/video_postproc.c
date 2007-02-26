/*
 * \brief   Postprocessing filter setup for VERNER's core
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


#include "postproc.h"
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* configuration */
#include "verner_config.h"


#if VCORE_VIDEO_ENABLE_POSTPROC_ENGINE	/* use postproc engine */


/* remember stream infos to reinit if necessary */
static int pp_current_xdim = -1;
static int pp_current_ydim = -1;
static int pp_current_colorspace = -1;

/* flag if postproc is active */
static int pp_active = 0;
/* flag if postproc is initialized */
static int pp_initialized = 0;
/* noof active default filter */
static int pp_active_default_filter = 0;
/* noof active default filter */
static int pp_default_filter_initialized = 0;
/* noof active user filter */
static int pp_active_user_filter = 0;

/* include filter plugins */
#include "vf_libpostproc.h"

/* default filters */
static plugin_ctrl_t PP_defaults[3];

#if VCORE_VIDEO_STAMP_POSTPROC
/* logos for stamping */
#include "p1_logo.h"
#include "p2_logo.h"
#include "p3_logo.h"
#define LOGO_WIDTH 32
#define LOGO_HEIGHT 32
#endif

/* only allow MAX_USER_PP user defined plugins */
#define MAX_USER_PP 2
#define MAX_USER_PP_OPTIONS_LEN 64
static struct PP_user_t
{
  int used;
  int initialized;
  char pp_options[MAX_USER_PP_OPTIONS_LEN];
  plugin_ctrl_t plugin_ctrl;
  /* function ptrs to filter specific functions */
  int (*vf_init) (plugin_ctrl_t * attr, stream_info_t * info, char *options);
  int (*vf_step) (plugin_ctrl_t * attr, unsigned char *buffer);
  int (*vf_close) (plugin_ctrl_t * attr);

} PP_user[MAX_USER_PP];


static void
initDefaultFilter (control_struct_t * control)
{
  LOGdL (DEBUG_POSTPROC, "initializing default postprocessing levels");
  /* close old default filters */
  if (pp_default_filter_initialized)
  {
    pp_default_filter_initialized = 0;
    vf_libpostproc_close (&PP_defaults[0]);
    vf_libpostproc_close (&PP_defaults[1]);
    vf_libpostproc_close (&PP_defaults[2]);
  }

  /* copy stream value into default plugin_ctrl_t */
  memcpy (&PP_defaults[0], &control->plugin_ctrl, sizeof (plugin_ctrl_t));
  memcpy (&PP_defaults[1], &control->plugin_ctrl, sizeof (plugin_ctrl_t));
  memcpy (&PP_defaults[2], &control->plugin_ctrl, sizeof (plugin_ctrl_t));

  /* init filter */
  if ((vf_libpostproc_init
       (&PP_defaults[0], &control->streaminfo, VCORE_VIDEO_DEFAULT_PP_Q1))
      ||
      (vf_libpostproc_init
       (&PP_defaults[1], &control->streaminfo, VCORE_VIDEO_DEFAULT_PP_Q2))
      ||
      (vf_libpostproc_init
       (&PP_defaults[2], &control->streaminfo, VCORE_VIDEO_DEFAULT_PP_Q3)))
  {
    LOG_Error ("failed to init default filters!");
    /* close already opened */
    vf_libpostproc_close (&PP_defaults[0]);
    vf_libpostproc_close (&PP_defaults[1]);
    vf_libpostproc_close (&PP_defaults[2]);
    pp_default_filter_initialized = 0;
    return;
  }
  LOGdL (DEBUG_POSTPROC, "default filters 1-3 enabled and availiable.");
  pp_default_filter_initialized = 1;
}

/* execute command */
int
postProcessEngineCommand (const char *command, const char *ppName,
			  const char *ppOptions)
{
  int i;			/* for-loop counter */

  /* initilialize filters */
  if (!pp_initialized)
  {
    LOGdL (DEBUG_POSTPROC, "initilialize filters");
    /* setting to ZERO */
    for (i = 0; i < MAX_USER_PP; i++)
    {
      memset (&PP_user[i], 0, sizeof (struct PP_user_t));
    }
  }
  pp_initialized = 1;

  LOGdL (DEBUG_POSTPROC, "command \"%s %s %s\" received.", command, ppName,
	 ppOptions);

  /* process add command */
  if (!strcasecmp ("add", command))
  {
    /* default filter */
    if (!strcasecmp ("default", ppName))
    {
      if (!strcasecmp ("1", ppOptions))
	pp_active_default_filter = 1;
      if (!strcasecmp ("2", ppOptions))
	pp_active_default_filter = 2;
      if (!strcasecmp ("3", ppOptions))
	pp_active_default_filter = 3;
    }
    /* libpostproc */
    if (!strcasecmp ("libpostproc", ppName))
    {
      /* get first free user slot */
      int free = -1;
      for (i = 0; i < MAX_USER_PP; i++)
	if (!PP_user[i].used)
	{
	  free = i;
	  break;
	}
      if ((free >= 0) && (free < MAX_USER_PP))
      {
	/* copy all necessary info to init plugin */
	LOGdL (DEBUG_POSTPROC,
	       "found free slot and initializing libpostproc-plugin.");
	PP_user[free].used = 1;
	PP_user[free].initialized = 0;
	/* remember options string */
	strncpy (PP_user[free].pp_options, ppOptions,
		 MAX_USER_PP_OPTIONS_LEN);
	/*set function ptrs */
	PP_user[free].vf_init = vf_libpostproc_init;
	PP_user[free].vf_step = vf_libpostproc_step;
	PP_user[free].vf_close = vf_libpostproc_close;
	pp_active_user_filter++;
      }
      else
	LOG_Error ("no free slot found. Can't init plugin.");
    }
  }
  /* process delete command */
  if (!strcasecmp ("delete", command))
  {

    /* close default filters */
    if ((!strcasecmp ("default", ppName)) && (pp_default_filter_initialized))
    {
      pp_default_filter_initialized = 0;
      vf_libpostproc_close (&PP_defaults[0]);
      vf_libpostproc_close (&PP_defaults[1]);
      vf_libpostproc_close (&PP_defaults[2]);
    }
    /* close user filters */
    if ((!strcasecmp ("libpostproc", ppName))
	&& (pp_default_filter_initialized))
    {
      for (i = 0; i < MAX_USER_PP; i++)
	if ((PP_user[i].used) && (PP_user[i].initialized)
	    && (!strcasecmp (PP_user[i].pp_options, ppOptions)))
	{
	  if (PP_user[i].vf_close != NULL)
	    PP_user[i].vf_close (&PP_user[i].plugin_ctrl);
	  memset (&PP_user[i], 0, sizeof (struct PP_user_t));
	  pp_active_user_filter--;

	}
    }
    /* no filters avail - so we deactivate the proccessing engine */
    if ((!pp_active_user_filter) && (!pp_default_filter_initialized))
      pp_active = 0;
  }


  /* process de/active command */
  if (!strcasecmp ("activate", command))
  {
    pp_active = 1;
    pp_current_colorspace = -1; /* force to reinit filter! */
  }
  if (!strcasecmp ("deactivate", command))
    pp_active = 0;


  return 0;
}

/* execute selected filters */
inline int
postProcessEngineStep (control_struct_t * control, unsigned char *buffer)
{
  int i;			/* for-loop counter */
#if VCORE_VIDEO_STAMP_POSTPROC
  void *src = NULL;
  void *dest;
#endif


  /* apply filter only when decoding ! */
  /* filters initialized and active ? */
  if ((!control->plugin_ctrl.mode == PLUG_MODE_DEC) || (!pp_active))
  {
    LOGdL (DEBUG_POSTPROC, "non active filters or not decoding.");
    return 0;
  }

  /*
   * QAP: add default filters as QAP level
   * when qap is enabled and codec doesn't support it's own levels and
   * we should use pp as QAP 
   *
   * mmhhh... it's not the best to put here, but where else ? 
   */
#if VCORE_VIDEO_ENABLE_QAP
  if ((control->qap_settings.useQAP) && (control->qap_settings.usePPasQAP)
      && (!control->plugin_ctrl.supportsQAP))
    pp_active_default_filter = control->plugin_ctrl.currentQLevel;
#endif

  /* verbose settings after QAP */
  LOGdL (DEBUG_POSTPROC, "pp_default_init=%i;active_default=%i;",pp_default_filter_initialized, pp_active_default_filter);

  /* detect parameter changes and reeinit ALL filters in chain */
  if ((control->streaminfo.vi.ydim != pp_current_ydim)
      || (control->streaminfo.vi.xdim != pp_current_xdim)
      || (control->plugin_ctrl.target_colorspace != pp_current_colorspace))
  {
    LOGdL (DEBUG_POSTPROC,
	   "video params have changed, we'll reeinit ALL filters");
    /* store relevant values to compare */
    pp_current_ydim = control->streaminfo.vi.ydim;
    pp_current_xdim = control->streaminfo.vi.xdim;
    pp_current_colorspace = control->plugin_ctrl.target_colorspace;
    /* default filters */
    initDefaultFilter (control);
    /* user filters */
    for (i = 0; i < MAX_USER_PP; i++)
      if (PP_user[i].used)
      {
	LOGdL (DEBUG_POSTPROC, "init user filter %i", i);
	if (PP_user[i].initialized)
	  PP_user[i].vf_close (&PP_user[i].plugin_ctrl);
	/* copy stream value into default plugin_ctrl_t */
	memcpy (&PP_user[i].plugin_ctrl, &control->plugin_ctrl,
		sizeof (plugin_ctrl_t));
	/* init filter */
	if (!PP_user[i].
	    vf_init (&PP_user[i].plugin_ctrl, &control->streaminfo,
		     (char *) &PP_user[i].pp_options))
	  PP_user[i].initialized = 1;
	else
	  PP_user[i].initialized = 0;
      }

  }


  /* filters now initialized ? */
  if (!pp_initialized)
  {
    LOGdL (DEBUG_POSTPROC, "filters not initialized.");
    return 0;
  }


  /* apply default filter */
  if ((pp_default_filter_initialized) && (pp_active_default_filter))
  {
    LOGdL (DEBUG_POSTPROC, "applying default filter %i.",
	   pp_active_default_filter);
    if ((pp_active_default_filter > 1) && (pp_active_default_filter <= 3))
      vf_libpostproc_step (&PP_defaults[pp_active_default_filter - 1],
			   buffer);
  }

  /* apply user filters */
  /* may be we've got an filter in use, but not already initialized */
  if (pp_active_user_filter);
  for (i = 0; i < MAX_USER_PP; i++)
    if (PP_user[i].used)
    {
      LOGdL (DEBUG_POSTPROC,
	     "apply user filter filter %i (%s)", i, PP_user[i].pp_options);
      if (!PP_user[i].initialized)
      {

	LOGdL (DEBUG_POSTPROC, "init user filter %i", i);
	/* copy stream value into default plugin_ctrl_t */
	memcpy (&PP_user[i].plugin_ctrl, &control->plugin_ctrl,
		sizeof (plugin_ctrl_t));
	/* init filter */
	if (!PP_user[i].
	    vf_init (&PP_user[i].plugin_ctrl, &control->streaminfo,
		     (char *) &PP_user[i].pp_options))
	  PP_user[i].initialized = 1;
	else
	  PP_user[i].initialized = 0;
      }
      else
	PP_user[i].vf_step (&PP_user[i].plugin_ctrl, buffer);

    }

#if VCORE_VIDEO_STAMP_POSTPROC
    /* stamp a logo into picture. YUV420 only, default filters only */
    if((control->plugin_ctrl.target_colorspace == VID_YUV420) && (pp_active_default_filter))
    {
	if (pp_active_default_filter == 1) src = p1_logo_data;
	else if (pp_active_default_filter == 2) src = p2_logo_data;
	else if (pp_active_default_filter == 3) src = p3_logo_data;
	/* no valid picture */
	else return 0;
	/* copy logo into picture buffer */
	for (i = 0; i<LOGO_HEIGHT; i++)
	{
	  dest = buffer + (i*control->streaminfo.vi.xdim) + sizeof(frame_ctrl_t);
	  memcpy(dest,src + (LOGO_WIDTH*i),LOGO_WIDTH);
	}
    }
#endif

  return 0;
}

/* close all filters */
int
postProcessEngineClose ()
{
  int i;			/* for-loop counter */

  pp_initialized = 0;
  LOGdL (DEBUG_POSTPROC, "closing postProcessEngine.");

  /* close default predefined filters */
  if (pp_default_filter_initialized)
  {
    vf_libpostproc_close (&PP_defaults[0]);
    vf_libpostproc_close (&PP_defaults[1]);
    vf_libpostproc_close (&PP_defaults[2]);
    pp_default_filter_initialized = 0;
  }

  /* close user filters */
  for (i = 0; i < MAX_USER_PP; i++)
    if ((PP_user[i].used) && (PP_user[i].initialized))
    {
      if (PP_user[i].vf_close != NULL)
	PP_user[i].vf_close (&PP_user[i].plugin_ctrl);
    }
  pp_active_user_filter = 0;

  return 0;
}


#else /* end use postproc engine */
/* don't use postproc engine */

/* execute command */
int
postProcessEngineCommand (const char *command, const char *ppName,
			  const char *ppOptions)
{
  LOG_Error ("postprocessing is disabled at compile time.");
  return -L4_ENOTSUPP;
}

/* execute selected filters */
inline int
postProcessEngineStep (control_struct_t * control, unsigned char *buffer)
{
  return 0;
}

/* close all filters */
int
postProcessEngineClose ()
{
  return 0;
}
#endif /* don't use postproc engine */
