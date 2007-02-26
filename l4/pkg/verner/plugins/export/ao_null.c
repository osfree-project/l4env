/*
 * \brief   Audio specific for VERNER's sync component
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

/* L4 includes */
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/sys/types.h>
#include <l4/generic_io/libio.h>
#include <l4/dde_linux/dde.h>
#include <l4/dde_linux/sound.h>

/* Linux */
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/soundcard.h>

/* verner */
#include "arch_globals.h"

/* configuration */
#include "verner_config.h"

/* local */
#include "ao_null.h"




/*
 * init soundcard
 */
int
ao_null_init (plugin_ctrl_t * attr, stream_info_t * info)
{
  return -L4_ENOTSUPP;
}

/*
 * commit chung - unused
 */
int
ao_null_commit (plugin_ctrl_t * attr)
{
  /* nothing todo */
  return 0;
}


/*
 * play one submitted chunk
 */
int
ao_null_step (plugin_ctrl_t * attr, void *addr)
{
  return -L4_ENOTSUPP;
}

/*
 * close device
 */
int
ao_null_close (plugin_ctrl_t * attr)
{
  return -L4_ENOTSUPP;
}


/*
 * get playback position in millisec
 */
int
ao_null_getPosition (plugin_ctrl_t * attr, double *position)
{
  return 0;
}

/* 
 * set volume (pcm + master)
 */
int
ao_oss_setVolume (plugin_ctrl_t * attr, int left, int right)
{
  return -L4_ENOTSUPP;
}
