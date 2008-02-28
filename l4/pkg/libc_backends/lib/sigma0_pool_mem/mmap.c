/**
 * \file   libc_backends/lib/sigma0_pool_mem/mmap.c
 * \brief
 *
 * \date   01/22/2007
 * \author Alexander Warg  <aw11@os.inf.tu-dresden.de>
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 * \author Adam Lackorzynski <adam@os.inf.tu-dressden.de>
 */
/* (c) 2004,2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <sys/mman.h>
#include <errno.h>

#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/l4int.h>
#include <l4/sigma0/sigma0.h>
#include <l4/crtx/ctor.h>
#include <l4/sys/kdebug.h>

#include <stdio.h>

const size_t libc_backend_sigma0_pool_mem_size __attribute__ ((weak))
  = 128 << 10;

// fixme: find a better solution here
#warning fixme: Arbitrary heap base and limit used
#define sigma0_heap_base  ((char*)0xb0000000)
#define sigma0_heap_limit ((char*)0xb5000000)

static l4_addr_t sigma0_heap_cur_offset;
static char *sigma0_heap_end;

static l4_threadid_t pager_id = L4_INVALID_ID;

static int get_pager(void)
{
  pager_id = l4_thread_ex_regs_pager(l4_myself());
  return l4_thread_equal(pager_id, L4_NIL_ID);
}

static unsigned request_page(void *addr)
{
  int err;
  l4_umword_t dummy;

  if(l4_thread_equal(pager_id, L4_INVALID_ID) && get_pager() != 0)
    {
      /* no pager, can't allocate memory */
      outstring("morecore: no pager!\n");
      return 0;
    }

  if ((err = l4sigma0_map_anypage(pager_id, (l4_addr_t)addr,
                                  L4_LOG2_PAGESIZE, &dummy)))
    {
      switch (err)
	{
	case -2: printf("morecore: IPC error!\n");
                 return 0;
	case -3: printf("morecore: page request failed!\n");
                 return 0;
	}
    }

  return 1;
}

static void libc_backend_sigma0_pool_mem_init(void)
{
  char *addr;

  addr = sigma0_heap_base;

  if (addr + libc_backend_sigma0_pool_mem_size > sigma0_heap_limit)
    {
      outstring("Requested pool size for libc_backend_sigma0_pool_mem too big!\n");
      return;
    }

  for (; addr < sigma0_heap_base + libc_backend_sigma0_pool_mem_size;
       addr += L4_PAGESIZE)
    {
      if (!request_page(addr))
	{
          outstring("Allocation of page of ");
          outhex32((l4_umword_t)addr);
          outstring("failed\n");
	  return;
	}
    }

  sigma0_heap_end
    = sigma0_heap_base + libc_backend_sigma0_pool_mem_size;
}
L4C_CTOR(libc_backend_sigma0_pool_mem_init, L4CTOR_BACKEND);

void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);

void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
  char *addr;

  // some checks
  if (!sigma0_heap_end)
    {
      printf("libc_backend_sigma0_pool_mem_init() not called.\n");
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

  addr = sigma0_heap_base + sigma0_heap_cur_offset;

  if (addr + length > sigma0_heap_end)
    {
      errno = ENOMEM;
      return MAP_FAILED;
    }

  sigma0_heap_cur_offset += length;

  return addr;
}

int munmap(void *start, size_t length)
{
  printf("munmap() called: unimplemented!\n");
  errno = EINVAL;
  return -1;
}

void *mremap(void * old_address, size_t old_size, size_t new_size,
	     int __flags, ...)
{
  printf("mremap() called: unimplemented!\n");
  errno = EINVAL;
  return MAP_FAILED;
}
