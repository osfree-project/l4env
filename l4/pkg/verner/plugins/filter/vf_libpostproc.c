/*
 * \brief   Postprocessing filter setup for VERNER's core
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
 * based upon:
 *
 *
 *  filter_pp.c
 *
 *  Copyright (C) Gerhard Monzel - Januar 2002
 *
 *  This file is part of transcode, a linux video stream processing tool
 *      
 *  transcode is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  transcode is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

/* l4 */
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* LibC includes */
#include <stdlib.h>		/* malloc, free */
#include <inttypes.h>

/* local */
#include "arch_globals.h"
#include "vf_libpostproc.h"

/* configuration */
#include "verner_config.h"

/* libpostproc */
#include <postprocess.h>

typedef struct
{
  int height;
  int width;
  int pre;
  pp_context_t *context;
  pp_mode_t *mode;
} pp_type;


/* internal helper functions */
static int filter_init (char *options, unsigned int colorspace, int width,
			int height, int cpucaps, pp_type * pp_t);



/* init plugin */
int
vf_libpostproc_init (plugin_ctrl_t * attr, stream_info_t * info,
		     char *options)
{
  pp_type *pp;

  /* verbose info */
  strncpy (attr->info, "libpostproc", 32);

  /* check valid stream type */
  if (info->type != STREAM_TYPE_VIDEO)
  {
    LOG_Error ("only video streams supported.");
    return -L4_ENOTSUPP;
  }

  /* alloc memory */
  attr->handle = (pp_type *) malloc (sizeof (pp_type));
  if (!attr->handle)
  {
    LOG_Error ("malloc failed!");
    return -L4_ENOMEM;
  }
  pp = (pp_type *) attr->handle;

  /* init filter */
  if (filter_init
      (options, attr->target_colorspace, info->vi.xdim, info->vi.ydim,
       attr->cpucaps, pp))
  {
    LOG_Error ("initialize filter failed.");
    free (attr->handle);
    attr->handle = NULL;
    return -L4_EUNKNOWN;
  }

  /* done */
  return 0;
}

/* filter one frame */
int
vf_libpostproc_step (plugin_ctrl_t * attr, unsigned char *buffer)
{
  unsigned char *pp_page[3];
  int ppStride[3];
  pp_type *pp_t = (pp_type *) attr->handle;

  /* check for valid handle and mode */
  if (!attr->handle)
  {
    LOG_Error ("HANDLE FAILURE");
    return CODING_ERROR;
  }
  if (attr->mode != PLUG_MODE_DEC)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return CODING_ERROR;
  }

  /* now filter */
  pp_page[0] = buffer + sizeof (frame_ctrl_t);
  pp_page[1] = pp_page[0] + (pp_t->width * pp_t->height);
  pp_page[2] = pp_page[1] + (pp_t->width * pp_t->height) / 4;

  ppStride[0] = pp_t->width;
  ppStride[1] = ppStride[2] = pp_t->width >> 1;

  pp_postprocess (pp_page, ppStride,
		  pp_page, ppStride,
		  pp_t->width, pp_t->height,
		  NULL, 0, pp_t->mode, pp_t->context, 0);

  /* done */
  return 0;
}

/* close plugin */
int
vf_libpostproc_close (plugin_ctrl_t * attr)
{
  pp_type *pp_t = (pp_type *) attr->handle;

  /* close filter */
  LOGdL (DEBUG_POSTPROC, "closing filter.");
  if (attr->handle)
  {
    if (pp_t->mode)
      pp_free_mode (pp_t->mode);
    pp_t->mode = NULL;
    if (pp_t->context)
      pp_free_context (pp_t->context);
    pp_t->context = NULL;
    free (attr->handle);
    attr->handle = NULL;
  }
  return 0;
}




/* internal helper functions */
static int isalpha (int ch);

static int
isalpha (int ch)
{
  return (unsigned int) ((ch | 0x20) - 'a') < 26u;
}

static void
optstr_help (void)
{
  LOGdL (DEBUG_POSTPROC,
	 "<filterName>[:<option>[:<option>...]][[|/][-]<filterName>[:<option>...]]...\n"
	 "long form example:\n"
	 "vdeblock:autoq/hdeblock:autoq/linblenddeint    default,-vdeblock\n"
	 "short form example:\n"
	 "vb:a/hb:a/lb                                   de,-vb\n"
	 "more examples:\n" "tn:64:128:256\n"
	 "Filters                        Options\n"
	 "short  long name       short   long option     Description\n"
	 "*      *               a       autoq           cpu power dependant enabler\n"
	 "                       c       chrom           chrominance filtring enabled\n"
	 "                       y       nochrom         chrominance filtring disabled\n"
	 "hb     hdeblock        (2 Threshold)           horizontal deblocking filter\n"
	 "       1. difference factor: default=64, higher -> more deblocking\n"
	 "       2. flatness threshold: default=40, lower -> more deblocking\n"
	 "                       the h & v deblocking filters share these\n"
	 "                       so u cant set different thresholds for h / v\n"
	 "vb     vdeblock        (2 Threshold)           vertical deblocking filter\n"
	 "h1     x1hdeblock                              Experimental h deblock filter 1\n"
	 "v1     x1vdeblock                              Experimental v deblock filter 1\n"
	 "dr     dering                                  Deringing filter\n"
	 "al     autolevels                              automatic brightness / contrast\n"
	 "                       f       fullyrange      stretch luminance to (0..255)\n"
	 "lb     linblenddeint                           linear blend deinterlacer\n"
	 "li     linipoldeint                            linear interpolating deinterlace\n"
	 "ci     cubicipoldeint                          cubic interpolating deinterlacer\n"
	 "md     mediandeint                             median deinterlacer\n"
	 "fd     ffmpegdeint                             ffmpeg deinterlacer\n"
	 "de     default                                 hb:a,vb:a,dr:a,al\n"
	 "fa     fast                                    h1:a,v1:a,dr:a,al\n"
	 "tn     tmpnoise        (3 Thresholds)          Temporal Noise Reducer\n"
	 "                       1. <= 2. <= 3.          larger -> stronger filtering\n"
	 "fq     forceQuant      <quantizer>             Force quantizer\n");
}


