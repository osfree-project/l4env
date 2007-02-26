#include "probe_mp3.h"
#include "fileops.h"
#include <stdio.h>
#include <sys/stat.h>

#include <math.h>
#include "mpg123.h"


int
probe_mp3 (const char *filename)
{
  /* 
   * we read the first 64 bytes where a mp3 header normally is,
   * but some files have the ID3(v.2) tag at the beginning, so read more.
   */
  char buf[4096];
  int fdes;
  unsigned long header;

  fdes = fileops_open (filename, 0);
  if (fdes < 0)
    return 0;

  fileops_lseek (fdes, 0, SEEK_SET);
  if (fileops_read (fdes, buf, 4096))
  {
    if (find_mp3_header (buf, 4096, &header) >= 0)
    {
      /* is mp3 */
      fileops_close (fdes);
      return 1;
    }
  }
  fileops_close (fdes);
  return 0;
}



int
get_mp3_info (const char *filename, double *time, int *channels,
	      long *samplerate, int *bitrate)
{
  char buf[64];
  int fdes;
  unsigned long header;
  mp3_header_t mp3_header;
  long filesize;
  struct stat statbuf;

  fdes = fileops_open (filename, 0);
  if (fdes < 0)
    return -1;

  fileops_lseek (fdes, 0, SEEK_SET);
  if (fileops_read (fdes, buf, 64))
  {
    if (find_mp3_header (buf, 64, &header) >= 0)
    {
      //decode header to get bitrate, then file size and calculate length
      decode_mp3_header (header, &mp3_header);
      *bitrate = mp3_tabsel[mp3_header.lsf][mp3_header.bitrate_index];
      if (fileops_fstat (fdes, &statbuf) != 0)
      {
	//printf("probe_mp3.c: fstat failed.\n");
	fileops_close (fdes);
	return -1;
      }
      filesize = statbuf.st_size;
      fileops_close (fdes);
      *bitrate = *bitrate / 8;
      //this works only for CBR -> VBR time will be incorrect
      *time = (double) (filesize / *bitrate);
      *channels = mp3_header.stereo ? 2 : 1;
      *samplerate = mp3_header.sampling_frequency;


      return 0;
    }
  }
  fileops_close (fdes);
  return -1;
}


int
get_mp3_taginfo (const char *filename, char info[128])
{
  /* parts taken from GPL'ed mpg123 */
  int len, i;
  int fdes;
  /* 
   * normally the id3 tag (version 1) is within the last 128 bytes in file.
   * BUT: the file's length might be incorrect (for example for grubfs)
   * to ensure capturing tag, we read 4096 bytes.
   */
  char buf[4096];
  struct id3tag
  {
    char tag[3];
    char title[30];
    char artist[30];
    char album[30];
    char year[4];
    char comment[30];
    unsigned char genre;
  };
  struct id3tag *tag;
  char title[31];
  char artist[31];
  char album[31];
  /* we're silently ignore the genre, year and comment! */
  //char year[5] = { 0, };
  //char comment[31] = { 0, };

  /* valid info ? */
  if (!info)
    return -1;

  /* set info to filename */
  strncpy (info, filename, 128);

  /* open file */
  fdes = fileops_open (filename, 0);
  if (fdes < 0)
    return -1;

  /* look for id3-tag at end of file */
  if ((len = fileops_lseek (fdes, 0, SEEK_END)) < 0)
  {
    fileops_close (fdes);
    return -1;
  }

  if (fileops_lseek (fdes, len - 4096, SEEK_SET) < 0)
  {
    fileops_close (fdes);
    return -1;
  }

  if ((len = fileops_read (fdes, buf, 4096)) < 128)
  {
    fileops_close (fdes);
    return -1;
  }

  /* all fileops done */
  fileops_close (fdes);

  /* 
   * check if TAG is read and tag > 128bytes 
   * there might be ZEROS in string which strstr to fail
   * so find a T as indicator
   */
  tag = NULL;
  for (i = (len - 127); i >= 0; i--)
  {
    if ((buf[i] == 'T') && (buf[i + 1] == 'A') && (buf[i + 2] == 'G'))
    {
      tag = (struct id3tag *) &buf[i];
      break;
    }
  }
  if (tag == NULL)
  {
#if 0
  /* debug output */
    printf ("no tag found (x %p buf %p)\n\n", tag, buf);
    /* didn't found tag? may be it's at the beginning ? */
#endif
    return -1;
  }

  /* here? so we've got an ID3 tag */
  strncpy (title, tag->title, 30);
  strncpy (artist, tag->artist, 30);
  strncpy (album, tag->album, 30);
  //strncpy (year, tag->year, 4);
  //strncpy (comment, tag->comment, 30);
  title[30] = artist[30] = album[30] = '\0';

  /* 
   * ignore empty strings. 
   * sometimes it's 0x20 (space) if nothing is entered 
   * change the last spaces to \0
   */
  i = 29;
  while ((i > 0) && (title[i] == 0x20))
  {
    if (title[i] == 0x20)
      title[i] = '\0';
    i--;
  }
  i = 29;
  while ((i > 0) && (artist[i] == 0x20))
  {
    if (artist[i] == 0x20)
      artist[i] = '\0';
    i--;
  }
  i = 29;
  while ((i > 0) && (album[i] == 0x20))
  {
    if (album[i] == 0x20)
      album[i] = '\0';
    i--;
  }

#if 0
  /* debug output */
  printf ("Artist: %s\n", title);
  printf ("Title: %s\n", artist);
  printf ("Album: %s\n", album);
  //printf ("Comment: %s  Year: %s\n", comment, year);
#endif

  /* tags are filled ? */
  if ((strlen (artist) == 0) && (strlen (title) == 0))
    snprintf (info, 128, filename);
  else if (strlen (album) > 0)
    snprintf (info, 128, "%s-%s (%s)", artist, title, album);
  else
    snprintf (info, 128, "%s-%s", artist, title);

  /* ensure \0 at end! */
  info[127] = '\0';

  /* sucess */
  return 0;
}
