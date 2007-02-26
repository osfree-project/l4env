/*
  ogmlib -- a replacement of avilib based on OGM container format
  
      ogmlib.c
      
      Based on ogmtools Written by Moritz Bunkus <moritz@bunkus.org>
      See http://www.bunkus.org/videotools/ogmtools/
      Based on Xiph.org's 'oggmerge' found in their CVS repository
      See http://www.xiph.org
      
      Distributed under the GPL
      see the file COPYING for details
      or visit http://www.gnu.org/copyleft/gpl.html
*/



/* std/L4 */
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* ogg stuff */
#include "ogg/ogg.h"
#include "ogmlib.h"
#include "ogmstreams.h"
#include "common.h"

/* mp3 stuff */
#include "mpg123.h"

/* file reading */
#include "drops-compat.h"

/* printf for floating point */
#warning Remove these lines when printf can do floating point output!
#undef printf
#define printf cr7_printf
#include "cr7libc.h"

/* 
 * shortcut defines
 */
#define BLOCK_SIZE 4096
#define MAX_AUDIO_FRAME_SIZE 11052

#define ISSYNCPOINT ((pack->packet[0] & PACKET_IS_SYNCPOINT) ? \
         1 : 0)

#define OUTOFSYNC(a,b)   (((a - b) < 0.0 ) ? \
                     "OUT_OF_SYNC " : "sync_ok ")

/*
 * well known formats 
 */
#define ACVORBIS 0xffff
#define ACPCM    0x0001
#define ACMP3    0x0055
#define ACAC3    0x2000



/*
 * internal helper functions below 
 */


/*
 *  prototypes 
 */
void dump_streamheader (stream_header * sth);
stream_t *find_stream (int fserial, ogg_t * OGG);
void add_stream (stream_t * ndmx, ogg_t * OGG);
int handle_packet (stream_t * stream, ogg_packet * pack, ogg_page * page,
		   ogg_t * OGG, char *buf, int *keyframe, int *duration);
int read_page (ogg_t * OGG, ogg_sync_state * sync, ogg_page * page,
	       unsigned long *fpos);


/*
 * dump info of ogm-streamheader
 */
void
dump_streamheader (stream_header * sth)
{
  char streamtype[9], subtype[5];
  memcpy (streamtype, sth->streamtype, 8);
  streamtype[8] = 0;
  memcpy (subtype, sth->subtype, 4);
  subtype[4] = 0;
  printf ("Full stream_header dump:\n {"
	  "streamtype = \"%s\", subtype = \"%s\", size = %lu,\n "
	  "time_unit = %lld, samples_per_unit = %lld, default_len = %lu,\n "
	  "buffersize = %lu, bits_per_sample = %d,\n "
	  "sh = {\n   video = { width = %lu, height = %lu}\n   audio = { "
	  "channels = %d, blockalign = %d, avgbytespersec = %lu }\n }\n",
	  streamtype, subtype, get_uint32 (&sth->size),
	  get_uint64 (&sth->time_unit),
	  get_uint64 (&sth->samples_per_unit), get_uint32 (&sth->default_len),
	  get_uint32 (&sth->buffersize),
	  get_uint16 (&sth->bits_per_sample),
	  get_uint32 (&sth->sh.video.width),
	  get_uint32 (&sth->sh.video.height),
	  get_uint16 (&sth->sh.audio.channels),
	  get_uint16 (&sth->sh.audio.blockalign),
	  get_uint32 (&sth->sh.audio.avgbytespersec));
}


/*
 * find stream in list of existing streams of ogg_t 
 */
stream_t *
find_stream (int fserial, ogg_t * OGG)
{
  stream_t *cur = OGG->first;
  if (fserial == -1)
    return NULL;
  while (cur != NULL)
  {
    if (cur->serial == fserial)
      return cur;
    cur = cur->next;
  }
  return NULL;
}


/*
 * add a stream into list of existing streams
 */
void
add_stream (stream_t * ndmx, ogg_t * OGG)
{
  stream_t *cur = OGG->first;

  if (OGG->first == NULL)
  {
    OGG->first = ndmx;
    OGG->first->next = NULL;
  }
  else
  {
    cur = OGG->first;
    while (cur->next != NULL)
      cur = cur->next;
    cur->next = ndmx;
    ndmx->next = NULL;
  }
}


/*
 * handle packet
 * extract chunks/frames from OGM-packet and sets usefull infos
 */
