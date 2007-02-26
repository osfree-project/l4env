#include <stdio.h>
#include "probe_wav.h"

#include "drops-compat.h"

#define WAV_SIGNATURE_SIZE 16
/* this is the hex value for 'data' */
#define data_TAG 0x61746164
#define PCM_BLOCK_ALIGN 1024


int probe_wave(const char *filename) 
{

  unsigned char signature[WAV_SIGNATURE_SIZE];
    int fd;

    fd = fileops_open(filename,0);
    if(fd<0)
    {	
	return 0;
    }
  /* check the signature */
  fileops_lseek(fd, 0, SEEK_SET);
 if (fileops_read(fd, signature, WAV_SIGNATURE_SIZE) ==
    WAV_SIGNATURE_SIZE)
 {
  if ((signature[0] == 'R') ||
      (signature[1] == 'I') ||
      (signature[2] == 'F') ||
      (signature[3] == 'F') ||
      (signature[8] == 'W') ||
      (signature[9] == 'A') ||
      (signature[10] == 'V') ||
      (signature[11] == 'E') ||
      (signature[12] == 'f') ||
      (signature[13] == 'm') ||
      (signature[14] == 't') ||
      (signature[15] == ' '))
    {
    // is wav
     fileops_close(fd);
     return 1;
    }
  }
  fileops_close(fd);
  return 0;
}
