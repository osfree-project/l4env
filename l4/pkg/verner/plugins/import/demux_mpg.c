/*
 * \brief   MPEG demuxeing plugin for VERNER
 * \date    2004-09-09
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

#include "demux_mpg.h"

/* verner */
#include "arch_globals.h"
/* configuration */
#include "verner_config.h"

/* L4 includes */
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* OSKit includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* file I/O */
#include "fileops.h"

/* globals */
#include "arch_globals.h"


/* internal data */
#define READBUF_SIZE (32678-sizeof(frame_ctrl_t))
typedef struct vid_mpeg_data
{
  long frameID;
  unsigned char *readbuf;
  int readbuf_ptr;		/* bytes currently in readbuf */
  int fd;			/* File Descriptor */
  frame_ctrl_t frameattr;

/* reading buffer for file-I/O 
   FIXME: move into mpeg_d */
#define BUFFER_SIZE 4096
  unsigned char buffer[BUFFER_SIZE];
  unsigned char *end;

#define MPEG1PS 1
#define MPEG1ES 2
#define MPEG2PS 3
#define MPEG2ES 4
  int type;			/* mpeg and stream type */

  int demux_track;		/* ps */
  int demux_pid;		/* =0  */
  //int demux_pva = 0;

  signed long pts;

}
vid_mpeg_data_t;


/* internal functions - prototypes from extract_mpeg2.c */
static int demux (unsigned char *buf, unsigned char *end, int flags,
		  vid_mpeg_data_t * mpeg_d);
static int ps_loop (vid_mpeg_data_t * mpeg_d);
/*
static void ts_loop (void);
static int pva_demux (unsigned char * buf, unsigned char * end);
static void pva_loop (void);
*/

/* end extract_mpeg2.c */