int
handle_packet (stream_t * stream, ogg_packet * pack, ogg_page * page,
	       ogg_t * OGG, char *buf, int *keyframe, int *duration)
{
  int i, hdrlen;
  long long lenbytes;
  int pos = 0;
  unsigned long header;
  mp3_header_t mp3header;

  if (pack->e_o_s)
  {
    stream->eos = 1;
    pack->e_o_s = 1;
  }

  hdrlen = (*pack->packet & PACKET_LEN_BITS01) >> 6;
  hdrlen |= (*pack->packet & PACKET_LEN_BITS2) << 1;
  for (i = 0, lenbytes = 0; i < hdrlen; i++)
  {
    lenbytes = lenbytes << 8;
    lenbytes += *((unsigned char *) pack->packet + hdrlen - i);
  }
  if (lenbytes == 0)
    lenbytes = stream->default_len;	//! Anzahl der Samples oder Frames !
  *duration = 0;		//wird bei audio (ausser vorbis) ausgesetzt

  switch (stream->stype)
  {
  case 'v':
    /* video stream */
    if (((*pack->packet & 3) == PACKET_TYPE_HEADER) ||
	((*pack->packet & 3) == PACKET_TYPE_COMMENT))
      return -1;

    /* copy data into output buffer */
    stream->vi.size += (u_int64_t) pack->bytes;
    *keyframe = ISSYNCPOINT;
    i = pack->bytes - 1 - hdrlen;
    if (buf != NULL)
      memcpy (buf, (char *) &pack->packet[hdrlen + 1], i);
    /* setup timestamps */
    OGG->end_pts = (double) ogg_page_granulepos (page) *
      (double) 1000.0 / (double) stream->sample_rate;
    OGG->start_pts = (double) stream->last_granulepos *
      (double) 1000.0 / (double) stream->sample_rate;
#if OGMLIB_VERBOSE_PACKET
    printf ("V: start: % 7.2fms  end: % 7.2fms last: % 7.2fms %s ",
	    OGG->start_pts, OGG->end_pts, stream->last_pts,
	    OUTOFSYNC (OGG->start_pts, stream->last_pts));
    printf ("granulepos: % 10lld last: % 10lld ", ogg_page_granulepos (page),
	    stream->last_granulepos);
    printf ("pno: % 10lld  \n", pack->packetno);
#endif
    stream->last_pts = OGG->start_pts;

    /* got some bytes */
    return i;

    break;

  case 't':
    /* currently text streams are ignored! */
    return -1;
    break;

  case 'a':
    /* audio stream */
    *keyframe = 1;		/* audio is always a sync-point */
    switch (stream->acodec)
    {
    case ACVORBIS:
      /* OGG Vorbis */
      stream->ai.size += (u_int64_t) pack->bytes;
      stream->packetno++;
      if (stream->packetno == 0)
      {
	i = pack->bytes;
	if (buf)
	  memcpy (buf, pack->packet, i);
      }
      else
      {
	i = pack->bytes - 1;
	if (buf)
	  memcpy (buf, &pack->packet[1], i);
      }

      /* update timestamps */
      OGG->end_pts = (double) ogg_page_granulepos (page) *
	(double) 1000.0 / (double) stream->sample_rate;
      OGG->start_pts = (double) stream->last_granulepos *
	(double) 1000.0 / (double) stream->sample_rate;
#if OGMLIB_VERBOSE_PACKET
      printf ("A: start: % 7.2fms  end: % 7.2fms last: % 7.2fms %s ",
	      OGG->start_pts, OGG->end_pts, stream->last_pts,
	      OUTOFSYNC (OGG->start_pts, stream->last_pts));
      printf ("granulepos: % 10lld last: % 10lld ",
	      ogg_page_granulepos (page), stream->last_granulepos);
      printf ("pno: % 10lld  \n", pack->packetno);
#endif
      stream->last_pts = OGG->start_pts;
      /* ok */
      return i;
      break;
    case ACMP3:
      /* MP3 */
      if (((*pack->packet & 3) == PACKET_TYPE_HEADER) ||
	  ((*pack->packet & 3) == PACKET_TYPE_COMMENT))
	return -1;
      /* we search an mp3-header to get mp3-frame-length */
      stream->ai.size += (u_int64_t) pack->bytes;
      i = pack->bytes - 1 - hdrlen;
      /* search header */
      pos = find_mp3_header (pack->packet + hdrlen + 1, i, &header);
      if (pos < 0)
      {
#if OGMLIB_VERBOSE_PACKET
	printf ("ogmlib.c mp3 - no header found ! EOS ?\n");
#endif
	return -1;
      }
      decode_mp3_header (header, &mp3header);
      *duration = mp3header.framesize + 4;
      stream->ai.mp3rate = mp3_tabsel[mp3header.lsf][mp3header.bitrate_index];
      /* update timestamp */
      if (stream->sample_rate != -1)
      {
	OGG->end_pts = (double) ogg_page_granulepos (page) *
	  (double) 1000.0 / (double) stream->sample_rate;
	OGG->start_pts = (double) stream->last_granulepos *
	  (double) 1000.0 / (double) stream->sample_rate;
#if OGMLIB_VERBOSE_PACKET
	printf ("A: start: % 7.2fms  end: % 7.2fms last: % 7.2fms %s ",
		OGG->start_pts, OGG->end_pts, stream->last_pts,
		OUTOFSYNC (OGG->start_pts, stream->last_pts));
	printf ("granulepos: % 10lld last: % 10lld ",
		ogg_page_granulepos (page), stream->last_granulepos);
	printf ("pno: % 10lld  \n", pack->packetno);
#endif
	stream->last_pts = OGG->start_pts;
      }

      *keyframe = 1;		/* audio is always a sync point */
      if (buf != NULL)
	memcpy (buf, pack->packet + hdrlen + 1, i);
      return i;
      break;
    default:
      /* neither Vorbis nor MP3, but audio */
      if (((*pack->packet & 3) == PACKET_TYPE_HEADER) ||
	  ((*pack->packet & 3) == PACKET_TYPE_COMMENT))
	return -1;
      /* timing */
      stream->ai.size += (u_int64_t) pack->bytes;
      if (stream->sample_rate != -1)
      {
	OGG->end_pts = (double) ogg_page_granulepos (page) *
	  (double) 1000.0 / (double) stream->sample_rate;
	OGG->start_pts = (double) stream->last_granulepos *
	  (double) 1000.0 / (double) stream->sample_rate;
#if OGMLIB_VERBOSE_PACKET
	printf ("?: start: % 7.2fms  end: % 7.2fms last: % 7.2fms %s ",
		OGG->start_pts, OGG->end_pts, stream->last_pts,
		OUTOFSYNC (OGG->start_pts, stream->last_pts));
	printf ("granulepos: % 10lld last: % 10lld ",
		ogg_page_granulepos (page), stream->last_granulepos);
	printf ("pno: % 10lld  \n", pack->packetno);
#endif
	stream->last_pts = OGG->start_pts;
      }

      *keyframe = 1;		/* audio is always a sync point */
      i = pack->bytes - 1 - hdrlen;
      /* copy data */
      if (buf != NULL)
	memcpy (buf, pack->packet + hdrlen + 1, i);
      return i;
      break;
    }
  }
  return -1;
}




/*
 * general functions below 
 */



/*
 * open OGM file 
 */
