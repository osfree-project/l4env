/*
 * @author Alexander Boettcher
 */

/* (c) 2008 Technische Universitaet Dresden
 * This file is part of TUD-OS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/env/env.h>
#include <l4/l4rm/l4rm.h>
#include <l4/names/libnames.h>
#include <l4/generic_fprov/generic_fprov-client.h>

#include <string.h>

#include "tpmrun.h"

int loadkeyfile(const char * servername, const char * filename, keydata * key)
{
  int err, ret=0;
  l4_threadid_t server_id, dm_id;
  l4dm_dataspace_t ds;
  void *addr;
  l4_size_t size;
  CORBA_Environment _env = dice_default_environment;

  if (!names_waitfor_name(servername, &server_id, 1000))
    return -1;

  dm_id = l4env_get_default_dsm();
  if (l4_is_invalid_id(dm_id))
    return -2;

  if ((err = l4fprov_file_open_call(&server_id,
             filename, &dm_id, 0, &ds, &size, &_env)))
  {
    LOG_Error("Error opening file \"%s\" at %s: %s",
           filename, servername, l4env_errstr(err));
    return -3;
  }

  if ((err = l4rm_attach(&ds, size, 0, L4DM_RO | L4RM_MAP, &addr)))
  {
    LOG_Error("Error attaching dataspace: %s", l4env_errstr(err));
    ret = -4;
    goto e_destroy;
  }

  if ( size == sizeof(*key) * 2 + 1)
    redumpkey(addr, key);
  else
  {
    LOG_Error("size=%d != sizeof(key)=%d",size, sizeof(*key) * 2);
    ret = -5;
  }

  if ((err = l4rm_detach(addr)))
    LOG_Error("l4rm_detach(): %s", l4env_errstr(err));

  e_destroy:

  if ((err = l4dm_close(&ds)))
    LOG_Error("l4dm_close(): %s", l4env_errstr(err));

  return ret;
}
