#include <sys/mman.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/l4rm/l4rm.h>
#include <l4/log/l4log.h>

#ifdef DEBUG
static int _DEBUG = 1;
#else
static int _DEBUG = 0;
#endif

void * mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
    void * tmp;

    LOGd(_DEBUG, "mmap: length %u, flags %08x", length, flags);
    if (flags & MAP_ANON) // & fd = 0, ...
    {
        tmp = l4dm_mem_allocate(length, 0);  // for each mmap get new ds
        LOGd(_DEBUG, "got mem at %p", tmp);
        return tmp;
    }
    else
    {
        LOGd(_DEBUG, "Impl. me!");
        return NULL;
    }
}

int munmap(void *start, size_t length)
{
    LOGd(_DEBUG, "munmap at %p", start);
    // problem: complete ds is freed, length is ignored
    l4dm_mem_release(start);
    return 0;
}

void * mremap(void * old_address, size_t old_size, size_t new_size, unsigned long flags)
{
    int ret;
    l4dm_dataspace_t ds;
    l4_offs_t offset;
    l4_addr_t map_addr;
    l4_size_t map_size;
    void * addr;
        
    LOGd(_DEBUG, "mremap");

    ret = l4rm_lookup(old_address, &ds, &offset, &map_addr, &map_size);
    if (ret != 0)
        return (void *)-1;

    ret = l4rm_detach(old_address);
    if (ret != 0)
        return (void *)-1;

    ret = l4dm_mem_resize(&ds, new_size);
    if (ret != 0)
        return (void *)-1;

    ret = l4rm_attach(&ds, new_size, 0, L4DM_RW, &addr);
    if (ret != 0)
        return (void *)-1;
    return addr;
}
