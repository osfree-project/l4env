#include <l4/sys/types.h>

#include <l4/dietlibc/io-types.h>

#include "file-table.h"

file_desc_t file_descs[MAX_FILES_OPEN];
object_id_t cwd;  // current working directory

/* Find empty entry in file_descs table.
 * return -1 on failure
 */
int ft_get_next_free_entry(void)
{
    int i;
    for (i = 0;
         (i < MAX_FILES_OPEN) &&
             (! l4_is_nil_id(file_descs[i].server_id));
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
    file_descs[i].server_id = L4_NIL_ID;
    file_descs[i].object_handle = 0;
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
    file_descs[i].server_id = fd_s.server_id;
    file_descs[i].object_handle = fd_s.object_handle;
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

    return (! l4_is_nil_id(file_descs[i].server_id));
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
    cwd.volume_id = NAME_SERVER_VOLUME_ID;
    cwd.object_id = ROOT_OBJECT_ID;
}

file_desc_t ft_get_entry(int i)
{
    file_desc_t fd;
    if ((i < 0) || (i >= MAX_FILES_OPEN))
    {
        // debug statement here
        fd.server_id = L4_NIL_ID;
        fd.object_handle = -1;
        return fd;
    }
    return file_descs[i];
}