ogg_t *
OGG_open_input_file (char *filename, int getIndex)
{

  int duration, keyframe, nread, np;
  char *buf;
  unsigned long codec;
  int sno;			//current stream no
  int i;
  /* init ogg structure */
  ogg_t *OGG = NULL;
  OGG = (ogg_t *) malloc (sizeof (ogg_t));
  if (OGG == NULL)
  {
    printf ("ogmlib.c: failed to malloc !");
    return NULL;
  }

  /* init some vars */
  memset (OGG, 0, sizeof (ogg_t));
  OGG->video_serial = -1;
  OGG->audio_serial = -1;
  OGG->streaming = 0;
  for (i = 0; i < MAX_STREAMS_TO_BUFFER; i++)
    OGG->buffered_streams[i] = -1;
  /* build ogg-sync-stuff */
  OGG->sync = (ogg_sync_state *) malloc (sizeof (ogg_sync_state));
  if (OGG->sync == NULL)
  {
    printf ("ogmlib.c: failed to malloc !");
    free (OGG);
    return NULL;
  }

  /* open file */
  OGG->fdes = open (filename, 0);
  if (OGG->fdes < 0)
  {
    printf ("openening input failed !\n");
    ogg_sync_destroy (OGG->sync);
    free (OGG);
    return NULL;
  }

  /* init ogg-syncer */
  ogg_sync_init (OGG->sync);
  /* 
   * run through file to find all streams 
   */
  while (1)
  {
    np = ogg_sync_pageseek (OGG->sync, &OGG->page);
    /* error */
    if (np < 0)
    {
      printf ("ogmlib.c: (%s) ogg_sync_pageseek failed\n", filename);
      free (OGG);
      ogg_sync_destroy (OGG->sync);
      return NULL;
    }
    /* need more data */
    if (np == 0)
    {
      buf = ogg_sync_buffer (OGG->sync, BLOCK_SIZE);
      if (!buf)
      {
	printf ("ogmlib.c: (%s) ogg_sync_buffer failed\n", filename);
	ogg_sync_destroy (OGG->sync);
	free (OGG);
	return NULL;
      }
      /* read more */
      if ((nread = read (OGG->fdes, buf, BLOCK_SIZE)) <= 0)
      {
	printf ("ogmlib.c: (%s) end of stream\n", filename);
      }
      ogg_sync_wrote (OGG->sync, nread);
      continue;
    }

    /* found the BOS (begin of stream)? */
    if (!ogg_page_bos (&OGG->page))
    {
      /* now abort cause all BOSses should be found at the beginning of stream */
      break;
    }
    else
    {
      ogg_stream_state sstate;
      /* found bos - inspect it */
#if OGMLIB_VERBOSE
      printf ("ogmlib.c: (%s) found the bos\n", filename);
#endif
      sno = ogg_page_serialno (&OGG->page);
      if (ogg_stream_init (&sstate, sno))
      {
	printf ("ogmlib.c: (%s) ogg_stream_init failed\n", filename);
	free (OGG);
	ogg_sync_destroy (OGG->sync);
	return NULL;
      }
      ogg_stream_pagein (&sstate, &OGG->page);
      ogg_stream_packetout (&sstate, &OGG->pack);
      /* found Vorbis */
      if ((OGG->pack.bytes >= 7)
	  && !strncmp (&OGG->pack.packet[1], "vorbis", 6))
      {
	printf ("ogmlib.c: (%s) (a%d/%d) Vorbis audio stream\n",
		filename, OGG->nastreams + 1, OGG->numstreams + 1);
	OGG->stream = (stream_t *) malloc (sizeof (stream_t));
	if (OGG->stream == NULL)
	{
	  printf ("ogmlib.c: malloc failed.\n");
	  ogg_sync_destroy (OGG->sync);
	  free (OGG);
	  return NULL;
	}
	memset (OGG->stream, 0, sizeof (stream_t));
	OGG->stream->sno = OGG->nastreams + 1;
	OGG->stream->stype = 'a';
	OGG->stream->sample_rate = -1;
	OGG->stream->serial = sno;
	OGG->stream->acodec = ACVORBIS;
	memcpy (&OGG->stream->instate, &sstate, sizeof (sstate));
	/* handle complete packet */
	{
	  do
	    handle_packet (OGG->stream, &OGG->pack, &OGG->page, OGG, NULL,
			   &keyframe, &duration);
	  while (ogg_stream_packetout (&OGG->stream->instate, &OGG->pack)
		 == 1);
	}
	add_stream (OGG->stream, OGG);
	OGG->nastreams++;
	OGG->numstreams++;
	/* as default first audio stream is current audio stream */
	if (OGG->nastreams == 1)
	  OGG->audio_serial = sno;
      }				/* end vorbis */
      else
	if (((*OGG->pack.packet & PACKET_TYPE_BITS) ==
	     PACKET_TYPE_HEADER)
	    && (OGG->pack.bytes >= (int) (sizeof (stream_header) + 1)))
      {
	stream_header *sth = (stream_header *) (OGG->pack.packet + 1);
#if OGMLIB_VERBOSE
	dump_streamheader (sth);
#endif
	/* found a video stream */
	if (!strncasecmp (sth->streamtype, "video", 5))
	{
	  char ccodec[5];
	  codec = (sth->subtype[0] << 24) +
	    (sth->subtype[1] << 16) + (sth->subtype[2] << 8) +
	    sth->subtype[3];
	  ccodec[0] = sth->subtype[0];
	  ccodec[1] = sth->subtype[1];
	  ccodec[2] = sth->subtype[2];
	  ccodec[3] = sth->subtype[3];
	  ccodec[4] = 0;
#if OGMLIB_VERBOSE
	  printf
	    ("ogmlib.c: (%s) (v%d/%d) fps: %.3f width height: %lux%lu "
	     "codec: %p (%s)\n", filename, OGG->nvstreams + 1,
	     OGG->numstreams + 1,
	     (double) 10000000 / (double) get_uint64 (&sth->time_unit),
	     get_uint32 (&sth->sh.video.width),
	     get_uint32 (&sth->sh.video.height), (void *) codec, ccodec);
#endif
	  OGG->stream = (stream_t *) malloc (sizeof (stream_t));
	  if (OGG->stream == NULL)
	  {
	    printf ("ogmlib.c: malloc failed.\n");
	    ogg_sync_destroy (OGG->sync);
	    free (OGG);
	    return NULL;
	  }
	  memset (OGG->stream, 0, sizeof (stream_t));
	  OGG->stream->default_len = sth->default_len;
	  OGG->stream->stype = 'v';
	  OGG->stream->serial = sno;
	  OGG->stream->sample_rate =
	    (double) 10000000 / (double) get_uint64 (&sth->time_unit);
	  OGG->stream->sno = OGG->nvstreams + 1;
	  memcpy (&OGG->stream->instate, &sstate, sizeof (sstate));
	  /* handle first packet completly */
	  {
	    do
	      handle_packet (OGG->stream, &OGG->pack, &OGG->page, OGG, NULL,
			     &keyframe, &duration);
	    while (ogg_stream_packetout (&OGG->stream->instate, &OGG->pack)
		   == 1);
	  }
	  add_stream (OGG->stream, OGG);
	  OGG->nvstreams++;
	  OGG->numstreams++;
	  /* as default first video stream is current video stream */
	  if (OGG->nvstreams == 1)
	    OGG->video_serial = sno;
	  /* remember data */
	  OGG->stream->vi.width = get_uint32 (&sth->sh.video.width);
	  OGG->stream->vi.height = get_uint32 (&sth->sh.video.height);
	  memcpy (OGG->stream->vi.compressor, ccodec, 4);
	}
	/* found an audio stream */
	else if (!strncmp (sth->streamtype, "audio", 5))
	{
	  int codec;
	  char buf[5];
	  memcpy (buf, sth->subtype, 4);
	  buf[4] = 0;
	  codec = strtoul (buf, NULL, 16);
#if OGMLIB_VERBOSE
	  printf ("ogmlib.c: (%s) (a%d/%d) codec: %d (0x%04x) (%s), bps: "
		  "%d channels: %hd  samples per second: %lld\n",
		  filename, OGG->nastreams + 1, OGG->numstreams + 1,
		  codec, codec,
		  codec == ACPCM ? "PCM" : codec == 55 ? "MP3" : codec ==
		  ACMP3 ? "MP3" : codec == ACAC3 ? "AC3" : "unknown",
		  get_uint16 (&sth->bits_per_sample),
		  get_uint16 (&sth->sh.audio.channels),
		  get_uint64 (&sth->samples_per_unit));
	  printf ("ogmlib.c:  avgbytespersec: %lu blockalign: %d\n",
		  get_uint32 (&sth->sh.audio.avgbytespersec),
		  get_uint16 (&sth->sh.audio.blockalign));
#endif
	  OGG->stream = (stream_t *) malloc (sizeof (stream_t));
	  if (OGG->stream == NULL)
	  {
	    printf ("ogmlib.c: malloc failed.\n");
	    ogg_sync_destroy (OGG->sync);
	    free (OGG);
	    return NULL;
	  }
	  memset (OGG->stream, 0, sizeof (stream_t));
	  OGG->stream->sno = OGG->nastreams + 1;
	  OGG->stream->stype = 'a';
	  OGG->stream->sample_rate = get_uint64 (&sth->samples_per_unit);
	  OGG->stream->default_len = sth->default_len;
	  OGG->stream->serial = sno;
	  OGG->stream->acodec = codec;
	  memcpy (&OGG->stream->instate, &sstate, sizeof (sstate));
	  /* again - handle complete packet */
	  {
	    do
	      handle_packet (OGG->stream, &OGG->pack, &OGG->page, OGG, NULL,
			     &keyframe, &duration);
	    while (ogg_stream_packetout (&OGG->stream->instate, &OGG->pack)
		   == 1);
	  }
	  add_stream (OGG->stream, OGG);
	  OGG->nastreams++;
	  OGG->numstreams++;
	  /* as default first audio stream is current audio stream */
	  if (OGG->nastreams == 1)
	    OGG->audio_serial = sno;
	  /* remember infos */
	  OGG->stream->ai.a_fmt = codec;
	  OGG->stream->ai.a_bits = get_uint16 (&sth->bits_per_sample);
	  OGG->stream->ai.a_chans = get_uint16 (&sth->sh.audio.channels);
	  memcpy (OGG->stream->ai.audio_tag,
		  codec == ACPCM ? "PCM " :
		  codec == 55 ? "MP3 " :
		  codec == ACMP3 ? "MP3 " :
		  codec == ACAC3 ? "AC3 " : "unknown", 4);
	}
	/* found a text stream */
	else if (!strncmp (sth->streamtype, "text", 4))
	{
	  printf ("ogmlib.c: (%s) (t%d/%d) text/subtitle stream\n",
		  filename, OGG->ntstreams + 1, OGG->numstreams + 1);
	  OGG->stream = (stream_t *) malloc (sizeof (stream_t));
	  if (OGG->stream == NULL)
	  {
	    printf ("ogmlib.c: malloc failed.\n");
	    ogg_sync_destroy (OGG->sync);
	    free (OGG);
	    return NULL;
	  }
	  OGG->stream->sno = OGG->ntstreams + 1;
	  OGG->stream->stype = 't';
	  OGG->stream->sample_rate =
	    (double) 10000000 / (double) get_uint64 (&sth->time_unit);
	  OGG->stream->serial = sno;
	  memcpy (&OGG->stream->instate, &sstate, sizeof (sstate));
	  /* handle complete packet */
	  {
	    do
	      handle_packet (OGG->stream, &OGG->pack, &OGG->page, OGG, NULL,
			     &keyframe, &duration);
	    while (ogg_stream_packetout (&OGG->stream->instate, &OGG->pack)
		   == 1);
	  }
	  add_stream (OGG->stream, OGG);
	  OGG->ntstreams++;
	  OGG->numstreams++;
	}
	else
	{
	  printf ("ogmlib.c: (%s) (%d) found new header of unknown/"
		  "unsupported type\n", filename, OGG->numstreams + 1);
	}

      }
      /* unknown stream */
      else
      {
	printf ("ogmlib.c: (%s) OGG stream %d is of an unknown type "
		"(bad header?)\n", filename, OGG->numstreams + 1);
      }

    }				/* end found bos */

  }				/* end while(1) */

  /* should we find out bitrate of streams ? */
  if (getIndex != 0)
  {
    int i, keyframe;
#if OGMLIB_VERBOSE
    printf
      ("ogmlib.c: get index, this may take a while for %i video, %i audio stream(s).\n",
       OGG->nvstreams, OGG->nastreams);
#endif
    /* read all video streams */
    for (i = 0; i < OGG->nvstreams; i++)
    {
      int framecnt = 0;
      /* next stream */
      if (OGG_set_video_track (OGG, i + 1) != 0)
	break;
      while (OGG_read_frame (OGG, NULL, &keyframe) != -1)
      {
	/* just count all frames */
	framecnt++;
      }
      /* calculate statistiks and remember */
      OGG->stream->vi.frames = framecnt;
      OGG->stream->vi.highest_pts = OGG->end_pts;
      OGG->stream->vi.bytes = OGG->stream->vi.size;
      OGG->stream->vi.size = 0;
      printf ("ogmlib.c: (%s) (%d) %2.2f ms. %i frames. %i bytes.\n",
	      filename, i + 1, OGG->stream->vi.highest_pts,
	      (int) framecnt, (int) OGG->stream->vi.bytes);
      printf ("ogmlib.c: (%s) (%d) %.3f kbit/s = %.3f KB/s\n",
	      filename, i + 1,
	      (OGG->stream->vi.bytes * 8.0) /
	      OGG->stream->vi.highest_pts,
	      (OGG->stream->vi.bytes * 1000 / 1024.0) /
	      OGG->stream->vi.highest_pts);
      /* seek to begining of ogm */
      OGG_seek_start (OGG);
    }

    /* read all audio streams */
    for (i = 0; i < OGG->nastreams; i++)
    {
      unsigned char aud_buf[MAX_AUDIO_FRAME_SIZE];
      u_int64_t size = 0;
      int chunkcnt = 0;
      /* next stream */
      if (OGG_set_audio_track (OGG, i + 1) != 0)
	break;
      while (OGG_read_audio_chunk (OGG, aud_buf) != -1)
      {
	/* just count all chunks and remember size */
	chunkcnt++;
	size = OGG->stream->ai.size;
      }
      /* calculate statistiks and remember */
      OGG->stream->ai.chunks = chunkcnt;
      OGG->stream->ai.highest_pts = OGG->stream->last_pts;
      OGG->stream->ai.bytes = size;
      OGG->stream->ai.size = 0;
      printf ("ogmlib.c: (%s) (%d) %2.2f ms. %i chunks. %i bytes.\n",
	      filename, i + 1, OGG->stream->ai.highest_pts,
	      (int) chunkcnt, (int) OGG->stream->ai.bytes);
      printf ("ogmlib.c: (%s) (%d) %.3f kbit/s = %.3f KB/s\n",
	      filename, i + 1,
	      (OGG->stream->ai.bytes * 8.0) /
	      OGG->stream->ai.highest_pts,
	      (OGG->stream->ai.bytes * 1000 / 1024.0) /
	      OGG->stream->ai.highest_pts);
      /* seek to begining of ogm */
      OGG_seek_start (OGG);
    }

  }				//end getIndex != 0

  /* reset some vars to zero (changed by getIndex) */
  OGG->end_pts = OGG->start_pts = 0;
  /* done */
#if OGMLIB_VERBOSE
  printf ("ogmlib.c: open done.\n");
#endif
  return OGG;
}



