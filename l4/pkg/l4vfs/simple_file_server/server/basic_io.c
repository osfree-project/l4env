/**
 * \file   l4vfs/simple_file_server/server/basic_io.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Jens Syckor <js712688@inf.tu-dresden.de>
 * \author Bjoern Doebel <doebel@os.inf.tu-dresden.de>
 */
/* (c) 2004 - 2008 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/dm_phys/dm_phys.h>

#include "basic_io.h"
#include "state.h"
#include "dirs.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <l4/l4vfs/types.h>
#include <l4/env/env.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>

#define MIN(a, b) ((a)<(b)?(a):(b))

clientstate_t clients[MAX_CLIENTS];

#define DEBUG
#ifdef DEBUG
static int _DEBUG = 1;
#else
static int _DEBUG = 0;
#endif

int get_free_clientstate(void)
{
    int i;
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].open == false)
            return i;
    }
    return -1;
}

void free_clientstate(int handle)
{
    clients[handle].open      = false;
    clients[handle].rw_mode   = 0;
    clients[handle].seek_pos  = 0;
    clients[handle].client    = L4_INVALID_ID;
    clients[handle].object_id = L4VFS_ILLEGAL_OBJECT_ID;

}

int clientstate_open(int flags, l4_threadid_t client,
                     local_object_id_t object_id)
{
    int ret;
    simple_file_t *s_file;

    LOGd(_DEBUG, "check for space ...");
    // check for space
    ret = get_free_clientstate();
    if (ret < 0)
        return -ENOMEM;
    LOGd(_DEBUG, "1");

    // check some error conditions
    LOGd(_DEBUG, "2");
    if (object_id < 0)
        return -ENOENT;
    if (object_id == 0) // check for directory open
        ;  // handle root dir
    else
    {   /* object id not allowed */
        if (arraylist->get_elem(files,object_id - 1) == NULL)
        {
            /* creating new files is not supported */
            if (flags & O_CREAT)
                return -ENOSPC;
            else
                return -ENOENT;
        }

        /* get file from list */
        s_file = (simple_file_t *) arraylist->get_elem(files,object_id - 1);

        /* should not happen every entry needs a name */
        if (s_file->name == NULL)
            return -ENOENT;
        LOGd(_DEBUG,"file_name: %s",s_file->name);
    }

    LOGd(_DEBUG, "fill data (flags = %d, o_id = %d)", flags, object_id);
    // fill data and return handle
    clients[ret].open      = true;
    clients[ret].rw_mode   = flags;
    clients[ret].seek_pos  = 0;
    clients[ret].client    = client;
    clients[ret].object_id = object_id;

    return ret;
}

int clientstate_close(int handle, l4_threadid_t client)
{
    LOGd(_DEBUG,"handle = %d",handle);

    // check for open and client
    if (handle < 0 || handle >= MAX_CLIENTS) {
    LOGd(_DEBUG,"undefined handle");
        return -EBADF;
    }
    if (clients[handle].open == false) {
    LOGd(_DEBUG,"handle already closed");
        return -EBADF;
    }
    if (! l4_task_equal(clients[handle].client, client)) {
    LOGd(_DEBUG,"false client tries to close file");
        return -EBADF;
    }

    // clean data
    free_clientstate(handle);
    return 0;
}

int clientstate_access(int mode, l4_threadid_t client,
                       local_object_id_t object_id)
{
    int ret;
    simple_file_t *s_file;

    // check some error conditions
    LOGd(_DEBUG, "2");
    if (object_id < 0)
        return -ENOENT;

    if (object_id == 0)
    { 
        /* access()ing root directory, allow everything but writing,
           which would mean we can create files here */
        ret = (mode & W_OK) ? -EACCES : 0;
    }
    else
    {   /* object id not allowed */
        if (arraylist->get_elem(files,object_id - 1) == NULL)
        {
            return -ENOENT;
        }

        /* get file from list */
        s_file = (simple_file_t *) arraylist->get_elem(files,object_id - 1);

        /* should not happen every entry needs a name */
        if (s_file->name == NULL)
            return -ENOENT;
        LOGd(_DEBUG,"file_name: %s",s_file->name);

        /* Okay, file exists. Now let's have a look at 'mode':
           there are only files left, as we do not support subdirectories,
           so we allow everything except for executing */
        ret = (mode & X_OK) ? -EACCES : 0;
    }
    
    return ret;
}

