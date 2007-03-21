/**
 * \file   libc_backends/lib/self_mem/mmap.c
 * \brief
 *
 * \date   03/12/2007
 * \author Alexander Warg  <aw11@os.inf.tu-dresden.de>
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 * \author Adam Lackorzynski <adam@os.inf.tu-dressden.de>
 */
/* (c) 2004,2007 Technische Universität Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <sys/mman.h>
#include <errno.h>
#include <l4/sys/types.h>
#include <stdio.h>

extern char libc_backend_self_mem[];
const size_t libc_backend_self_mem_size __attribute__ ((weak));
static l4_addr_t heap_cur_offset;

void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);
void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
  char *addr;

  // some checks
  if (!libc_backend_self_mem_size
      || !libc_backend_self_mem
      || libc_backend_self_mem_size & (L4_PAGESIZE - 1)
      || (l4_addr_t)libc_backend_self_mem & (L4_PAGESIZE - 1))
    {
      printf("libc_backend_self_mem: No or non-aligned memory backing store supplied.\n");
      printf("libc_backend_self_mem: p = %p sz = 0x%lx\n",
	     libc_backend_self_mem, (unsigned long)libc_backend_self_mem_size);
      errno = ENOMEM;
      return MAP_FAILED;
    }
  if (offset < 0)
    {
      errno = EINVAL;
      return MAP_FAILED;
    }
  if (! (flags & MAP_ANON))
    {
      printf("mmap() called without MAP_ANON flag, not supported!\n");
      errno = EINVAL;
      return MAP_FAILED;
    }

  length = (length + (L4_PAGESIZE -1)) & ~(L4_PAGESIZE-1);

  addr = libc_backend_self_mem + heap_cur_offset;

  if (addr + length > libc_backend_self_mem + libc_backend_self_mem_size)
    {
      errno = ENOMEM;
      return MAP_FAILED;
    }

  heap_cur_offset += length;

  return addr;
}

int munmap(void *start, size_t length)
{
  printf("munmap() called: unimplemented!\n");
  errno = EINVAL;
  return -1;
}

void *mremap(void * old_address, size_t old_size, size_t new_size,
#ifdef USE_DIETLIBC
             unsigned long flags
#else /* UCLIBC */
	     int may_move
#endif
	     )
{
  printf("mremap() called: unimplemented!\n");
  errno = EINVAL;
  return MAP_FAILED;
}
