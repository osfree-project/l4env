/**
 * \file   l4vfs/include/types.h
 * \brief  Global types for l4vfs
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_INCLUDE_TYPES_H_
#define __L4VFS_INCLUDE_TYPES_H_

#include <l4/sys/types.h>

#include <l4/l4vfs/volume_ids.h>

#define true 1
#define false 0

#define L4VFS_WRITE_RCVBUF_SIZE 65536

//#include <sys/types.h>  // for 'sa_family_t'
//#include <sys/un.h>  // for 'struct sockaddr_un'
//#define L4VFS_SOCKET_MAX_ADDRLEN (sizeof(struct sockaddr_un))
// todo: DICE currently can not parse the above, so we hardcode the
//       number for now, bug is filed (#143)
//       The current number 120 leaves some room for compiler padding
//       in the struct
#define L4VFS_SOCKET_MAX_ADDRLEN 120

#define MAX_FILES_OPEN 1024                /**< size of file table in client */

#define L4VFS_ILLEGAL_OBJECT_NAME_CHARS "/:"   /**< illegal chars. for names */
#define L4VFS_ROOT_OBJECT_ID 0      /**< local object_id for usual root node */
#define L4VFS_MAX_NAME 255                     /**< maximum file name length */
#define L4VFS_PATH_SEPARATOR '/'   /**< char. used to separate path elements */
#define L4VFS_PATH_PARENT    ".."     /**< char. used to point to parent dir */
#define L4VFS_PATH_IDENTITY  '.'            /**< char. used to point to self */

#define SELECT_READ         1  /**< non-blocking read notification */
#define SELECT_WRITE        2  /**< non-blocking write notification */
#define SELECT_EXCEPTION    4  /**< exceptional condition notification */

#define L4VFS_ILLEGAL_OBJECT_ID -1     /**< reserved illegal local object_id */
#define L4VFS_ILLEGAL_VOLUME_ID -1           /**< reserved illegal volume_id */

typedef l4_int32_t volume_id_t;
typedef l4_int32_t local_object_id_t;
typedef l4_int32_t object_handle_t;

typedef l4_int32_t  l4vfs_off_t;
typedef l4_int32_t  l4vfs_ssize_t;
typedef l4_uint32_t l4vfs_size_t;
typedef l4_uint32_t l4vfs_socklen_t;
typedef l4_uint32_t l4vfs_mode_t;
typedef l4_int32_t  l4vfs_time_t;

typedef l4_uint32_t l4vfs_blkcnt_t;
typedef l4_uint32_t l4vfs_blksize_t;
typedef l4_uint16_t l4vfs_nlink_t;

typedef l4_uint16_t l4vfs_dev_t;
typedef l4_uint16_t l4vfs_gid_t;
typedef l4_uint16_t l4vfs_uid_t;

typedef struct l4vfs_dirent
{
    local_object_id_t d_ino;
    l4vfs_off_t       d_off;
    l4_uint16_t       d_reclen;
#ifdef USE_UCLIBC
    unsigned char d_type;
#elif defined(USE_DIETLIBC)
#endif
    char              d_name[L4VFS_MAX_NAME + 1];
} l4vfs_dirent_t;

typedef struct l4vfs_stat
{
    volume_id_t       st_dev;
    local_object_id_t st_ino;
    l4vfs_mode_t      st_mode;
    l4vfs_nlink_t     st_nlink;
    l4vfs_uid_t       st_uid;
    l4vfs_gid_t       st_gid;
    l4vfs_dev_t       st_rdev;
    l4vfs_off_t       st_size;
    l4vfs_blksize_t   st_blksize;
    l4vfs_blkcnt_t    st_blocks;
    l4vfs_time_t      st_atime;
    l4vfs_time_t      st_mtime;
    l4vfs_time_t      st_ctime;
} l4vfs_stat_t;

/**
 * @brief identifier for objects
 *
 * Global object_ids comprise a volume_id and an object_id.
 * Both together define exactly one object in the namespace (an object
 * might be a directory too)
 *
 * It seems logical to define object_id == 0 as root object_id,
 * however ext disagrees here.  It uses 2.
 */
typedef struct
{
    volume_id_t       volume_id;
    local_object_id_t object_id;
} object_id_t;

/**
 * @brief base type for client file_table entries
 * 
 * file_desc_t contains the information to address the server of the
 * object in question and the object itself via a handle.
 * Additionally the original object_id is stored to:
 *  - address object via it in case of fstat(), fchdir, ...
 *  - in case of server failure and restart, to re-resolve the
 *    server_id and the object_handle
 */
typedef struct
{
    l4_threadid_t   server_id;
    object_handle_t object_handle;
    object_id_t     object_id;
    void          * user_data; /**< can be used by other APIs to
                                *   store specific data */
} file_desc_t;

#endif
