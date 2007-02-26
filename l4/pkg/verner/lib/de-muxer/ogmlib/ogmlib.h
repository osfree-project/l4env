/*
  ogmlib -- a replacement of avilib based on OGM container format
	
    ogmlib.h
	    
    Based on ogmtools Written by Moritz Bunkus <moritz@bunkus.org>
    See http://www.bunkus.org/videotools/ogmtools/
    Based on Xiph.org's 'oggmerge' found in their CVS repository
    See http://www.xiph.org
		  
    Distributed under the GPL
    see the file COPYING for details
    or visit http://www.gnu.org/copyleft/gpl.html
*/

#ifndef __OGMLIB_H__
#define __OGMLIB_H__


/* set to enable ouput information */
#define OGMLIB_VERBOSE 1

/* set to enable debug for each OGG-packet */
#define OGMLIB_VERBOSE_PACKET  0

/* set to enable debug for buffering */
#define OGMLIB_VERBOSE_BUFFER  0


/* std/L4 */
#include <stdio.h>

/* ogg bitstream and wave definition */
#include <ogg/ogg.h>


/*
 * info of video stream
 */
struct video_info
{
  int width, height;
  char compressor[8];

  unsigned long long size;	/* bytes read until current pos in stream */
  unsigned long long bytes;	/* total bytes in stream */
  long frames;			/* total frames in stream */
  double highest_pts;
};

/*
 * info of audio stream
 */
struct audio_info
{

  long a_fmt;			/* Audio format, see #defines below */
  long a_chans;			/* Audio channels, 0 for no audio */
  long a_rate;			/* Rate in Hz */
  long a_bits;			/* bits per audio sample */
  long mp3rate;			/* mp3 bitrate kbs */

  long audio_strn;		/* Audio stream number */
  unsigned long long bytes;	/* Total number of bytes of audio data */
  long chunks;			/* Chunks of audio data in the file */

  char audio_tag[8];		/* Tag of audio data */

  unsigned long long size;	/* bytes read in stream */
  double highest_pts;
};


/*
 * struct for buffered elements while streaming 
 */
typedef struct buffer_elem_t
{
    /* next buffer elem */
    void *next_elem;

    /* size of data packet */
    long data_size;
    /* data packet */
    char* data_packet;
    /* timestamps */
    double end_pts;
    double start_pts;
    /* keyframe */
    int keyframe;
    
} buffer_elem_t;

/*
 * stream type (all infos of physical stream)
 */
typedef struct stream_t
{
  int serial;
  int fd;
  unsigned long fpos;
  int eos, comment;
  int sno;
  char stype;
  int default_len;		/* number of samples per packet (default value) */

  struct video_info vi;
  struct audio_info ai;

  double sample_rate;

  ogg_stream_state instate;
  struct stream_t *next;

  int acodec;

  int packetno;
  ogg_int64_t last_granulepos, this_granulepos;

  /* timestamp - last pts */
  double last_pts;

  /* buffer */
  buffer_elem_t *buffer_elem;
  /* buffer stats */
  int buffer_packets_current;
  int buffer_packets_max;
  long buffer_bytes_current;
  long buffer_bytes_max;

} stream_t;


/*
 *
 */
typedef struct
{
  /* file descriptor */
  int fdes;
  /* file position */
  int fpos;
  
  /* first elem of linked list of logical streams */
  stream_t *first;

  /* number of avail streams (audio, video, text, all) */
  int nastreams, nvstreams, ntstreams, numstreams;

  /* current stream */
  stream_t *stream;

  /* video stuff */
  int video_serial;		// stream serial current video track
  /* audio stuff */    
  int audio_serial;		// stream serial current audio track

  /* timestamps from last data*/
  double end_pts; 
  double start_pts;

  /* ogg stuff */
  ogg_sync_state *sync;
  ogg_page page;
  ogg_packet pack;

  /* streaming mode ? */
  int streaming;
  /* streaming buffer */
#define MAX_STREAMS_TO_BUFFER 4
  int buffered_streams[MAX_STREAMS_TO_BUFFER];

} ogg_t;



/*
 * public functions below
 */


/* general functions */

/*
 * open input file as OGM
 * returns NULL on error
 * getIndex reads the complete file to get more infos like bitrates
 */
ogg_t *OGG_open_input_file (char *filename, int getIndex);
/* 
 * close OGM and release all resources
 */ 
int OGG_close (ogg_t * OGG);
/*
 * seek to the beginning of the ogm 
 */
