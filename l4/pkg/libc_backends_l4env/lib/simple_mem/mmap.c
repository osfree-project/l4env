/**
 * \file   libc_backends_l4env/lib/simple_mem/mmap.c
 * \brief  Anonymous mmap() / munmap() and mremap().
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <errno.h>
#include <sys/mman.h>

#include <l4/dm_phys/dm_phys.h>
#include <l4/l4rm/l4rm.h>
#include <l4/log/l4log.h>
#include <l4/env/errno.h>
#include <l4/sys/syscalls.h>

#include <l4/libc_backends_l4env/mmap_util.h>

#ifdef DEBUG
static int _DEBUG = 1;
#else
static int _DEBUG = 0;
#endif

void * mmap_anon(void *start, size_t length, int prot, int flags, int fd,
                 off_t offset);

void * mmap_anon(void *start, size_t length, int prot, int flags, int fd,
                 off_t offset)
{
    void * tmp;
    int res;
    l4_addr_t addr;
    l4_uint32_t area;
    l4dm_dataspace_t ds;
    l4_size_t ds_size;

    res = l4dm_mem_open(L4DM_DEFAULT_DSM, length, 0, 0, "libc heap", &ds);

    if (res)
    {
        if (res == -L4_ENOMEM)
            errno = ENOMEM;
        else
            errno = EACCES;

        return MAP_FAILED;
    }

    l4dm_mem_size(&ds,&ds_size);

    LOGd(_DEBUG,"mem_size of created dataspace = %ld", (l4_addr_t)ds_size);

    res = l4rm_area_reserve(ds_size, L4RM_LOG2_ALIGNED, &addr, &area);

    LOGd(_DEBUG,"reserved area with id: %d", area);

    if (res)
    {
        if (res == -L4_ENOMEM)
            errno = ENOMEM;
        else
            errno = EACCES;

        l4dm_close(&ds);

        return MAP_FAILED;
    }


    res = l4rm_area_attach(&ds, area, ds_size, 0, L4DM_RW, &tmp);

    if (res == 0)
    {
        LOGd(_DEBUG,"Attached dataspace to area with id (%d)", area);

        res = add_ds2server(&ds, l4_myself(), area);
        if (res != 0)
        {
            res = l4rm_detach(tmp);

            if (res != 0)
            {
                errno = EINVAL;
            }

            l4rm_area_release(area);
            l4dm_close(&ds);

            return MAP_FAILED;
        }
    }
    else
    {
        LOG("Could not attach area, %s", l4env_errstr(res));

        switch (res) {

            case -L4_EINVAL:
                errno = EINVAL;
                break;

            case -L4_ENOMEM:
            case -L4_ENOMAP:
                errno = ENOMEM;
                break;

            case -L4_EIPC:
                errno = EACCES;
                break;
        }

        l4dm_close(&ds);

        return MAP_FAILED;
    }

    LOGd(_DEBUG,"return address %p", tmp);

    /* zero out the memory
     * This seems to be required at least by the dietlibc internally.
     *
     * While the linux man page for mmap does not state that anonymous
     * memory is zeroed the netbsd man page does.
     */
    /* update: we don't need to zero out the memory anymore, because
     * dm_phys now delivers zero'ed memory
     */
#if 0
    if (tmp != NULL)
    {
        memset(tmp, 0, ds_size);
    }
#endif

    return tmp;
}

int munmap_anon(ds2server_t *current, void *start, size_t length);

int munmap_anon(ds2server_t *current, void *start, size_t length)
{
    if (! current)
    {
        errno = EINVAL;
        return -1;
    }

    // problem: complete ds is freed, length is ignored
    l4dm_mem_release(start);

    // release area
    l4rm_area_release(current->area_id);

    // delete ds2server
    del_ds2server(&(current->ds));

    return 0;
}

#ifdef USE_DIETLIBC
void * mremap(void * old_address, size_t old_size, size_t new_size,
              unsigned long flags)
#else
void * mremap(void * old_address, size_t old_size, size_t new_size,
              int may_move)
#endif
{
    int ret;
    l4dm_dataspace_t ds;
    l4_offs_t offset;
    l4_addr_t map_addr;
    l4_size_t map_size;
    void * addr;
    l4_threadid_t dummy;

    LOGd(_DEBUG, "mremap");

    ret = l4rm_lookup(old_address, &map_addr, &map_size,&ds, &offset, &dummy);
    if (ret != L4RM_REGION_DATASPACE)
        return MAP_FAILED;

    ret = l4rm_detach(old_address);
    if (ret != 0)
        return MAP_FAILED;

    ret = l4dm_mem_resize(&ds, new_size);
    if (ret != 0)
        return MAP_FAILED;

    ret = l4rm_attach(&ds, new_size, 0, L4DM_RW, &addr);
    if (ret != 0)
        return MAP_FAILED;

    return addr;
}