/*
 * close OGM and release ressources
 */
int
OGG_close (ogg_t * OGG)
{
  if (OGG != NULL)
  {
    /* streams free */
    stream_t *cur = OGG->first;
    stream_t *del;
    while (cur != NULL)
    {
      del = cur;
      cur = cur->next;
      /* flush buffer */
      if (del->buffer_elem)
	OGG_streaming_flush_buffer (OGG, 1, del->serial);
      /* stream clear */
      ogg_stream_clear (&del->instate);
      /* free */
      free (del);
    }
    /* free sync */
    ogg_sync_destroy (OGG->sync);
#if OGMLIB_VERBOSE
    printf ("ogmlib.c: (fdes %i) closed.\n", OGG->fdes);
#endif
    /* ile close */
    close (OGG->fdes);
    /* struct free */
    if (OGG != NULL)
      free (OGG);
  }
  /* done */
  OGG = NULL;
  return 0;
}


/*
 * seek to beginning of OGM
 */
int
OGG_seek_start (ogg_t * OGG)
{
  /* setting everthing to zero ms / granulepos */
  stream_t *cur = OGG->first;
  while (cur != NULL)
  {
    cur->last_granulepos = cur->this_granulepos = 0;
    cur->eos = 0;
    cur->ai.size = 0;
    cur->vi.size = 0;
    /* set fpos at start for next read */
    cur->fpos = 0;
    /* set last pts */
    cur->last_pts = 0.00;
    /* flush buffer */
    if (cur->buffer_elem)
      OGG_streaming_flush_buffer (OGG, 1, cur->serial);
    /* process next stream */
    cur = cur->next;
  }
  /* seek now - also in streaming mode, cause it's an explicit call */
  lseek (OGG->fdes, 0, SEEK_SET);
  if (OGG->streaming)
    printf ("ogmlib.c: Warning: lseek in streaming mode!\n");
  /* reset timestamps */
  OGG->start_pts = OGG->end_pts = 0;
  /* reset sync */
  ogg_sync_reset (OGG->sync);
#if OGMLIB_VERBOSE
  printf ("ogmlib.c: (fdes %i) seeked to start.\n", OGG->fdes);
#endif
  return 0;
}




