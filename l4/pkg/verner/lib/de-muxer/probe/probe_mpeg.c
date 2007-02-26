
#include <stdio.h>
#include "probe_mpeg.h"

#include "drops-compat.h"

#ifndef uint32_t
#define uint32_t unsigned int
#endif

static uint32_t
read_bytes (int input, int n)
{

  uint32_t res;
  uint32_t i;
  unsigned char buf[6];

  buf[4] = 0;

  i = fileops_read (input, buf, n);

  if (i != n)
  {
    //this->status = DEMUX_FINISHED;
    printf ("not enough bytes read\n");
  }

  switch (n)
  {
  case 1:
    res = buf[0];
    break;
  case 2:
    res = (buf[0] << 8) | buf[1];
    break;
  case 3:
    res = (buf[0] << 16) | (buf[1] << 8) | buf[2];
    break;
  case 4:
    res = (buf[2] << 8) | buf[3] | (buf[1] << 16) | (buf[0] << 24);
    break;
  default:
    printf
      ("probe_mpeg: how how - something wrong in wonderland demux:read_bytes (%d)\n",
       n);
    return 0;;
  }

  return res;
}


int
probe_mpeg (const char *filename)
{


  int ok = 0;
  int ret;
  unsigned char buf[16];

  int input = fileops_open (filename, 0);
  if (input < 0)
  {
    printf ("probe_mpeg: error opening file.\n");
    return 0;
  }

  fileops_lseek (input, 0, SEEK_SET);
  ret = fileops_read (input, buf, 16);
  if (ret == 16)
  {

    if (!buf[0] && !buf[1] && (buf[2] == 0x01))
      switch (buf[3])
      {
      case 0xba:		/* pack header */
	/* skip */
	if ((buf[4] & 0xf0) == 0x20)	/* mpeg1 */
	{
	  uint32_t pckbuf;
	  pckbuf = read_bytes (input, 1);
	  if ((pckbuf >> 4) != 4)
	  {
	    printf ("probe_mpeg: MPEG-1 detected\n");
	    ok = 1;
	    break;
	  }
	}
	else if ((buf[4] & 0xc0) == 0x40)	/* mpeg2 */
	{
	  printf ("probe_mpeg: MPEG-2 detected\n");
	  ok = 1;
	}
	else
	{
	  printf ("probe_mpeg: weird pack header\n");
	}

      case 0xb3:

	/* MPEG video ES */
	ok = 1;
	if ((buf[6] & 0xc0) == 0x80)
	{
	  printf ("probe_mpeg: MPEG-2 ES detected\n");
	}
	else
	{
	  printf ("probe_mpeg: MPEG-1 ES detected\n");
	}

	break;

      default:
	if (buf[3] < 0xb9)
	{
	  ok = 1;
	  printf
	    ("probe_mpeg: looks like an elementary stream - not program stream\n");
	  if ((buf[6] & 0xc0) == 0x80)
	  {
	    printf ("probe_mpeg: MPEG-2 ES detected\n");
	  }
	  else
	  {
	    printf ("probe_mpeg: MPEG-1 ES detected\n");
	  }
	}
	break;
      }
  }
  /*
  else
  {
    printf ("probe_mpeg: read error (bytes=%i)\n", ret);
  }
  */

  fileops_close (input);

  if (ok)
  {
    //printf ("probe_mpeg: MPEG(-1/-2) detected\n");
    return 1;
  }

  return 0;
}