int
vid_import_mpg_init (plugin_ctrl_t * attr, stream_info_t * info)
{
  static unsigned char buf[16];	/* buffer for probing */
  struct vid_mpeg_data *mpeg_d;

  strncpy (attr->info, "video demux MPEG(1/2)", 32);

  if (attr->mode != PLUG_MODE_IMPORT)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }

  /* alloc internal structure */
  attr->handle =
    (struct vid_mpeg_data *) malloc (sizeof (struct vid_mpeg_data));
  if (!attr->handle)
  {
    LOG_Error ("malloc failed!");
    return -L4_ENOHANDLE;
  }
  memset (attr->handle, 0, sizeof (struct vid_mpeg_data));
  mpeg_d = (struct vid_mpeg_data *) attr->handle;

  /* open file */
  mpeg_d->fd = fileops_open (attr->filename, 1);
  if (mpeg_d->fd < 0)
  {
    free (mpeg_d);
    LOG_Error ("file_open failed");
    attr->handle = NULL;
    return -L4_ENOMEM;
  }

  /* select track no */
  /* FIXME: add support for others streams */
  mpeg_d->demux_track = 0xe0;
  mpeg_d->demux_pid = 0;
  mpeg_d->pts = 0;

  /* probe mpeg */
  fileops_lseek (mpeg_d->fd, 0, SEEK_SET);
  if (fileops_read (mpeg_d->fd, buf, 16) == 16)
  {

    if (!buf[0] && !buf[1] && (buf[2] == 0x01))
      switch (buf[3])
      {
      case 0xba:		/* pack header */
	/* skip */
	if ((buf[4] & 0xf0) == 0x20)	/* mpeg1 */
	{
	  printf ("vid_import_mpeg.c: MPEG-1 PS detected\n");
	  mpeg_d->type = MPEG1PS;
	  break;
	}
	else if ((buf[4] & 0xc0) == 0x40)	/* mpeg2 */
	{
	  printf ("vid_import_mpeg.c: MPEG-2 PS detected\n");
	  mpeg_d->type = MPEG2PS;
	}
	else
	{
	  printf ("vid_import_mpeg.c: weird pack header\n");
	}

      case 0xb3:

	/* MPEG video ES */
	if ((buf[6] & 0xc0) == 0x80)
	{
	  printf ("vid_import_mpeg.c: MPEG-2 ES detected\n");
	  mpeg_d->type = MPEG2ES;
	}
	else
	{
	  printf ("vid_import_mpeg.c: MPEG-1 ES detected\n");
	  mpeg_d->type = MPEG1ES;
	}
	break;

      default:
	if (buf[3] < 0xb9)
	{
	  printf
	    ("vid_import_mpeg.c: looks like an elementary stream - not program stream\n");
	  if ((buf[6] & 0xc0) == 0x80)
	  {
	    printf ("vid_import_mpeg.c: MPEG-2 ES detected\n");
	    mpeg_d->type = MPEG2PS;
	  }
	  else
	  {
	    printf ("vid_import_mpeg.c: MPEG-1 ES detected\n");
	    mpeg_d->type = MPEG1ES;
	  }
	}
	break;
      }
  }

  if (mpeg_d->type == 0)
  {
    printf ("vid_import_mpeg.c: no MPEG stream found.\n");
    free (mpeg_d);
    attr->handle = NULL;
    return -L4_ENOTFOUND;

  }

  /*Frametype as default  -  doesn't change in stream */
  mpeg_d->frameattr.type = info->type = STREAM_TYPE_VIDEO;
  if ((mpeg_d->type == MPEG1PS) || (mpeg_d->type == MPEG1ES))
  {
    mpeg_d->frameattr.vi.format = info->vi.format = VID_FMT_MPEG1;
    mpeg_d->frameattr.vi.fourCC = info->vi.fourCC = vid_fourcc2int ("MPG1");
  }
  else
  {
    mpeg_d->frameattr.vi.format = info->vi.format = VID_FMT_MPEG2;
    mpeg_d->frameattr.vi.fourCC = info->vi.fourCC = vid_fourcc2int ("MPG2");
  }
  mpeg_d->frameattr.vi.colorspace = info->vi.colorspace = VID_YV12;

  /* ok we should seek back to start, to get all data. */
  fileops_lseek (mpeg_d->fd, 0, SEEK_SET);

  attr->bufferSize = attr->bufferElements = 0;	/* there's no buffer */
  return 0;
}


int
vid_import_mpg_commit (plugin_ctrl_t * attr)
{
  return 0;
}

int
vid_import_mpg_step (plugin_ctrl_t * attr, void *addr)
{

  long framesize;
  struct vid_mpeg_data *mpeg_d;
  frame_ctrl_t *frameattr;

  mpeg_d = (struct vid_mpeg_data *) attr->handle;

  if (!attr->handle)
  {
    LOG_Error ("HANDLE FAILURE");
    return NULL;
  }

  if (attr->mode != PLUG_MODE_IMPORT)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return NULL;
  }

  mpeg_d->readbuf = addr;
  /* PS */
  if ((mpeg_d->type == MPEG1PS) || (mpeg_d->type == MPEG2PS))
  {
    if (ps_loop (mpeg_d) != 0)
      return -L4_EUNKNOWN;
    framesize = mpeg_d->readbuf_ptr;
  }
  else
    /* ES */
  if ((mpeg_d->type == MPEG1ES) || (mpeg_d->type == MPEG2ES))
  {
    if ((framesize =
	 fileops_read (mpeg_d->fd, (addr + sizeof (frame_ctrl_t)),
		       4096)) <= 0)
    {
      LOG_Error ("fileops_read()<=0 (EOF ?), FrameID=%i",
		 (int) mpeg_d->frameID);
      return -L4_EOPEN;
    }
  }
  else
  {
    LOG_Error (" unsupported streamtype.");
    return -L4_ENOTSUPP;
  }

  frameattr = (frame_ctrl_t *) addr;

  /* first copy unchangeable */
  memcpy (frameattr, &mpeg_d->frameattr, sizeof (frame_ctrl_t));

  /* Frametype */
  frameattr->keyframe = 1;
  if (mpeg_d->frameID == 0)	//sync reset ptr
  {
    frameattr->keyframe = RESET_SYNC_POINT;
    frameattr->last_sync_pts = 0.00;
  }
  /* framesize */
  frameattr->framesize = framesize;
  /* Packetsize for DSI submit */
  attr->packetsize = frameattr->framesize + sizeof (frame_ctrl_t);

  /* set frame ID */
  frameattr->frameID = mpeg_d->frameID;
  mpeg_d->frameID++;

  /* done */
  return 0;
}

