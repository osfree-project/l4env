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
const char *fprov_name = "BMODFS";

// ***************************************************************

bool init() {

  if (!names_waitfor_name("LOADER", &loader_id, 30000)) {
    LOG("Dynamic loader LOADER not found\n");
    return false;
  }

  if (!names_waitfor_name(fprov_name, &fprov_id, 30000)) {
    LOG("File provider '%s' not found\n", fprov_name);
    return false;
  }

  return true;
}


bool loaderRun(const char *binary) {

  int error;
  DICE_DECLARE_ENV(env);
  l4dm_dataspace_t dummy_ds = L4DM_INVALID_DATASPACE;
  char error_msg[1024] = "";
  char *error_ptr = error_msg;
  l4_taskid_t task_ids[l4loader_MAX_TASK_ID];

  error = l4loader_app_open_call(&loader_id, &dummy_ds, binary,
                                 &fprov_id, 0, task_ids, &error_ptr, &env);

  if (error) {
    LOG("Error %d (%s) loading application\n", error, l4env_errstr(error));
    if (*error_msg)
      LOG("(Loader said: '%s')\n", error_msg);

    return false;
  }

  return true;
}
