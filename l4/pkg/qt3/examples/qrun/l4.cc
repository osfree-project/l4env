/* $Id$ */
/*****************************************************************************/
/**
 * \file   examples/qrun/l4.cc
 * \brief  QRun L4 init/loader code.
 *
 * \date   03/10/2005
 * \author Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2005-2006 Technische Universitaet Dresden
 * This file is part of the Qt3 port for L4/DROPS, which is distributed under
 * the terms of the GNU General Public License 2. Please see the COPYING file
 * for details.
 */

// based on 'launchpad' ('loader' example program) by Adam
// Lockorzynski <adam@os.inf.tu-dresden.de>

extern "C" {

#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/l4rm/l4rm.h>
#include <l4/names/libnames.h>
#include <l4/loader/loader-client.h>
#include <l4/loader/loader.h>
#include <l4/log/l4log.h>

}

// ***************************************************************

l4_threadid_t loader_id = L4_INVALID_ID;
l4_threadid_t fprov_id  = L4_INVALID_ID;

char fprov_name[256];
char binary_prefix[256];

// ***************************************************************

bool init(const char *prefix, const char *fprov) {

    if (prefix == NULL)
        prefix = "";
    else if (strlen(prefix) + 1 > sizeof(binary_prefix)) {
        LOG("Path prefix for binaries too long");
        return false;
    }
    strcpy(binary_prefix, prefix);
    strcat(binary_prefix, "/");

    if (fprov == NULL)
        fprov = "TFTP";
    else if (strlen(fprov) > sizeof(fprov_name)) {
        LOG("FPROV name too long");
        return false;
    }
    strcpy(fprov_name, fprov);
    
    if (!names_waitfor_name("LOADER", &loader_id, 30000)) {
        LOG("Dynamic loader LOADER not found");
        return false;
    }
    
    if (!names_waitfor_name(fprov_name, &fprov_id, 30000)) {
        LOG("File provider '%s' not found", fprov_name);
        return false;
    }
    
    return true;
}


bool loaderRun(const char *binary) {

    int error;
    DICE_DECLARE_ENV(env);
    l4dm_dataspace_t dummy_ds = L4DM_INVALID_DATASPACE;
    char             full_path[256];
    char             error_msg[1024] = "";
    char            *error_ptr = error_msg;
    l4_taskid_t      task_ids[l4loader_MAX_TASK_ID];

    if (strlen(binary) + strlen(binary) >= sizeof(full_path)) {
        LOG("Binary name too long");
        return false;
    }
    strcpy(full_path, binary_prefix);
    strcat(full_path, binary);

    error = l4loader_app_open_call(&loader_id, &dummy_ds, full_path,
                                   &fprov_id, 0, task_ids, &error_ptr, &env);

    if (error) {
        LOG("Error %d (%s) loading application\n", error, l4env_errstr(error));
        if (*error_msg)
            LOG("(Loader said: '%s')\n", error_msg);

        return false;
    }

    return true;
}