int clientstate_stat(l4vfs_stat_t *buf, l4_threadid_t client,
                     local_object_id_t object_id)
{
    int ret;
    simple_file_t *s_file;

    // check some error conditions
    LOGd(_DEBUG, "2");
    if (object_id < 0)
        return -ENOENT;

    if (object_id == 0)
    { 
        /* root directory */
        buf->st_dev   = SIMPLE_FILE_SERVER_VOLUME_ID;
        buf->st_ino   = object_id;
        buf->st_mode  = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
        buf->st_nlink = 1;
        buf->st_size  = 0;
        ret = 0;
    }
    else
    {   /* object id not allowed */
        if (arraylist->get_elem(files,object_id - 1) == NULL)
        {
            return -ENOENT;
        }

        /* get file from list */
        s_file = (simple_file_t *) arraylist->get_elem(files,object_id - 1);

        /* should not happen every entry needs a name */
        if (s_file->name == NULL)
            return -ENOENT;
        LOGd(_DEBUG,"file_name: %s",s_file->name);

        /* Okay, file exists. Now let's have a look at 'mode':
           there are only files left, as we do not support subdirectories,
           so we allow everything except for executing */
        buf->st_dev   = SIMPLE_FILE_SERVER_VOLUME_ID;
        buf->st_ino   = object_id;
        buf->st_mode  = S_IFREG | S_IRUSR | S_IWUSR | 
                        S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
        buf->st_nlink = 1;
        buf->st_size  = s_file->length;
        ret = 0;
    }

    return ret;
}

int clientstate_read(object_handle_t fd, l4_int8_t * buf, size_t count)
{
    int ret;
    local_object_id_t oid;
    simple_file_t *s_file;

    LOGd(_DEBUG, "fd = %d, buf = %p, count = %d", fd, buf, count);
    // check errors
    if (fd < 0 || fd >= MAX_CLIENTS)
        return -EBADF;
    if (clients[fd].open == false)
        return -EBADF;
    oid = clients[fd].object_id;
    if (oid == 0)
        return -EISDIR;

    /* get file from list */
    s_file = (simple_file_t *) arraylist->get_elem(files,oid-1);

    LOGd(_DEBUG, "iod = %d, len = %d, seek_pos = %d",
         oid, s_file->length, clients[fd].seek_pos);
    ret = MIN(count, s_file->length - clients[fd].seek_pos);
    memcpy(buf, s_file->data + clients[fd].seek_pos, ret);

    // now update seek_pos
    clients[fd].seek_pos += ret;

    return ret;
}

int clientstate_write(object_handle_t fd, const l4_int8_t * buf, size_t count)
{
    int ret;
    local_object_id_t oid;
    simple_file_t *s_file;

    LOGd(_DEBUG, "fd = %d, buf = %p, count = %d", fd, buf, count);
    // check errors
    if (fd < 0 || fd >= MAX_CLIENTS)
        return -EBADF;
    if (clients[fd].open == false)
        return -EBADF;
    if (!((clients[fd].rw_mode & O_ACCMODE) == O_WRONLY) &&
        !((clients[fd].rw_mode & O_ACCMODE) == O_RDWR))
        return -EBADF;
    oid = clients[fd].object_id;
    if (oid == 0)
        return -EISDIR;

    /* get file from list */
    s_file = (simple_file_t *) arraylist->get_elem(files,oid-1);

    LOGd(_DEBUG, "iod = %d, len = %d, seek_pos = %d",
         oid, s_file->length, clients[fd].seek_pos);
    // abort if file would grow
    if ((ret = count) > (s_file->length - clients[fd].seek_pos))
        return -EFBIG;

    memcpy(s_file->data + clients[fd].seek_pos, buf, ret);

    // now update seek_pos
    clients[fd].seek_pos += ret;

    return ret;
}

int clientstate_select_notify(object_handle_t *fd, int *mode)
{
    local_object_id_t oid;

    LOGd(_DEBUG, "fd = %d, mode = %d", *fd, *mode);
    // check errors
    if (*fd < 0 || *fd >= MAX_CLIENTS)
        return -EBADF;
    if (clients[*fd].open == false)
        return -EBADF;
    oid = clients[*fd].object_id;
    if (oid == 0)
        return -EISDIR;


    /* simple file server currently supports only read operation */
    if (*mode & SELECT_READ)
    {
        *mode = SELECT_READ;
        return 0;
    }
    else
    {
        /* asked for a mode that simple file server does not support */
        return -1;
    }
}

