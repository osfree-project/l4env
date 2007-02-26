#include <l4/sys/types.h>

#include <l4/l4vfs/types.h>
#include <l4/l4vfs/file-table.h>

file_desc_t file_descs[MAX_FILES_OPEN];
object_id_t cwd;       // current working directory
object_id_t c_root;    // current root (e.g. for chroot environments)

/* Find empty entry in file_descs table.
 * return -1 on failure
 */
int ft_get_next_free_entry(void)
{
    int i;
    for (i = 0;
         (i < MAX_FILES_OPEN) &&
             (! l4_is_invalid_id(file_descs[i].server_id));
         i++)
        ; // empty body
    
    if (i >= MAX_FILES_OPEN)
        return -1;
    else
        return i;
}

/* Free exactly the one entry given by i
 */
void ft_free_entry(int i)
{
    if ((i < 0) || (i >= MAX_FILES_OPEN))
    {
        // debug statement here
        return;
    }
    file_descs[i].server_id           = L4_INVALID_ID;
    file_descs[i].object_handle       = 0;
    file_descs[i].object_id.volume_id = L4VFS_ILLEGAL_VOLUME_ID;
    file_descs[i].object_id.object_id = L4VFS_ILLEGAL_OBJECT_ID;
}

/* Fill the entry given by i
 */
void ft_fill_entry(int i, file_desc_t fd_s)
{
    if ((i < 0) || (i >= MAX_FILES_OPEN))
    {
        // debug statement here
        return;
    }
    file_descs[i].server_id     = fd_s.server_id;
    file_descs[i].object_handle = fd_s.object_handle;
    file_descs[i].object_id     = fd_s.object_id;
    file_descs[i].user_data     = fd_s.user_data;
}

/* Check if the entry given by index i is open
 * return true if it is open
 * return false otherwise
 */
int ft_is_open(int i)
{
    if ((i < 0) || (i >= MAX_FILES_OPEN))
    {
        // debug statement here
        return false;
    }

    return (! l4_is_invalid_id(file_descs[i].server_id));
}


/* setup datastructures
 */
void ft_init(void)
{
    int i;

    for (i = 0; i < MAX_FILES_OPEN; i++)
    {
        ft_free_entry(i);
    }
    cwd.volume_id    = L4VFS_NAME_SERVER_VOLUME_ID;
    cwd.object_id    = L4VFS_ROOT_OBJECT_ID;
    c_root.volume_id = L4VFS_NAME_SERVER_VOLUME_ID;
    c_root.object_id = L4VFS_ROOT_OBJECT_ID;
}

file_desc_t ft_get_entry(int i)
{
    file_desc_t fd;
    if ((i < 0) || (i >= MAX_FILES_OPEN))
    {
        // debug statement here
        fd.server_id           = L4_INVALID_ID;
        fd.object_handle       = -1;
        fd.object_id.volume_id = L4VFS_ILLEGAL_VOLUME_ID;
        fd.object_id.object_id = L4VFS_ILLEGAL_OBJECT_ID;

        return fd;
    }
    return file_descs[i];
}