int
vid_import_mpg_close (plugin_ctrl_t * attr)
{
  int status = 0;
  struct vid_mpeg_data *mpeg_d;
  mpeg_d = (struct vid_mpeg_data *) attr->handle;

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
  fileops_close (mpeg_d->fd);
  /* struct free */
  free (mpeg_d);
  attr->handle = NULL;

  return status;
}



int
vid_import_mpg_seek (plugin_ctrl_t * attr, void *addr, double position,
		     int whence)
{
  /* not supported yet */
  return NULL;
}



/*
 * extract_mpeg2.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */



#define DEMUX_PAYLOAD_START 1
static int
demux (unsigned char *buf, unsigned char *end, int flags,
       struct vid_mpeg_data *mpeg_d)
{
  static int mpeg1_skip_table[16] = {
    0, 0, 4, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };

  /*
   * the demuxer keeps some state between calls:
   * if "state" = DEMUX_HEADER, then "head_buf" contains the first
   *     "bytes" bytes from some header.
   * if "state" == DEMUX_DATA, then we need to copy "bytes" bytes
   *     of ES data before the next header.
   * if "state" == DEMUX_SKIP, then we need to skip "bytes" bytes
   *     of data before the next header.
   *
   * NEEDBYTES makes sure we have the requested number of bytes for a
   * header. If we dont, it copies what we have into head_buf and returns,
   * so that when we come back with more data we finish decoding this header.
   *
   * DONEBYTES updates "buf" to point after the header we just parsed.
   */

#define DEMUX_HEADER 0
#define DEMUX_DATA 1
#define DEMUX_SKIP 2
  static int state = DEMUX_SKIP;
  static int state_bytes = 0;
  static unsigned char head_buf[264];

  unsigned char *header;
  int bytes;
  int len;


#define NEEDBYTES(x)						\
    do {							\
	int missing;						\
								\
	missing = (x) - bytes;					\
	if (missing > 0) {					\
	    if (header == head_buf) {				\
		if (missing <= end - buf) {			\
		    memcpy (header + bytes, buf, missing);	\
		    buf += missing;				\
		    bytes = (x);				\
		} else {					\
		    memcpy (header + bytes, buf, end - buf);	\
		    state_bytes = bytes + end - buf;		\
		    return 0;					\
		}						\
	    } else {						\
		memcpy (head_buf, header, bytes);		\
		state = DEMUX_HEADER;				\
		state_bytes = bytes;				\
		return 0;					\
	    }							\
	}							\
    } while (0)

#define DONEBYTES(x)		\
    do {			\
	if (header != head_buf)	\
	    buf = header + (x);	\
    } while (0)

  if (flags & DEMUX_PAYLOAD_START)
    goto payload_start;
  switch (state)
  {
  case DEMUX_HEADER:
    if (state_bytes > 0)
    {
      header = head_buf;
      bytes = state_bytes;
      goto continue_header;
    }
    break;
  case DEMUX_DATA:
    if (mpeg_d->demux_pid || (state_bytes > end - buf))
    {
      memcpy ((unsigned char *) (mpeg_d->readbuf + sizeof (frame_ctrl_t) +
				 mpeg_d->readbuf_ptr), buf, end - buf);
      mpeg_d->readbuf_ptr += end - buf;
      state_bytes -= end - buf;
      return 0;
    }
    memcpy ((unsigned char *) (mpeg_d->readbuf + sizeof (frame_ctrl_t) +
			       mpeg_d->readbuf_ptr), buf, state_bytes);
    mpeg_d->readbuf_ptr += state_bytes;
    buf += state_bytes;
    break;
  case DEMUX_SKIP:
    if (mpeg_d->demux_pid || (state_bytes > end - buf))
    {
      state_bytes -= end - buf;
      return 0;
    }
    buf += state_bytes;
    break;
  }

