#include <l4/dde/dde.h>
#define REGISTER_PROCESS_WORKER() l4dde26_process_add_worker()
#define REMOVE_PROCESS_WORKER() LOG("no support for removing dde26 worker");
