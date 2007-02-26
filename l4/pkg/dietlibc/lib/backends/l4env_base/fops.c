/*!
 * \file   dietlibc/lib/backends/l4env_base/fops.c
 * \brief  Basic vfs functionality
 *
 * \date   08/19/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/sys/consts.h>
#include <l4/util/l4_macros.h>
#include <l4/semaphore/semaphore.h>

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <l4/dietlibc/fops.h>

l4diet_vfs_file* l4diet_vfs_files[L4DIET_VFS_MAXFILES] = {0,
						          &l4diet_vfs_file1,
							  &l4diet_vfs_file2
							 };

extern inline l4diet_vfs_file* get_file(int nr);
extern inline l4diet_vfs_file* get_file(int nr){
    if(nr>=0 && nr<L4DIET_VFS_MAXFILES) return l4diet_vfs_files[nr];
    __set_errno(EBADF);
    return 0;
}

int close(int fd){
    int err=0;
    l4diet_vfs_file *file;

    if((file = get_file(fd))==0) return -1;
    if(file->close==0 || (err = file->close(file))==0){
	l4diet_vfs_files[fd]=0;
    }
    return err;
}

int read(int fd,void* buf,size_t len){
    l4diet_vfs_file *file;

    if((file = get_file(fd))==0) return -1;
    if(file->read == 0){
	(*__errno_location()) = EBADF;
	return -1;
    }
    return file->read(file, buf, len);
}
int write(int fd,const void* buf,size_t len){
    l4diet_vfs_file *file;

    if((file = get_file(fd))==0) return -1;
    if(file->write == 0){
	(*__errno_location()) = EBADF;
	return -1;
    }
    return file->write(file, buf, len);
}

off_t lseek(int fd, off_t offset, int whence){
    l4diet_vfs_file *file;

    if((file = get_file(fd))==0) return -1;
    if(file->lseek==0){
	(*__errno_location()) = EBADF;
	return -1;
    }
    return file->lseek(file, offset, whence);
}

int fstat(int fd, struct stat *buf){
    l4diet_vfs_file *file;

    if((file = get_file(fd))==0) return -1;
    if(file->fstat == 0){
	(*__errno_location()) = EBADF;
	return -1;
    }
    return file->fstat(file, buf);
}
void* mmap(void *start, size_t length, int prot, int flags, int fd,
	   off_t offset){
    l4diet_vfs_file *file;

    if((file = get_file(fd))==0) return 0;
    if(file->mmap == 0){
	(*__errno_location()) = EBADF;
	return 0;
    }
    return file->mmap(start, length, prot, flags, file, offset);
}

static l4semaphore_t files_lock = L4SEMAPHORE_UNLOCKED;
l4diet_vfs_file* l4diet_vfs_alloc_nr(int nr){
    l4diet_vfs_file*file=0;

    l4semaphore_down(&files_lock);
    if(l4diet_vfs_files[nr] == 0){
	file = calloc(1,sizeof(l4diet_vfs_file));
    }
    l4semaphore_up(&files_lock);
    return file;
}