/*
 * frame/chunk-reader functions below 
 */



/*
 * dry to fetch a complete OGM-page
 */
int
read_page (ogg_t * OGG, ogg_sync_state * sync, ogg_page * page,
	   unsigned long *fpos)
{
  char *buf;
  int nread;
  /* seek only if necessary and !streaming */
  if ((!OGG->streaming) && (*fpos != OGG->fpos))
    lseek (OGG->fdes, *fpos, SEEK_SET);
  while (ogg_sync_pageseek (sync, page) <= 0)
  {
    /* how many bytes do we need for this page */
    buf = ogg_sync_buffer (sync, BLOCK_SIZE);
    /* read from file */
    nread = read (OGG->fdes, buf, BLOCK_SIZE);
    /* error or EOS */
    if (nread <= 0)
    {
      OGG->stream = OGG->first;
#if OGMLIB_VERBOSE
      printf ("ogmlib.c: (fdes %i) end of stream\n", OGG->fdes);
#endif
      return -1;
    }
    else
      ogg_sync_wrote (sync, nread);
    /* increment file position */
    *fpos = *fpos + nread;
  }
  /* ok */
  OGG->fpos = *fpos;
  return 0;
}


/*
 * read one video frame 
 */
long
OGG_read_frame (ogg_t * OGG, char *vidbuf, int *keyframe)
{
  /* read data */
  return OGG_streaming_read_data (OGG, OGG->video_serial, vidbuf, keyframe);
}


/*
 * read one audio chunk
 */
long
OGG_read_audio_chunk (ogg_t * OGG, char *audbuf)
{
  int keyframe;
  /* read data */
  return OGG_streaming_read_data (OGG, OGG->audio_serial, audbuf, &keyframe);
}






