#include "probe_avi.h"
#include "fileops.h"
#include <stdio.h>
#include <strings.h>

int
probe_avi (const char *filename)
{
  unsigned char buf[13];
  int fdes = 0;

  fdes = fileops_open (filename, 0);
  if (fdes < 0)
    return 0;

  fileops_lseek (fdes, 0, SEEK_SET);
  if (fileops_read (fdes, buf, 12) == 12)
  {
    if ((strncasecmp (buf, "RIFF", 4) == 0) ||
	(strncasecmp (buf + 8, "AVI ", 4) == 0))
    {
      fileops_close (fdes);
//          printf("seems AVI\n");
      return 1;
    }
  }
  fileops_close (fdes);
  return 0;
}
