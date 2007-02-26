/**
 * \file   libc_backends_l4env/lib/mmap/mmap.c
 * \brief  Switch point between usual and anonymous mmap/munmap
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
/*** GENERAL INCLUDES ***/
#include <errno.h>
#include <sys/mman.h>

/*** L4-SPECIFIC INCLUDES ***/
#include <l4/dm_phys/dm_phys.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/sys/syscalls.h>
#include <l4/thread/thread.h>

#include <l4/libc_backends_l4env/mmap_util.h>

#ifdef DEBUG
static int _DEBUG = 1;
#else
static int _DEBUG = 0;
#endif

extern void *
mmap_anon(void *start, size_t length, int prot, int flags,
          int fd, off_t offset) __attribute__((weak));

extern void *
mmap_normal(void * start, size_t length, int prot, int flags,
            int fd, off_t offset) __attribute__((weak));

extern int
munmap_normal(ds2server_t *current, void *start, size_t length)
    __attribute__((weak));

extern int
munmap_anon(ds2server_t *current, void *start, size_t length)
    __attribute__((weak));

void *
mmap(void *start, size_t length, int prot , int flags, int fd, off_t offset)
{
    void * ret = MAP_FAILED;

    LOGd(_DEBUG,"mmap, fd: %d, flags: %d", fd, flags);

    // 1. some error checks
    // skip length check because size_t is unsigned
    if (offset < 0)
    {
        errno = -EINVAL;
        return MAP_FAILED;
    }

    if (fd < 0 && !(flags & MAP_ANON))
    {
        LOGd(_DEBUG,"error in mmap, fd < 0 and flags != MAP_ANON");
        errno = -EBADF;
        return MAP_FAILED;
    }

    // test if address start is page aligned
    if (start != NULL)
    {
        LOGd(_DEBUG, "start: %p, L4_PAGESIZE %d", start, L4_PAGESIZE);

        if (((l4_int32_t)start % L4_PAGESIZE != 0) && (flags & MAP_FIXED))
        {
            LOGd(_DEBUG, "error, address start not page aligned"
                         " and MAP_FIXED flag set");
            errno = -EINVAL;
            return MAP_FAILED;
        }
    }

    if (flags & MAP_ANON)
    {
        if (mmap_anon)
        {
            ret = mmap_anon(start, length, prot, flags, fd, offset);
        }
        else
        {
            LOGd(_DEBUG, "You better link a lib providing mmap_anon!");
        }
    }
    else
    {
        if (mmap_normal)
        {
            LOGd(_DEBUG,"mmmap: try to call mmap_normal");
            ret = mmap_normal(start, length, prot, flags, fd, offset);
        }
        else
        {
            LOGd(_DEBUG, "You better link a lib providing mmap_normal!");
        }
    }

    return ret;
}

int munmap(void *start, size_t length) 
{
    int res;
    l4dm_dataspace_t ds;
    l4_offs_t offset;
    l4_addr_t map_addr;
    l4_size_t map_size;
    ds2server_t *current;
    l4_threadid_t t_id, dummy;

    // 1. some error checks
    if (!start || length == 0)
    {
        errno = -EINVAL;
        return -1;
    }

    // 2. find ds which belongs to memory area
    res = l4rm_lookup(start, &map_addr, &map_size, &ds, &offset, &dummy);

    if (res != L4RM_REGION_DATASPACE)
    {
         if (res == -L4_ENOTFOUND)
             errno = -EFAULT;
         else
             errno = -EACCES;
         return -1;
    }

    current = get_ds2server(&ds);

    if (! current)
    {
        errno = -EFAULT;
        return -1;
    }

    t_id = l4_myself();

    // if the owner of the ds is not in our task, it must have been a
    // remote mmap file server

    // XXX: we now support munmap of dataspaces which where mmaped in
    //      other threads of our task, however, other threads have no
    //      such rights at dm_phys (dm_phys uses a thread_equal and
    //      not a task_equal to check ownership).  Unfortunately,
    //      there is no easy fix for this!
    if (! l4_task_equal(t_id, current->id))
    {
        if (munmap_normal)
        {
            res = munmap_normal(current, start,length);
        }
        else
        {
            LOGd(_DEBUG, "You better link a lib providing munmap_normal!");
            errno = -EACCES;
            return -1;
        }
    }
    else
    {
        if (munmap_anon)
        {
            res = munmap_anon(current, start, length);
        }
        else
        {
            LOGd(_DEBUG, "You better link a lib providing munmap_anon!");
            errno = -EACCES;
            return -1;
        }
    }

    return res;
}
