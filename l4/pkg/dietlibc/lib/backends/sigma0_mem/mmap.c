/**
 * \file   dietlibc/lib/backends/sigma0_mem/mmap.c
 * \brief  
 *
 * \date   09/29/2004
 * \author Alexander Warg  <aw11@os.inf.tu-dresden.de>
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <sys/mman.h>
#include <errno.h>

#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/l4int.h>

#include <stdio.h>

#if !ARCH_arm
#warning fixme: ARM specific heap base and limit
#endif
// fixme: find a better solution here
#define diet_sigma0_heap_base  ((char*)0xb0000000)
#define diet_sigma0_heap_limit ((char*)0xb5000000)

l4_addr_t diet_sigma0_heap_cur_pages;

static l4_threadid_t pager_id = L4_INVALID_ID;

static int get_pager(void)
{
    l4_threadid_t preempter = L4_INVALID_ID;
    l4_umword_t d;
  
    pager_id = L4_INVALID_ID;
    l4_thread_ex_regs(l4_myself(), -1, -1, &preempter, &pager_id, &d, &d, &d);
    return l4_thread_equal(pager_id, L4_NIL_ID);
}

static unsigned request_page(void *addr)
{
    int err;
    l4_msgdope_t result;
    l4_umword_t dw0, dw1;

    if(l4_thread_equal(pager_id, L4_INVALID_ID) && get_pager() != 0)
    {
        /* no pager, can't allocate memory */
        printf("morecore: no pager!\n");
        return 0;
    }

    err = l4_ipc_call(pager_id,
                      L4_IPC_SHORT_MSG, 0xfffffffc, 0,
                      L4_IPC_MAPMSG((l4_umword_t)addr, L4_LOG2_PAGESIZE), 
                      &dw0, &dw1, L4_IPC_NEVER, &result);

    if (err)
    {
        printf("morecore: IPC error 0x%02x!\n", err);
        return 0;
    }

    if (! l4_ipc_fpage_received(result))
    {
        printf("morecore: page request failed!\n");
        return 0;
    }

    return 1;
}

void * mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);

void * mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
    char *addr;

    // some checks
    if (offset < 0)  
    {
        errno = -EINVAL;
        return MAP_FAILED;
    }
    if (! (flags & MAP_ANON))
    {
        printf("mmap() called without MAP_ANON flag, not supported!\n");
        errno = -EINVAL;
        return MAP_FAILED;
    }

    length = (length + (L4_PAGESIZE -1)) & ~(L4_PAGESIZE-1);

    addr = diet_sigma0_heap_base + (diet_sigma0_heap_cur_pages * L4_PAGESIZE);

    if (addr + length > diet_sigma0_heap_limit)
    {
        errno = ENOMEM;
        return MAP_FAILED;
    }

    for (; length > 0; length -= L4_PAGESIZE, diet_sigma0_heap_cur_pages++)
    {
        if (! request_page(diet_sigma0_heap_base + (diet_sigma0_heap_cur_pages * L4_PAGESIZE)))
	{
            errno = ENOMEM;
            return MAP_FAILED;
	}
    }

    return addr;
}

int munmap(void *start, size_t length)
{
    printf("munmap() called: unimplemented!\n");
    errno = EINVAL;
    return -1;
}

void * mremap(void * old_address, size_t old_size, size_t new_size, unsigned long flags)
{
    printf("mremap() called: unimplemented!\n");
    errno = EINVAL;
    return MAP_FAILED;
}