int clientstate_getdents(int fd, l4vfs_dirent_t *dirp, int count, l4_threadid_t client)
{
    local_object_id_t oid;
    int ret;

    if (fd < 0 || (fd >= MAX_CLIENTS))
        return -EBADF;

    if (clients[fd].open == false)
        return -EBADF;

    if (! l4_task_equal(clients[fd].client, client))
        return -EBADF;

    oid = clients[fd].object_id;
    if (oid != 0)
        return -ENOTDIR;

    ret = fill_dirents(clients[fd].seek_pos,(l4vfs_dirent_t *)dirp, &count);
    if (count > 0)
    {
        clients[fd].seek_pos += ret;
        LOGd(_DEBUG, "New seek_pos = %d, offset = %d", clients[fd].seek_pos, ret);
    }

    return count;
}

int clientstate_mmap(l4dm_dataspace_t *ds, size_t length, int prot, int flags, object_handle_t fd, off_t offset)
{
    local_object_id_t oid;
    simple_file_t *s_file;
    int res;
    char *buf;
    l4_uint32_t rights;
    l4_addr_t content_addr;
	static int foo = 0;

	LOGd_Enter(_DEBUG, " call count: %d", ++foo);
    // check errors
    if (fd < 0 || fd >= MAX_CLIENTS)
        return -EBADF;
    if (clients[fd].open == false)
        return -EBADF;
    oid = clients[fd].object_id;
    if (oid == 0)
        return -EISDIR;
    if (offset < 0 || length <= 0)
    return -EINVAL;

    /* get file from list */
    s_file = (simple_file_t *) arraylist->get_elem(files,oid-1);

    // should not happen
    if (s_file == NULL)
        return -EBADF;

    LOGd(_DEBUG, "iod = %d, len = %d (filelen = %d), seek_pos = %d",
         oid, length, s_file->length, clients[fd].seek_pos);
	LOGd(_DEBUG, "flags = %lx", flags);
	LOGd(_DEBUG, "\t%s", flags & MAP_SHARED ? "map_shared" : "map_private");
	if (flags & MAP_ANONYMOUS)
		LOGd(_DEBUG, "\tmap anonymous");
	if (flags & MAP_EXECUTABLE)
		LOGd(_DEBUG, "\tmap executable");
	if (flags & MAP_FIXED)
		LOGd(_DEBUG, "\tmap fixed");
	if (flags & MAP_LOCKED)
		LOGd(_DEBUG, "\tmap locked");
	if (flags & MAP_POPULATE)
		LOGd(_DEBUG, "\tmap populate");

    if (offset > s_file->length)
    {
        return -EINVAL;
    }
    if (s_file->length < offset+length)
    {
        return -EINVAL;
    }

    /* standard right is read_only */
    rights = L4DM_RO;

    if ((flags & MAP_SHARED) && (prot & PROT_WRITE))
        rights = L4DM_RW;

    /* Check, if we already have this chunk mapped. If so, increment
     * ref count and return.
     */
    if (s_file->chunks != NULL)
    {
	simple_file_chunk_t *chunk = s_file->chunks;
	while (chunk != NULL)
	{
	    if (chunk->start == offset && chunk->size == length)
	    {
		LOGd(_DEBUG,"dataspace already exists, share it now");
		ds = chunk->ds;
		chunk->refcnt++;
		return l4dm_share(chunk->ds, clients[fd].client, rights);
	    }
	    chunk = chunk->next;
	}
    }

    /* If we get here, no fitting chuk has been found. */
    res = l4dm_mem_open(L4DM_DEFAULT_DSM, length, 0, 0, s_file->name, ds);
    if (res)
        return res;

    LOGd(_DEBUG, "opened dataspace with name: %s", s_file->name);

    res = l4rm_attach(ds, s_file->length, 0, L4DM_RW, (void *)&content_addr);
    if (res)
    {
       l4dm_close(ds);
       return res;
    }

    LOGd(_DEBUG, "attached ds at address %p", content_addr);

    buf = (char *) content_addr;

    LOGd(_DEBUG, "data %p, offs %d -> %p", s_file->data, offset, s_file->data + offset);
    memcpy(buf, s_file->data + offset, length);

    res = l4rm_detach((void *)buf);
    LOGd(_DEBUG, "res after detach %d", res);
    if (res)
    {
        l4dm_close(ds);
        return res;
    }

    res = l4dm_share(ds, clients[fd].client, rights);
    if (res)
    {
        l4dm_close(ds);
        return res;
    }

    simple_file_chunk_t *ch = (simple_file_chunk_t*)malloc(sizeof(simple_file_chunk_t));
    ch->ds = ds;
    ch->start = offset;
    ch->size = length;
    ch->refcnt = 1;
    ch->next = s_file->chunks;

    s_file->chunks = ch;

    return 0;
}