  while (1)
  {
    if (mpeg_d->demux_pid)
    {
      state = DEMUX_SKIP;
      return 0;
    }
  payload_start:
    header = buf;
    bytes = end - buf;
  continue_header:
    NEEDBYTES (4);
    if (header[0] || header[1] || (header[2] != 1))
    {
      if (mpeg_d->demux_pid)
      {
	state = DEMUX_SKIP;
	return 0;
      }
      else if (header != head_buf)
      {
	buf++;
	goto payload_start;
      }
      else
      {
	header[0] = header[1];
	header[1] = header[2];
	header[2] = header[3];
	bytes = 3;
	goto continue_header;
      }
    }
    if (mpeg_d->demux_pid)
    {
      if ((header[3] >= 0xe0) && (header[3] <= 0xef))
	goto pes;
      LOG_Error ("bad stream id %x", header[3]);
      /*     exit (1); */
      return -L4_EUNKNOWN;
    }
    switch (header[3])
    {
    case 0xb9:			/* program end code */
      /* DONEBYTES (4); */
      /* break;         */
      return 1;
    case 0xba:			/* pack header */
      NEEDBYTES (5);
      if ((header[4] & 0xc0) == 0x40)
      {				/* mpeg2 */
	NEEDBYTES (14);
	len = 14 + (header[13] & 7);
	NEEDBYTES (len);
	DONEBYTES (len);
	/* header points to the mpeg2 pack header */
      }
      else if ((header[4] & 0xf0) == 0x20)
      {				/* mpeg1 */
	NEEDBYTES (12);
	DONEBYTES (12);
	/* header points to the mpeg1 pack header */
      }
      else
      {
	LOG_Error ("weird pack header");
	DONEBYTES (5);
      }
      break;
    default:
      if (header[3] == mpeg_d->demux_track)
      {
      pes:
	NEEDBYTES (7);
	if ((header[6] & 0xc0) == 0x80)
	{			/* mpeg2 */
	  NEEDBYTES (9);
	  len = 9 + header[8];
	  NEEDBYTES (len);
	  /* header points to the mpeg2 pes header */
	  if (header[7] & 0x80)
	  {
	    mpeg_d->pts = (((header[9] >> 1) << 30) |
			   (header[10] << 22) | ((header[11] >> 1) << 15) |
			   (header[12] << 7) | (header[13] >> 1));
//                      mpeg2_pts (mpeg2dec, pts);
	  }
	}
	else
	{			/* mpeg1 */
	  int len_skip;
	  unsigned char *ptsbuf;

	  len = 7;
	  while (header[len - 1] == 0xff)
	  {
	    len++;
	    NEEDBYTES (len);
	    if (len > 23)
	    {
	      LOG_Error ("too much stuffing");
	      break;
	    }
	  }
	  if ((header[len - 1] & 0xc0) == 0x40)
	  {
	    len += 2;
	    NEEDBYTES (len);
	  }
	  len_skip = len;
	  len += mpeg1_skip_table[header[len - 1] >> 4];
	  NEEDBYTES (len);
	  /* header points to the mpeg1 pes header */
	  ptsbuf = header + len_skip;
	  if (ptsbuf[-1] & 0x20)
	  {
	    mpeg_d->pts = (((ptsbuf[-1] >> 1) << 30) |
			   (ptsbuf[0] << 22) | ((ptsbuf[1] >> 1) << 15) |
			   (ptsbuf[2] << 7) | (ptsbuf[3] >> 1));
//                      mpeg2_pts (mpeg2dec, pts);
	  }
	}
	DONEBYTES (len);
	bytes = 6 + (header[4] << 8) + header[5] - len;
	if (mpeg_d->demux_pid || (bytes > end - buf))
	{
	  // fwrite (buf, end - buf, 1, stdout);
	  memcpy ((unsigned char *) (mpeg_d->readbuf + sizeof (frame_ctrl_t) +
				     mpeg_d->readbuf_ptr), buf, end - buf);
	  mpeg_d->readbuf_ptr += end - buf;
	  state = DEMUX_DATA;
	  state_bytes = bytes - (end - buf);
	  return 0;
	}
	else if (bytes > 0)
	{
	  //fwrite (buf, bytes, 1, stdout);
	  memcpy ((unsigned char *) (mpeg_d->readbuf + sizeof (frame_ctrl_t) +
				     mpeg_d->readbuf_ptr), buf, bytes);
	  mpeg_d->readbuf_ptr += bytes;
	  buf += bytes;
	}
      }
      else if (header[3] < 0xb9)
      {
	LOG_Error ("looks like a video stream, not system stream");
	DONEBYTES (4);
      }
      else
      {
	NEEDBYTES (6);
	DONEBYTES (6);
	bytes = (header[4] << 8) + header[5];
	if (bytes > end - buf)
	{
	  state = DEMUX_SKIP;
	  state_bytes = bytes - (end - buf);
	  return 0;
	}
	buf += bytes;
      }
    }
  }
}