int OGG_seek_start (ogg_t * OGG);
/*
 * select current video/audio track
 */
int OGG_set_video_track (ogg_t * OGG, int track);
int OGG_set_audio_track (ogg_t * OGG, int track);
/*
 * return number of current video/audio track
 */
int OGG_get_video_track (ogg_t * OGG);
int OGG_get_audio_track (ogg_t * OGG);
/*
 * return number of avail video/audio tracks
 */
int OGG_video_tracks (ogg_t * OGG);
int OGG_audio_tracks (ogg_t * OGG);
/*
 * get OGG's serial of current video/audio track
 */
int OGG_get_video_serial (ogg_t * OGG);
int OGG_get_audio_serial (ogg_t * OGG);


/*
 * streaming (better say seek-free) interface
 */

/*
 * open input file as OGM for streaming
 * default is: buffer nothing
 * !!! so use OGG_set_streaming_serial_no to setup streams for caching !!!
 * returns NULL on error
 */
ogg_t *OGG_streaming_open_input_file (char *filename);
/*
 * set streams w/ given serial_no to buffer data (audio chunks/video frames) 
 * until they were read or get flushed
 * first argument is a counter !!
 * serial_no - to buffer
 */
int OGG_streaming_setup_buffer(ogg_t * OGG, int buffer_serial_no, ...);
/*
 * flush buffered data for streams with given serial_no
 * first argument is a counter !!
 */
int OGG_streaming_flush_buffer(ogg_t * OGG, int buffer_serial_no, ...);
/*
 * remove streams with given serial_no from buffer list
 * first argument is a counter !!
 */ 
int OGG_streaming_remove_buffer(ogg_t * OGG, int buffer_serial_no, ...);
/*
 * read data (audio or video) for the given serial from buffer
 * if there's nothing in buffer - read it from stream
 */
long OGG_streaming_read_buffered_data(ogg_t * OGG, int serial_no, char* buffer, int *keyframe);
/*
 * read just next data from stream (no lseek will be called, no buffer is used nor filled)
 * serial_no=OGM_ALL_STREAMS means reading of all streams
 * let the user decide what to do with the data
 */
long OGG_streaming_read_data(ogg_t * OGG, int serial_no, char* buffer, int *keyframe);
#define OGM_ALL_STREAMS -1
/*
 * verbose statistics of buffer usage
 */
void OGG_streaming_stats(ogg_t * OGG);

/*
 * just use OGG_close to close the stream
 */



/* 
 * !!
 * !! functions below applied to current video/audio stream
 * !!
 */


/* video specific */

/*
 * get number of video frames 
 */
long OGG_video_frames (ogg_t * OGG);
/* 
 * get x and y dimension of video 
 */
int OGG_video_width (ogg_t * OGG);
int OGG_video_height (ogg_t * OGG);
/* 
 * get framerate 
 */
double OGG_frame_rate (ogg_t * OGG);
/* 
 * get FOURCC of compressor 
 */
char *OGG_video_compressor (ogg_t * OGG);
/* 
 * returns streamlength of video in ms
 */ 
double OGG_video_length (ogg_t * OGG);	
/*
 * read one and next video frame
 */
long OGG_read_frame (ogg_t * OGG, char *vidbuf, int *keyframe);


/* audio */


/* 
 * get FOURCC of compressor 
 */
char *OGG_audio_compressor (ogg_t * OGG);
/*
 * get audio format (only well known)
 */
int OGG_audio_format (ogg_t * OGG);
/*
 * get number of channels 
 */
int OGG_audio_channels (ogg_t * OGG);
/*
 * get bits per sample
 */
int OGG_audio_bits (ogg_t * OGG);
/*
 * get samplerate
 */
long OGG_audio_rate (ogg_t * OGG);
/*
 * get number of bytes in audio stream
 */
long OGG_audio_bytes (ogg_t * OGG);
/*
 * get number of audio chunks 
 */
long OGG_audio_chunks (ogg_t * OGG);
/*
 * returns streamlength of audio in ms
 */
double OGG_audio_length (ogg_t * OGG);
/*
 * get bitrate of MP3 stream (if selected audio stream is an MP3)
 */
long OGG_audio_mp3rate (ogg_t * OGG);
/*
 * read one and next audio chunk
 */
long OGG_read_audio_chunk (ogg_t * OGG, char *audbuf);

#if 0 /* not implemented yet, and maybe never will be */
/*
 * read bytes of audio stream
 */
long OGG_read_audio (ogg_t * OGG, char *audbuf, long bytes);
#endif


#endif
