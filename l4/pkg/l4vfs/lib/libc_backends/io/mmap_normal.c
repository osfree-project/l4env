/**
 * \file   dietlibc/lib/backends/io/mmap_normal.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <l4/dm_phys/dm_phys.h>
#include <l4/log/l4log.h>
#include <l4/env/errno.h>

#include <l4/l4vfs/mmap_io.h>
#include <l4/l4vfs/file-table.h>
#include <l4/libc_backends_l4env/mmap_util.h>


#ifdef DEBUG
static int _DEBUG = 1;
#else
static int _DEBUG = 0;
#endif

void * mmap_normal(void *start, size_t length, int prot, int flags, int fd,
                   off_t offset);

extern int munmap_normal(ds2server_t *current, void *start, size_t length);

void * mmap_normal(void *start, size_t length, int prot, int flags, int fd,
                   off_t offset)
{
    file_desc_t file_desc;
    l4_int32_t res;
    l4_uint32_t ds_flags;
    l4dm_dataspace_t ds;
    l4_addr_t content_addr, addr;
    l4_uint32_t area;
    l4_size_t ds_size;
    void *ret;
    int temp;

    LOGd(_DEBUG,"io backend, fd %d",fd);

    // 1.
    if (! ft_is_open(fd))
    {
        LOG("io backend error, tried mmap on closed fd");
        errno = EBADF;
        return MAP_FAILED;
    }

    // 2.
    file_desc = ft_get_entry(fd);
    if (l4_is_invalid_id(file_desc.server_id))
    { // should not happen
        errno = EBADF;
        return MAP_FAILED;
    }

    // 3.
    // XXX: are you crazy ???
    /*
    ds = (l4dm_dataspace_t *) malloc(sizeof(l4dm_dataspace_t));
    if (! ds)
    {
        errno = ENOMEM;
        return MAP_FAILED;
    }
    */

    // 4.
    res = l4vfs_mmap1(file_desc.server_id,
                      &ds,
                      length,
                      prot,
                      flags,
                      file_desc.object_handle,
                      offset);

    if (res != 0)
    {
        errno = -res;
        return MAP_FAILED;
    }


    // get size of current dataspace
    l4dm_mem_size(&ds, &ds_size);

    // 5. map prot to dataspace flags
    ds_flags = L4DM_RO;

    if ((flags & MAP_SHARED) && (prot & PROT_WRITE))
        ds_flags = L4DM_RW;


    // 6.
    if (start == NULL)
    {
        res = l4rm_area_reserve(ds_size, L4RM_LOG2_ALIGNED, &addr, &area);

        LOGd(_DEBUG,"reserved area with id: %u, result %d", area, res);

        if (res)
        {
            if (res == -L4_ENOMEM)
                errno = ENOMEM;
            else
                errno = EACCES;

            return MAP_FAILED;
        }

        res = l4rm_area_attach(&ds, area, length, offset, ds_flags,
                               (void *)&content_addr);
		LOGd(_DEBUG, "attached area to address %p, result %d", content_addr, res);
    }
    else
    {
        res = l4rm_area_reserve_region((l4_addr_t)start, ds_size,
                                       L4RM_LOG2_ALIGNED, &area);

        LOGd(_DEBUG,"reserved area with id: %d",area);

        if (res)
        {
            if (res == -L4_ENOMEM)
                errno = ENOMEM;
            else
                errno = EACCES;

            return MAP_FAILED;
        }

        res = l4rm_area_attach(&ds, area, length, offset, ds_flags,
                               (void *)&content_addr);
		LOGd(_DEBUG, "attached area to address %p, result %d", content_addr, res);
    }

    if (res)
    {
        LOG("error attaching ds, res: %d", res);

        assert(res != -L4_EINVAL);

        if (res == -L4_ENOMEM)
            errno = ENOMEM;
        else
            errno = EACCES;

        return MAP_FAILED;
    }
    else
    {
        ret = (void *) content_addr;
    }

    // 7. create local status information
    // fixme: this can fail
    temp = add_ds2server(&ds, file_desc.server_id, area);
    assert(temp == 0);  // we don't currently know how to handle too
                        // small management tables

    return ret;
}

int msync(void *start, size_t length, int flags)
{
    int res;
    l4dm_dataspace_t ds;
    l4_offs_t offset;
    l4_addr_t map_addr;
    l4_size_t map_size;
    ds2server_t *current;
    l4_threadid_t dummy;

    LOGd(_DEBUG,"msync called");

    res = l4rm_lookup(start, &map_addr, &map_size, &ds, &offset, & dummy);
    if (res != L4RM_REGION_DATASPACE)
    {
        if (res == -L4_ENOTFOUND)
            errno = EFAULT;
        else
            errno = EACCES;
        return -1;
    }

    LOGd(_DEBUG,"after l4rm_lookup, found dataspace with id %d", ds.id);

    current = get_ds2server(&ds);

    if (! current)
    {
        errno = EFAULT;
        return -1;
    }

    /* because we found server of ds now call msync */
    res = l4vfs_msync(current->id, &ds, offset, length, flags);

    if (res)
    {
        errno = res;
        return -1;
    }

    return 0;
}

int munmap_normal(ds2server_t *current, void *start, size_t length)
{
    int res;
    l4dm_dataspace_t ds;
    l4_offs_t offset;
    l4_addr_t map_addr;
    l4_size_t map_size;
    l4_threadid_t dummy;

    if (! current)
    {
        errno = EFAULT;
        return -1;
    }

    // fixme: we were probably called by munmap, which already did an
    //        l4rm_lookup(), maybe we should not do it again ???
    res = l4rm_lookup(start, &map_addr, &map_size, &ds, &offset, &dummy);

    if (res != L4RM_REGION_DATASPACE)
    {
        if (res == -L4_ENOTFOUND)
            errno = -EFAULT;
        else
            errno = -EACCES;
        return -1;
    }

    res = l4vfs_munmap(current->id, &ds, offset, length);

    if (! res)
    {
        res = del_ds2server(&(current->ds));

        if (res != 0)
            errno = EINVAL;
    }

    return res;
}
