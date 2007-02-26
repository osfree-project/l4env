/**
 * \file   dietlibc/lib/backends/io/vector_io.c
 * \brief  vector io operations mapped to multiple single io calls
 *
 * \date   2004-06-01
 * \author Jens Syckor <js712688@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/*** GENERAL INCLUDES ***/
#include <errno.h>
#include <unistd.h>
#include <sys/uio.h>

/*** L4-SPECIFIC INCLUDES ***/
#include <l4/l4vfs/basic_io.h>
#include <l4/l4vfs/file-table.h>
#include <l4/log/l4log.h>

#ifdef DEBUG
static int _DEBUG = 1;
#else
static int _DEBUG = 0;
#endif

/** VECTOR IO OPERATION WRITEV USING MULTIPLE WRITE CALLS **/
ssize_t writev(int fd, const struct iovec *vector, int count)
{
    int i, res;
    file_desc_t file_desc;
    ssize_t ret = 0;
    size_t len;
    void *buf;

    if (! ft_is_open(fd))
    {
        errno = EBADF;
        return -1;
    }

    if (!vector)
    {
        errno = EFAULT;
        return -1;
    }

    if (count <= 0 || count > UIO_MAXIOV)
    {
        errno = EINVAL;
        return -1;
    }

    file_desc = ft_get_entry(fd);

    if (l4_is_invalid_id(file_desc.server_id))
    {
        errno = EBADF;
        return -1;
    }

    for (i=0; i < count; i++)
    {
        len = vector[i].iov_len;
        buf = (void *) vector[i].iov_base;

        LOGd(_DEBUG,"len (%d) of iov (%d)", len, i);

        if (! buf || len < 0)
        {
            if (! buf)
            {
                errno = EFAULT;
            }
            else
            {
                errno = EINVAL;
            }

            return ret;
        }

        res = l4vfs_write(file_desc.server_id,
                          file_desc.object_handle, buf, &len);

        // check the case: res < vector[i].iov_len && res >= 0
        if (res == vector[i].iov_len)
        {
            ret += res;
        }
        else
        {
            errno = EAGAIN;
            return ret;
        }
    }

    LOGd(_DEBUG,"bytes written: %d", (int) ret);

    return ret;
}

/** VECTOR IO OPERATION READV USING MULTIPLE READ CALLS **/
ssize_t readv(int fd, const struct iovec *vector, int count)
{
    int i, res;
    file_desc_t file_desc;
    ssize_t ret = 0;
    size_t len;
    char *buf;

    if (! ft_is_open(fd))
    {
        errno = EBADF;
        return -1;
    }

    if (!vector)
    {
        errno = EFAULT;
        return -1;
    }

    if (count <= 0 || count > UIO_MAXIOV)
    {
        errno = EINVAL;
        return -1;
    }

    file_desc = ft_get_entry(fd);

    if (l4_is_invalid_id(file_desc.server_id))
    {
        errno = EBADF;
        return -1;
    }

    for (i=0; i < count; i++)
    {
        len = vector[i].iov_len;
        buf = vector[i].iov_base;

        if (! buf || len < 0)
        {
            if (!buf)
            {
                errno = EFAULT;
            }
            else
            {
                errno = EINVAL;
            }

            return ret;
        }

        res = l4vfs_read(file_desc.server_id, file_desc.object_handle,
                         &buf, &len);

        if (res >= 0)
        {
            ret += res;

            /* could not read complete length but no error */
            if (res != vector[i].iov_len)
            {
                errno = 0;
                return ret;
            }
        }
        else
        {
            errno = -res;
            return -1;
        }
    }

    return ret;
}