static int
no_optstr (char *s)
{
  int result = 0;		/* decrement if transcode, increment if mplayer */
  char *c = s;

  while (c && *c && (c = strchr (c, '=')))
  {
    result--;
    c++;
  }
  c = s;
  while (c && *c && (c = strchr (c, '/')))
  {
    result++;
    c++;
  }
  c = s;
  while (c && *c && (c = strchr (c, '|')))
  {
    result++;
    c++;
  }
  c = s;
  while (c && *c && (c = strchr (c, ',')))
  {
    result++;
    c++;
  }


  return (result <= 0) ? 0 : 1;
}

static void
do_optstr (char *opts)
{
  opts++;

  while (*opts)
  {

    if (*(opts - 1) == ':')
    {
      if (isalpha (*opts))
      {
	if ((strncmp (opts, "autoq", 5) == 0)
	    || (strncmp (opts, "chrom", 5) == 0)
	    || (strncmp (opts, "nochrom", 7) == 0)
	    || ((strncmp (opts, "a", 1) == 0)
		&& (strncmp (opts, "al", 2) != 0))
	    || ((strncmp (opts, "c", 1) == 0)
		&& (strncmp (opts, "ci", 2) != 0))
	    || (strncmp (opts, "y", 1) == 0))
	{
	  opts++;
	  continue;
	}
	else
	{
	  *(opts - 1) = '/';
	}
      }


    }

    if (*opts == '=')
      *opts = ':';

    opts++;
  }
}

static char *
pp_lookup (char *haystack, char *needle)
{
  char *ch = haystack;
  int found = 0;
  int len = strlen (needle);

  while (!found)
  {
    ch = strstr (ch, needle);

    if (!ch)
      break;

    if (ch[len] == '\0' || ch[len] == '=' || ch[len] == '/')
    {
      found = 1;
    }
    else
    {
      ch++;
    }
  }

  return (ch);


}



/*
 filter init
*/
int
filter_init (char *options, unsigned int colorspace, int width, int height,
	     int cpucaps, pp_type * pp_t)
{
  char *c;
  int len = 0;

  if ((colorspace != VID_YV12) && (colorspace != VID_YUV420))
  {
    LOGdL (DEBUG_POSTPROC,
	   "filter is only capable for YV12 and YUV420-Mode!");
    return (-1);
  }

  if (!options || !(len = strlen (options)))
  {
    LOGdL (DEBUG_POSTPROC, "this filter needs options!");
    return (-1);
  }


  if (!no_optstr (options))
  {
    do_optstr (options);
  }

  /* if "pre" is found, delete it */
  if ((c = pp_lookup (options, "pre")))
  {
    memmove (c, c + 3, &options[len] - c);
    pp_t->pre = 1;
  }

  if ((c = pp_lookup (options, "help")))
  {
    memmove (c, c + 4, &options[len] - c);
    optstr_help ();
  }

  /* printf( "[%s] after pre (%s)\n", MOD_NAME, options); */

  pp_t->mode = pp_get_mode_by_name_and_quality (options, PP_QUALITY_MAX);

  if (pp_t->mode == NULL)
  {
    LOGdL (DEBUG_POSTPROC,
	   "internal error (pp_get_mode_by_name_and_quality)");
    return (-1);
  }

  /* check cpu caps */
  if (cpucaps & (1 << 1))	/* CPU_MMXEXT (1<< 1) */
  {
    pp_t->context = pp_get_context (width, height, PP_CPU_CAPS_MMX2);
    LOGdL (DEBUG_POSTPROC, "Using MMX2 enabled PP-filters.");
  }
  else if (cpucaps & (1 << 4))	/* CPU_3DNOW (1<< 4) */
  {
    pp_t->context = pp_get_context (width, height, PP_CPU_CAPS_3DNOW);
    LOGdL (DEBUG_POSTPROC, "Using 3DNOW enabled PP-filters.");
  }
  else if (cpucaps & (1 << 0))	/* CPU_MMX (1<< 0) */
  {
    pp_t->context = pp_get_context (width, height, PP_CPU_CAPS_MMX);
    LOGdL (DEBUG_POSTPROC, "Using MMX enabled PP-filters.");
  }
  else
  {
    pp_t->context = pp_get_context (width, height, 0);
    LOGdL (DEBUG_POSTPROC, "Using simple C PP-filters.");
  }

  if (pp_t->context == NULL)
  {
    LOGdL (DEBUG_POSTPROC, "internal error (pp_get_context)");
    return (-1);
  }

  /* save width, height */
  pp_t->width = width;
  pp_t->height = height;

  /* filter init ok. */
  return (0);
}