static int
ps_loop (struct vid_mpeg_data *mpeg_d)
{
  int bytes_read = 0;
  mpeg_d->readbuf_ptr = 0;
  bytes_read = fileops_read (mpeg_d->fd, mpeg_d->buffer, BUFFER_SIZE);
  if (bytes_read <= 0)
  {
    LOGdL (DEBUG_IMPORT, "EOF.");
    return -1;			/* read nothing eof */
  }
  mpeg_d->end = mpeg_d->buffer + bytes_read;
  demux (mpeg_d->buffer, mpeg_d->end, 0, mpeg_d);
  return 0;			/* ok */
}

#if 0
/* code below is for demux transport streams - will be added soon... */
static int
pva_demux (unsigned char *buf, unsigned char *end)
{
  static int state = DEMUX_SKIP;
  static int state_bytes = 0;
  static unsigned char head_buf[12];

  unsigned char *header;
  int bytes;
  int len;

  switch (state)
  {
  case DEMUX_HEADER:
    if (state_bytes > 0)
    {
      header = head_buf;
      bytes = state_bytes;
      goto continue_header;
    }
    break;
  case DEMUX_DATA:
    if (state_bytes > end - buf)
    {
//            fwrite (buf, end - buf, 1, stdout);
#err
      state_bytes -= end - buf;
      return 0;
    }
    //fwrite (buf, state_bytes, 1, stdout);
#err
    buf += state_bytes;
    break;
  case DEMUX_SKIP:
    if (state_bytes > end - buf)
    {
      state_bytes -= end - buf;
      return 0;
    }
    buf += state_bytes;
    break;
  }

  while (1)
  {
  payload_start:
    header = buf;
    bytes = end - buf;
  continue_header:
    NEEDBYTES (2);
    if (header[0] != 0x41 || header[1] != 0x56)
    {
      if (header != head_buf)
      {
	buf++;
	goto payload_start;
      }
      else
      {
	header[0] = header[1];
	bytes = 1;
	goto continue_header;
      }
    }
    NEEDBYTES (8);
    if (header[2] != 1)
    {
      DONEBYTES (8);
      bytes = (header[6] << 8) + header[7];
      if (bytes > end - buf)
      {
	state = DEMUX_SKIP;
	state_bytes = bytes - (end - buf);
	return 0;
      }
      buf += bytes;
    }
    else
    {
      len = 8;
      if (header[5] & 0x10)
      {
	len = 12;
	NEEDBYTES (len);
      }
      DONEBYTES (len);
      bytes = (header[6] << 8) + header[7] + 8 - len;
      if (bytes > end - buf)
      {
	//fwrite (buf, end - buf, 1, stdout);
#error
	state = DEMUX_DATA;
	state_bytes = bytes - (end - buf);
	return 0;
      }
      else if (bytes > 0)
      {
	//fwrite (buf, bytes, 1, stdout);
#error
	buf += bytes;
      }
    }
  }
}