int clientstate_seek(object_handle_t fd, off_t offset, int whence)
{
    local_object_id_t oid;
    simple_file_t *s_file;
    // check errors
    if (fd < 0 || fd >= MAX_CLIENTS)
        return -EBADF;
    if (clients[fd].open == false)
        return -EBADF;

    oid = clients[fd].object_id;
    if (oid == 0)  // dir ...
    {
        switch (whence)  // fixme: support or handle all the other cases
        {
        case SEEK_SET:
            if (offset != 0)
                return -EINVAL;
            clients[fd].seek_pos = offset;
            break;
        default:
            return -EINVAL;
        }
        return 0;
    }
    else  // normal files ...
    {
        s_file = (simple_file_t *) arraylist->get_elem(files,oid-1);

        switch (whence)
        {
        case SEEK_SET:
            if (offset < 0 || offset > s_file->length)
                return -EINVAL;
            clients[fd].seek_pos = offset;
            break;
        case SEEK_CUR:
            if ((clients[fd].seek_pos + offset < 0) ||
                (clients[fd].seek_pos + offset > s_file->length))
                return -EINVAL;
            clients[fd].seek_pos += offset;
            break;
        case SEEK_END:
            if (offset > 0 || -1 * offset > s_file->length)
                return -EINVAL;
            clients[fd].seek_pos = s_file->length + offset;
            break;
        default:
            return -EINVAL;
        }
    }
    return clients[fd].seek_pos;
}

int clientstate_msync(const l4dm_dataspace_t *ds, l4_addr_t offset,
                      size_t length, int flags)
{
    simple_file_t *s_file;
    int i,size,res=0;
    int flag=0;
    l4_uint8_t *content;
    l4_addr_t content_addr;

    /* set iterator to first list element */
    arraylist->set_iterator(files);

    LOGd(_DEBUG,"msync with offset: %ld, length: %d",offset,length);

    for (i=0;i<arraylist->size(files);i++)
    {
        s_file = (simple_file_t *) arraylist->get_next(files);
		simple_file_chunk_t *ch = s_file->chunks;

		/* File has mmaped regions? */
		while (ch)
		{
			if (ch->ds->id == ds->id)
				break;
			ch = ch->next;
		}

		// Didn't find correct region, continue to next file
		if (!ch)
			continue;

		LOGd(_DEBUG,"found mmaped file");

		res = l4rm_attach(ch->ds, length, offset, L4DM_RW,
					  (void *)&content_addr);

		/* true if ds could not attached */
		if (res)
		{
			assert(res != -L4_EINVAL);

			/* map to POSIX errno */
			if (res == -L4_ENOMEM)
			{
				res = -ENOMEM;
			}
			else
			{
				res = -EACCES;
			}
		}
		else
		{
			content = (l4_uint8_t *) content_addr;

			LOGd(_DEBUG,"write back changed buffer to original file");

			/* calculate amount of bytes to copy */
			size = length;
			if (size > ch->size)
			{
				size = ch->size;
			}

			/* copy to original file */
			memcpy(s_file->data + ch->start + offset, content, size);

			flag=1;
		}

		break;
    }

    if (flag)
    {
        return 0;
    }
    else
    {
        return res;
    }
}

int clientstate_munmap(const l4dm_dataspace_t *ds, l4_addr_t start, size_t length)
{
    simple_file_t *s_file;
    int i, res=1;

    /* set iterator to first list element */
    arraylist->set_iterator(files);

    for (i=0;i<arraylist->size(files);i++) 
    {
        s_file = (simple_file_t *) arraylist->get_next(files);
		simple_file_chunk_t *ch = s_file->chunks;

		while (ch)
		{
			if (ch->ds->id == ds->id)
				break;
			ch = ch->next;
		}

		if (!ch)
			continue;

		/* check if reference count is one and client tries to munmap
		 * complete dataspace */
		if (start == 0
			&& length >= ch->size
			&& ch->refcnt == 1)
		{
			res = l4dm_close(ch->ds);

			//XXX: remove chunk from s_file->chunks!

			break;
		}
		else
		{
			ch->refcnt--;
			res = 0;
			break;
		}
    }

    if (res)
    {
        assert(res != -L4_EINVAL);

        if (res == -L4_EPERM)
        {
            errno = -EACCES;
        }
        else
        {
            errno = -EINVAL;
        }
    }

    return res;
}

