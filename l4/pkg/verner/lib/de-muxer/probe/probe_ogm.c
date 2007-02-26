#include "probe_ogm.h"
#include "fileops.h"
#include <stdio.h>

int
probe_ogm (const char *filename)
{
  char buf[5];
  int fdes;

  fdes = fileops_open (filename, 0);
  if (fdes < 0)
    return 0;

  fileops_lseek (fdes, 0, SEEK_SET);
  if (fileops_read (fdes, buf, 4))
  {
    if ((buf[0] == 'O')
	&& (buf[1] == 'g') && (buf[2] == 'g') && (buf[3] == 'S'))
    {
      fileops_close (fdes);
      //printf("seems OGM\n");
      return 1;
    }
  }
  fileops_close (fdes);
  return 0;
}
