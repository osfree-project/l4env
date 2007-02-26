#include <l4/l4vfs/volumes.h>
#include <l4/l4vfs/io.h>
#include <l4/l4vfs/basic_name_server.h>

#include <l4/sys/types.h>

#include <stdlib.h>
#include <errno.h>

/* fix me, We share much code with the name server here!
 *         Maybe we should externalize this stuff into a lib?
 */

volume_entry_t volume_entries[NAME_SERVER_MAX_VOLUMES];

void init_volume_entries(void)
{
    int i;
    for (i = 0; i < NAME_SERVER_MAX_VOLUMES; i++)
    {
        volume_entries[i].volume_id = L4VFS_ILLEGAL_VOLUME_ID;
        volume_entries[i].server_id = L4_INVALID_ID;
    }
}

l4_threadid_t server_for_volume(volume_id_t volume_id)
{
    int i;
    for (i = 0; i < NAME_SERVER_MAX_VOLUMES; i++)
    {
        if (volume_entries[i].volume_id == volume_id)
            return volume_entries[i].server_id;
    }
    return L4_INVALID_ID; // nothing found
}

int index_for_volume(volume_id_t volume_id)
{
    int i;
    for (i = 0; i < NAME_SERVER_MAX_VOLUMES; i++)
    {
        if (volume_entries[i].volume_id == volume_id)
            return i;
    }
    return -1; // nothing found
}

int insert_volume_server(volume_id_t volume_id, l4_threadid_t server_id)
{
    int i;
    l4_threadid_t t;

    // check for duplicates first
    t = server_for_volume(volume_id);
    if (! l4_is_invalid_id(t))
        return 1; // we have such an entry allready

    i = first_empty_volume_entry();
    if (i < 0)
        return 2; // error, we are full
    volume_entries[i].volume_id = volume_id;
    volume_entries[i].server_id = server_id;
    return 0;  // ok
}

int remove_volume_server(volume_id_t volume_id, l4_threadid_t server_id)
{
    int i;
    l4_threadid_t t;

    // check if we have the pair really
    t = server_for_volume(volume_id);
    if (! l4_thread_equal(t, server_id))
        return 1; // we dont have the exact pair

    i = index_for_volume(volume_id);
    if (i < 0)
        return 3; // should not happen as we have found volume above

    // reset entry
    volume_entries[i].volume_id = L4VFS_ILLEGAL_VOLUME_ID;
    volume_entries[i].server_id = L4_INVALID_ID;
    return 0;
}

int first_empty_volume_entry(void)
{
    int i;
    for (i = 0; i < NAME_SERVER_MAX_VOLUMES; i++)
    {
        if (volume_entries[i].volume_id == L4VFS_ILLEGAL_VOLUME_ID)
            return i;
    }
    return -1; // nothing found
}

/* Translates a volume_id into a thread_id to talk to a server.
 *
 * Returns:  0 ...... on success
 *           error .. value to put into errno otherwise
 */
int vol_resolve_thread_for_volume_id(volume_id_t v_id, l4_threadid_t * srv)
{
    int ret;

    if (index_for_volume(v_id) >= 0)
    { // we have a local hit
        *srv = server_for_volume(v_id);
        //LOGd(_DEBUG, "locally found server = '" l4util_idfmt "'",
        //     l4util_idstr(*srv));
    }
    else
    { // this means work, lookup translation remotely
        *srv = l4vfs_thread_for_volume(l4vfs_name_server, v_id);
        //LOGd(_DEBUG, "remotely found server = '" l4util_idfmt "'",
        //     l4util_idstr(*srv));

        if (l4_is_invalid_id(*srv))
            return ENOENT;

        // go to server and ask for a connection
        // fixme: currently we only support connection-less servers
        // *srv = open_connection(*srv);
        // now that we have it, store it locally

        ret = insert_volume_server(v_id, *srv);
        //LOGd(_DEBUG, "insert_volume_server = '%d'", ret);
        if (ret == 1)
        { // should not happen
            abort();
        }
        if (ret == 2)
        { // no more room for new mappings
            return ENOMEM;
        }
    }
    return 0;
}
