
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gunzip.h"
#include "uncompress.h"

static void *filestart;
static unsigned char *uncompressed_buffer;

/*
 * Upper address for the allocator for gunzip
 */
unsigned long gunzip_upper_mem_linalloc(void)
{
#ifdef ARCH_arm
  return RAM_BASE + 0x03400000;
#else
  extern unsigned long _mod_addr;
  return (_mod_addr - 1) & ~3;
#endif
}

/*
 * Returns true if file is compressed, false if not
 */
static void file_open(void *start,  int size)
{
  filepos = 0;
  filestart = start;
  filemax = size;
  fsmax = 0xffffffff; /* just big */
  compressed_file = 0;

  gunzip_test_header();
}

static int module_read(void *buf, int size)
{
  memcpy(buf, (const void *)((unsigned long)filestart + filepos), size);
  filepos += size;
  return size;
}

int grub_read(unsigned char *buf, int len)
{
  /* Make sure "filepos" is a sane value */
  if ((filepos < 0) || (filepos > filemax))
    filepos = filemax;

  /* Make sure "len" is a sane value */
  if ((len < 0) || (len > (signed)(filemax - filepos)))
    len = filemax - filepos;

  /* if target file position is past the end of
     the supported/configured filesize, then
     there is an error */
  if (filepos + len > fsmax)
    {
      printf("Filesize error %d + %d > %d\n", filepos, len, fsmax);
      return 0;
    }

  if (compressed_file)
    return gunzip_read (buf, len);

  return module_read(buf, len);
}

void *decompress(const char *name, void *start,
                 int size, int size_uncompressed)
{
  unsigned char *retbuf;
  extern unsigned long _mod_end;
  int read_size;

  if (!uncompressed_buffer)
    uncompressed_buffer
      = (unsigned char *)((_mod_end + 0xffff) & ~0xffff);

  retbuf = uncompressed_buffer;

  if (!size_uncompressed)
    return NULL;

  file_open(start, size);

  // don't move data around if the data isn't compressed
  if (!compressed_file)
    return start;

  printf("Uncompressing %14s to %p (%d to %d bytes).\n", name, retbuf, size, size_uncompressed);

  // Add 10 to detect too short given size
  if ((read_size = grub_read(retbuf, size_uncompressed + 10))
      != size_uncompressed)
    {
      printf("Uncorrect decompression: should be %d bytes but got %d bytes.\n",
             size_uncompressed, read_size);
      return NULL;
    }

  //printf("Read %d bytes.\n", read_size);

  // Page aligned (important for data modules to work!)
  uncompressed_buffer += (read_size + 0xfff) & ~0xfff;
  return retbuf;
}
