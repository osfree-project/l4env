/*
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include "demux_raw.h"

/* L4 includes */
#include <l4/env/errno.h>
#include <l4/util/macros.h>

#include <stdio.h>
#include <stdlib.h>

/* verner */
#include "arch_globals.h"
/* configuration */
#include "verner_config.h"

/* file I/O */
#include "fileops.h"


struct raw_context
{
  int fd;
  int frameID;
  frame_ctrl_t attr;
};


int
vid_import_raw_init (plugin_ctrl_t * attr, stream_info_t * info)
{
  struct raw_context *raw;

  /* info string */
  strncpy (attr->info, "RAW-Import-Plugin", 32);

  /* check valid mode */
  if (attr->mode != PLUG_MODE_IMPORT)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }

  /* alloc memory */
  attr->handle = malloc (sizeof (struct raw_context));
  if (!attr->handle)
  {
    LOG_Error ("malloc failed!");
    return -L4_ENOMEM;
  }
  memset (attr->handle, 0, sizeof (struct raw_context));
  raw = (struct raw_context *) attr->handle;

  /* open file */
  raw->fd = fileops_open (attr->filename, 1);
  if (raw->fd < 0)
  {
    free (raw);
    attr->handle = NULL;
    LOG_Error ("fileops_open() failed");
    return -L4_EOPEN;
  }

  /* ok we should seek back to start, to decode all. */
  fileops_lseek (raw->fd, 0, SEEK_SET);

#if H264_SLICE_SCHEDULE
  /* FIXME: some hardcoded crap ahead */
  raw->attr.type = info->type = STREAM_TYPE_VIDEO;
  raw->attr.vi.format = info->vi.format = VID_FMT_FOURCC;
  raw->attr.vi.fourCC = info->vi.fourCC = vid_fourcc2int("H264");
  raw->attr.vi.colorspace = info->vi.colorspace = VID_YV12;
  raw->attr.vi.xdim = info->vi.xdim = 704;
  raw->attr.vi.ydim = info->vi.ydim = 576;
  raw->attr.vi.framerate = info->vi.framerate = 25.00;
#else
#warning  The RAW demuxer cannot do any stream parameter detection. Use for special needs only.
#endif

  /* there's no buffer */
  attr->bufferSize = attr->bufferElements = 0;

  return 0;
}

int
vid_import_raw_commit (plugin_ctrl_t * attr)
{
  return 0;
}

int
vid_import_raw_step (plugin_ctrl_t * attr, void *addr)
{
  struct raw_context *raw = (struct raw_context *) attr->handle;
  frame_ctrl_t *packet_frameattr = (frame_ctrl_t *) addr;
  int size;

  /* check for valid handle and mode */
  if (!attr->handle)
  {
    LOG_Error ("HANDLE FAILURE");
    return -L4_ENOHANDLE;
  }
  if (attr->mode != PLUG_MODE_IMPORT)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }

  if ((size = fileops_read (raw->fd, addr + sizeof (frame_ctrl_t), 4096)) <= 0)
  {
    LOGdL (DEBUG_IMPORT, "fileops_read()<=0 (EOF ?)");
    return -L4_ENOTFOUND;
  }

  /* fill in the constant part */
  memcpy (packet_frameattr, &raw->attr, sizeof (frame_ctrl_t));

  packet_frameattr->keyframe = 0;
  if (raw->frameID == 0)
  {
    packet_frameattr->keyframe = RESET_SYNC_POINT;
    packet_frameattr->last_sync_pts = 0.00;
  }
  packet_frameattr->framesize = size;
  /* Packetsize for DSI submit */
  attr->packetsize = size + sizeof (frame_ctrl_t);

  /* Frame ID */
  packet_frameattr->frameID = raw->frameID++;

  return 0;
}

int
vid_import_raw_close (plugin_ctrl_t * attr)
{
  int status = 0;
  struct raw_context *raw = (struct raw_context *)attr->handle;

  /* check for valid handle and mode */
  if (!attr->handle)
  {
    LOG_Error ("HANDLE FAILURE");
    return -L4_ENOHANDLE;
  }
  if (attr->mode != PLUG_MODE_IMPORT)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }

  status = fileops_close (raw->fd);
  free(raw);
  attr->handle = NULL;

  return status;
}

int
vid_import_raw_seek (plugin_ctrl_t * attr, void *addr, double position,
		     int whence)
{
  return -L4_ENOTSUPP;
}