/*
 * streaming (seek-free) functions follow 
 */

/*
 * open input file as OGM for streaming
 * default is: buffer nothing !!!
 * returns NULL on error
 */
ogg_t *
OGG_streaming_open_input_file (char *filename)
{

  ogg_t *OGG = OGG_open_input_file (filename, 0 /* no index read */ );
  if (OGG == NULL)
    return NULL;
  /* set streaming */
  OGG->streaming = 1;
  /* ok */
  return OGG;
}


/*
 * set streams w/ given serial_no to buffer data (audio chunks/video frames)
 * until they were read or get flushed
 * count - number of following serial_no
 * serial_no - to buffer
 */
int
OGG_streaming_setup_buffer (ogg_t * OGG, int buffer_serial_no, ...)
{
  va_list list;
  int arg, i, added;
  if (buffer_serial_no)
  {
    va_start (list, buffer_serial_no);
    while (buffer_serial_no--)
    {
      arg = va_arg (list, int);
      /* add serial_no to buffer list */
      added = 0;
      for (i = 0; i < MAX_STREAMS_TO_BUFFER; i++)
      {
	if (OGG->buffered_streams[i] == -1)
	{
	  /* found one free */
	  OGG->buffered_streams[i] = arg;
	  added = 1;
#if OGMLIB_VERBOSE_BUFFER
	  printf ("ogmlib.c: added %i to buffer list\n", arg);
#endif
	  break;
	}
      }
      if (!added)
      {
	printf ("ogmlib.c: Maximum of streams to buffer reached!\n");
	va_end (list);
	return -1;
      }

    }
    va_end (list);
  }
  return 0;
}


/*
 * flush buffered data for streams with given serial_no
 */
int
OGG_streaming_flush_buffer (ogg_t * OGG, int buffer_serial_no, ...)
{
  va_list list;
  int arg;
  buffer_elem_t *buffer_elem;
  buffer_elem_t *next_buffer_elem;
  if (buffer_serial_no)
  {
    va_start (list, buffer_serial_no);
    while (buffer_serial_no--)
    {
      arg = va_arg (list, int);
      /* flush buffer for given serials */
#if OGMLIB_VERBOSE_BUFFER
      printf ("ogmlib.c: flush buffer for serial %i\n", arg);
#endif
      /* find stream */
      OGG->stream = find_stream (arg, OGG);
      if (OGG->stream == NULL)
	break;
      /* remove all buffer elems */
      buffer_elem = OGG->stream->buffer_elem;
      while (buffer_elem != NULL)
      {
	/* remember successor */
	next_buffer_elem = buffer_elem->next_elem;
	/* free data */
	free (buffer_elem->data_packet);
	/* free buffer entry */
	free (buffer_elem);
	/* next one */
	buffer_elem = next_buffer_elem;
      }
      /* set buffer to free */
      OGG->stream->buffer_elem = NULL;
      /* update stats */
      OGG->stream->buffer_packets_current = 0;
      OGG->stream->buffer_bytes_current = 0;
    }
    va_end (list);
  }
  return 0;
}


/*
 * remove streams with given serial_no from buffer list
 */
int
OGG_streaming_remove_buffer (ogg_t * OGG, int buffer_serial_no, ...)
{
  va_list list;
  int arg, i;
  if (buffer_serial_no)
  {
    va_start (list, buffer_serial_no);
    while (buffer_serial_no--)
    {
      arg = va_arg (list, int);
      /* flush buffer for serial_no */
      OGG_streaming_flush_buffer (OGG, 1, arg);
      /* remove serial_no from buffer list */
      for (i = 0; i < MAX_STREAMS_TO_BUFFER; i++)
      {
	if (OGG->buffered_streams[i] == arg)
	{
	  /* found one to remove */
	  OGG->buffered_streams[i] = -1;
#if OGMLIB_VERBOSE_BUFFER
	  printf ("ogmlib.c: removed %i from buffer list\n", arg);
#endif
	}
      }
    }
    va_end (list);
  }
  return 0;
}


/*
 * read data (audio or video) from buffer,
 * buffer miss means reading from stream
 */