static void
pva_loop (void)
{
  unsigned char *end;

  do
  {
    end = buffer + fread (buffer, 1, BUFFER_SIZE, in_file);
    pva_demux (buffer, end);
  }
  while (end == buffer + BUFFER_SIZE);
}

static void
ts_loop (void)
{
  unsigned char *buf;
  unsigned char *nextbuf;
  unsigned char *data;
  unsigned char *end;
  int pid;

  buf = buffer;
  while (1)
  {
    end = buf + fread (buf, 1, buffer + BUFFER_SIZE - buf, in_file);
    buf = buffer;
    for (; (nextbuf = buf + 188) <= end; buf = nextbuf)
    {
      if (*buf != 0x47)
      {
	iprintf ("bad sync byte\n");
	nextbuf = buf + 1;
	continue;
      }
      pid = ((buf[1] << 8) + buf[2]) & 0x1fff;
      if (pid != demux_pid)
	continue;
      data = buf + 4;
      if (buf[3] & 0x20)
      {				/* buf contains an adaptation field */
	data = buf + 5 + buf[4];
	if (data > nextbuf)
	  continue;
      }
      if (buf[3] & 0x10)
	demux (data, nextbuf, (buf[1] & 0x40) ? DEMUX_PAYLOAD_START : 0);
    }
    if (end != buffer + BUFFER_SIZE)
      break;
    memcpy (buffer, buf, end - buf);
    buf = buffer + (end - buf);
  }
}
#endif

#if 0
int
main (int argc, char **argv)
{

  handle_args (argc, argv);

  if (demux_pva)
    pva_loop ();
  if (demux_pid)
    ts_loop ();
  else
    ps_loop ();

  return 0;
}
#endif



#if 0
static void
handle_args (int argc, char **argv)
{
  int c;
  char *s;

  while ((c = getopt (argc, argv, "hs:t:p")) != -1)
    switch (c)
    {
    case 's':
      demux_track = strtol (optarg, &s, 0);
      if (demux_track < 0xe0)
	demux_track += 0xe0;
      if (demux_track < 0xe0 || demux_track > 0xef || *s)
      {
	iprintf ("Invalid track number: %s\n", optarg);
	print_usage (argv);
      }
      break;

    case 't':
      demux_pid = strtol (optarg, &s, 0);
      if (demux_pid < 0x10 || demux_pid > 0x1ffe || *s)
      {
	iprintf ("Invalid pid: %s\n", optarg);
	print_usage (argv);
      }
      break;

    case 'p':
      demux_pva = 1;
      break;

    default:
      print_usage (argv);
    }

  if (optind < argc)
  {
    in_file = fopen (argv[optind], "rb");
    if (!in_file)
    {
      iprintf ("%s - could not open file %s\n", strerror (errno),
	       argv[optind]);
      exit (1);
    }
  }
  else
    in_file = stdin;
}
#endif


/* audio functions - not implemented yet*/
int
aud_import_mpg_init (plugin_ctrl_t * attr, stream_info_t * info)
{
  return -L4_ENOTSUPP;
}

int
aud_import_mpg_commit (plugin_ctrl_t * attr)
{
  return -L4_ENOTSUPP;
}

int
aud_import_mpg_step (plugin_ctrl_t * attr, void *addr)
{
  return -L4_ENOTSUPP;
}

int
aud_import_mpg_close (plugin_ctrl_t * attr)
{
  return -L4_ENOTSUPP;
}

int
aud_import_mpg_seek (plugin_ctrl_t * attr, void *addr, double position,
		     int whence)
{
  return -L4_ENOTSUPP;
}