long
OGG_streaming_read_buffered_data (ogg_t * OGG, int serial_no,
				  char *buffer, int *keyframe)
{
  long size = -1;
  int i;
  /* buffer elements */
  buffer_elem_t *buffer_elem;
  buffer_elem_t *last_buffer_elem;
  /* stream_t for buffer */
  stream_t *buffer_stream;
  /* find stream */
  OGG->stream = find_stream (serial_no, OGG);
  if (OGG->stream == NULL)
  {
#if OGMLIB_VERBOSE
    printf
      ("ogmlib.c: (fdes %i) Encountered packet for an unknown serial "
       "%d !?\n", OGG->fdes, serial_no);
#endif
    return -1;
  }

  /* buffer lookup */
  if (OGG->stream->buffer_elem != NULL)
  {
    /* something in buffer - get it */
    buffer_elem = OGG->stream->buffer_elem;
    /* size */
    size = OGG->stream->buffer_elem->data_size;
#if OGMLIB_VERBOSE_BUFFER
    printf
      ("ogmlib.c: Hehe. Got buffered data for serial no %i (size %lu)\n",
       serial_no, size);
#endif
    /* copy data */
    memcpy (buffer, OGG->stream->buffer_elem->data_packet, size);
    /* free data buffer */
    free (OGG->stream->buffer_elem->data_packet);
    /* keyframe */
    *keyframe = OGG->stream->buffer_elem->keyframe;
    /* set timestamps */
    OGG->end_pts = OGG->stream->buffer_elem->end_pts;
    OGG->start_pts = OGG->stream->buffer_elem->start_pts;
    /* set ptr to next element */
    OGG->stream->buffer_elem =
      (buffer_elem_t *) OGG->stream->buffer_elem->next_elem;
    /* update stats */
    OGG->stream->buffer_packets_current--;
    OGG->stream->buffer_bytes_current -= size;
    /* free buffer_elemt */
    free (buffer_elem);
    /* done */
    return size;
  }

  /* read from stream - and buffer if it's for another stream */
  while (1)
  {
    size = OGG_streaming_read_data (OGG, OGM_ALL_STREAMS, buffer, keyframe);
    if (size <= 0)
    {
#if OGMLIB_VERBOSE_BUFFER
      printf
	("ogmlib.c: OGG_read_streaming_data failed and nothing in buffer - failed!\n");
#endif
      return size;
    }
    if (serial_no != ogg_page_serialno (&OGG->page))
    {
      /* check if we should buffer it ? */
      for (i = 0; i < MAX_STREAMS_TO_BUFFER; i++)
      {
	if (OGG->buffered_streams[i] == ogg_page_serialno (&OGG->page))
	{
	  /* found one to buffer */

	  /* find stream which belongs to serial we should buffer */
	  buffer_stream = find_stream (ogg_page_serialno (&OGG->page), OGG);
	  if (buffer_stream == NULL)
	  {
	    /* !!! this shouldn't ever happen !!! */
	    printf
	      ("ogmlib.c: (fdes %i) Cant' buffer packet for an unknown serial "
	       "%d !?\n", OGG->fdes, ogg_page_serialno (&OGG->page));
	    break;
	  }

	  /* we buffer it cause it's for another stream */
#if OGMLIB_VERBOSE_BUFFER
	  printf ("ogmlib.c: buffering for serial no %i (size %lu)\n",
		  OGG->buffered_streams[i], size);
#endif
	  /* malloc struct */
	  buffer_elem = (buffer_elem_t *) malloc (sizeof (buffer_elem_t));
	  if (buffer_elem == NULL)
	  {
	    printf ("ogmlib.c: malloc failed. Can't buffer.\n");
	    break;		/* for loop */
	  }

	  /* append elem to list */
	  if (buffer_stream->buffer_elem == NULL)
	  {
	    /* first elem */
	    last_buffer_elem = buffer_stream->buffer_elem = buffer_elem;
	  }
	  else
	  {
	    /* find last elem and append */
	    last_buffer_elem = buffer_stream->buffer_elem;
	    while (last_buffer_elem->next_elem)
	    {
	      last_buffer_elem = last_buffer_elem->next_elem;
	    }
	    last_buffer_elem->next_elem = buffer_elem;
	  }

	  /* size */
	  buffer_elem->data_size = size;
	  /* alloc data buffer */
	  buffer_elem->data_packet = (char *) malloc (size);
	  if (buffer_elem->data_packet == NULL)
	  {
	    printf ("ogmlib.c: malloc failed. Can't buffer.\n");
	    /* revert changes in buffer list */
	    last_buffer_elem->next_elem = NULL;
	    /* free buffer_elem */
	    free (buffer_elem);
	    break;		/* for loop */
	  }
	  /* copy data */
	  memcpy (buffer_elem->data_packet, buffer, size);
	  /* keyframe */
	  buffer_elem->keyframe = *keyframe;
	  /* set timestamps */
	  buffer_elem->end_pts = OGG->end_pts;
	  buffer_elem->start_pts = OGG->start_pts;
	  /* no elem is currently behind */
	  buffer_elem->next_elem = NULL;
	  /* update stats */
	  buffer_stream->buffer_packets_current++;
	  if (buffer_stream->buffer_packets_current >
	      buffer_stream->buffer_packets_max)
	    buffer_stream->buffer_packets_max =
	      buffer_stream->buffer_packets_current;
	  buffer_stream->buffer_bytes_current += size;
	  if (buffer_stream->buffer_bytes_current >
	      buffer_stream->buffer_bytes_max)
	    buffer_stream->buffer_bytes_max =
	      buffer_stream->buffer_bytes_current;
	  break;		/* for loop */
	}
      }
    }
    else
      break;
  }

  /* 
   * check type and set video_serial or audio_serial correct 
   * to ensure general functions return correct data (framerate, ...)
   */
  switch (OGG->stream->stype)
  {
  case 'a':
    OGG->audio_serial = OGG->stream->serial;
    break;
  case 'v':
    OGG->video_serial = OGG->stream->serial;
    break;
  }

  /* well done */
  return size;
}


/*
 * read just next data from stream
 * serial_no = OGM_ALL_STREAMS means all streams!
 * let the user decide what to do with the data 
 */
long
OGG_streaming_read_data (ogg_t * OGG,
			 int serial_no, char *buffer, int *keyframe)
{
  long len;
  int ret, duration;
  /* find stream in internal list */
  if (serial_no == OGM_ALL_STREAMS)
  {
    /* set to first stream to ensure not accessing invalid streams */
    if (OGG->stream == NULL)
      OGG->stream = OGG->first;
  }
  else
  {
    OGG->stream = find_stream (serial_no, OGG);
    if (OGG->stream == NULL)
    {
#if OGMLIB_VERBOSE
      printf
	("ogmlib.c: (fdes %i) Encountered packet for an unknown serial "
	 "%d !?\n", OGG->fdes, serial_no);
#endif
      return -1;
    }
  }

  /* will be left either when a frame is ready or the end of stream is reached */
  while (1)
  {

    /* packet out */
    while (ogg_stream_packetout (&OGG->stream->instate, &OGG->pack) == 1)
    {
      /* set timestamp */
      OGG->stream->this_granulepos = ogg_page_granulepos (&OGG->page);
      /* we got one frame, but maybe there's one more in the packet - try to get next time */
      if ((len =
	   handle_packet (OGG->stream, &OGG->pack,
			  &OGG->page, OGG, buffer, keyframe, &duration)) > 0)
      {
#if OGMLIB_VERBOSE
	/* should this frame get more bytes from another packe ? */
	if (duration > len)
	  printf ("\n\nogmlib.c: BUG in ogmlib.c. Please report.\n\n");
#endif
	return len;
      }

    }

    /* remember last timestamp */
    OGG->stream->last_granulepos = OGG->stream->this_granulepos;

    /* read a complete page from disk */
    ret = read_page (OGG, OGG->sync, &OGG->page, &OGG->stream->fpos);
    if (ret != 0)
    {
#if OGMLIB_VERBOSE
      printf ("ogmlib.c: (fdes %i) read_page failed\n", OGG->fdes);
#endif
      OGG->stream->eos = 1;
      return ret;
    }

    /* read all serial nos (which is the basically the same as streaming) */
    if (serial_no == OGM_ALL_STREAMS)
    {
      /* enabale streaming (to disable seeking for sure!) */
      OGG->streaming = 1;
      /* find correct stream */
      OGG->stream = find_stream (ogg_page_serialno (&OGG->page), OGG);
      if (OGG->stream == NULL)
	return -1;
      /* eos ? */
      if (OGG->stream->eos)
	return -1;
    }

    /* add page */
    ogg_stream_pagein (&OGG->stream->instate, &OGG->page);
  }				/* end while(1) */

  /* failed */
  return -1;
}

void
OGG_streaming_stats (ogg_t * OGG)
{
  /* check all streams and verbose stats */
  stream_t *cur = OGG->first;
  while (cur != NULL)
  {
    printf ("ogmlib.c: \n");
    printf ("Cache stats for stream w/ serial %i:\n", cur->serial);
    printf (" packets: current %i , max %i\n",
	    cur->buffer_packets_current, cur->buffer_packets_max);
    printf (" bytes:   current %lu , max %lu\n",
	    cur->buffer_bytes_current, cur->buffer_bytes_max);
    cur = cur->next;
  }
}






/*
 * simple video functions below
 */
long
OGG_video_frames (ogg_t * OGG)
{
  OGG->stream = find_stream (OGG->video_serial, OGG);
  if (OGG->stream)
    return (long) OGG->stream->vi.frames;
  return 0;
}


double
OGG_video_length (ogg_t * OGG)
{
  OGG->stream = find_stream (OGG->video_serial, OGG);
  if (OGG->stream)
    return OGG->stream->vi.highest_pts;
  return 0;
}


int
OGG_video_width (ogg_t * OGG)
{
  OGG->stream = find_stream (OGG->video_serial, OGG);
  if (OGG->stream)
    return OGG->stream->vi.width;
  return 0;
}


int
OGG_video_height (ogg_t * OGG)
{
  OGG->stream = find_stream (OGG->video_serial, OGG);
  if (OGG->stream)
    return OGG->stream->vi.height;
  return 0;
}


double
OGG_frame_rate (ogg_t * OGG)
{
  OGG->stream = find_stream (OGG->video_serial, OGG);
  if (OGG->stream)
    return OGG->stream->sample_rate;
  return 0;
}


char *
OGG_video_compressor (ogg_t * OGG)
{
  OGG->stream = find_stream (OGG->video_serial, OGG);
  if (OGG->stream)
    return OGG->stream->vi.compressor;
  return "\0";
}


int
OGG_set_video_track (ogg_t * OGG, int track)
{
  stream_t *cur = OGG->first;
  while (cur != NULL)
  {
    if ((cur->sno == track) && (cur->stype == 'v'))
    {
      OGG->video_serial = cur->serial;
      return 0;
    }
    cur = cur->next;
  }
  return -1;
}


int
OGG_get_video_track (ogg_t * OGG)
{
  OGG->stream = find_stream (OGG->video_serial, OGG);
  if (!OGG->stream)
    return -1;
  return OGG->stream->sno;
}


int
OGG_get_video_serial (ogg_t * OGG)
{
  OGG->stream = find_stream (OGG->video_serial, OGG);
  if (!OGG->stream)
    return -1;
  return OGG->stream->serial;
}


int
OGG_video_tracks (ogg_t * OGG)
{
  return OGG->nvstreams;
}




/*
 * simple audio functions below
 */
int
OGG_set_audio_track (ogg_t * OGG, int track)
{
  stream_t *cur = OGG->first;
  while (cur != NULL)
  {
    if ((cur->sno == track) && (cur->stype == 'a'))
    {
      OGG->audio_serial = cur->serial;
      return 0;
    }
    cur = cur->next;
  }
  return -1;
}


int
OGG_get_audio_track (ogg_t * OGG)
{
  OGG->stream = find_stream (OGG->audio_serial, OGG);
  if (!OGG->stream)
    return -1;
  return OGG->stream->sno;
}


int
OGG_get_audio_serial (ogg_t * OGG)
{
  OGG->stream = find_stream (OGG->audio_serial, OGG);
  if (!OGG->stream)
    return -1;
  return OGG->stream->serial;
}


int
OGG_audio_tracks (ogg_t * OGG)
{
  return OGG->nastreams;
}


int
OGG_audio_channels (ogg_t * OGG)
{
  OGG->stream = find_stream (OGG->audio_serial, OGG);
  /* Audio channels, 0 for no audio */
  if (OGG->stream)
    return OGG->stream->ai.a_chans;
  return 0;
}


int
OGG_audio_bits (ogg_t * OGG)
{
  OGG->stream = find_stream (OGG->audio_serial, OGG);
  /* XXX - not sure about "* 8" */
  if (OGG->stream)
    return OGG->stream->ai.a_bits * 8;
  return 0;
}


char *
OGG_audio_compressor (ogg_t * OGG)
{
  OGG->stream = find_stream (OGG->audio_serial, OGG);
  if (OGG->stream)
    return OGG->stream->ai.audio_tag;
  return "";
}


int
OGG_audio_format (ogg_t * OGG)
{
  OGG->stream = find_stream (OGG->audio_serial, OGG);
  if (OGG->stream)
    return (int) OGG->stream->ai.a_fmt;	/* well known audio format */
  return 0;
}


long
OGG_audio_rate (ogg_t * OGG)
{
  OGG->stream = find_stream (OGG->audio_serial, OGG);
  if (OGG->stream)
    return OGG->stream->sample_rate;	/* Rate in Hz */
  return 0;
}


long
OGG_audio_bytes (ogg_t * OGG)
{
  OGG->stream = find_stream (OGG->audio_serial, OGG);
  if (OGG->stream)
    return OGG->stream->ai.bytes;
  return 0;
}


long
OGG_audio_chunks (ogg_t * OGG)
{
  OGG->stream = find_stream (OGG->audio_serial, OGG);
  if (OGG->stream)
    return OGG->stream->ai.chunks;
  return 0;
}


long
OGG_audio_mp3rate (ogg_t * OGG)
{
  OGG->stream = find_stream (OGG->audio_serial, OGG);
  if (OGG->stream)
    return OGG->stream->ai.mp3rate;
  return 0;
}


double
OGG_audio_length (ogg_t * OGG)
{
  OGG->stream = find_stream (OGG->audio_serial, OGG);
  if (OGG->stream)
    return OGG->stream->ai.highest_pts;
  return 0;
}
